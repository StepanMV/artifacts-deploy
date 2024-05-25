#include "deploy_list_element.hpp"
#include <filesystem>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <zip.h>
#include <QSpacerItem>
#include <QLayout>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <thread>
#include "data_manager.hpp"

namespace fs = std::filesystem;

ZipException::ZipException() : std::runtime_error("") {}

int sendFile(const fs::path &src, const fs::path &dest, const QString &machine);
int unzip(const std::string &cacheDir);

DeployListElement::DeployListElement(const QString &filename, const QJsonObject &data, SSHConnection *connection, QWidget *parent)
    : ComplexListElement(filename, data, parent), connection(connection)
{
    this->directory = data["directory"].toString();
    this->cachePath = "cache/" + data["project"].toString() + "_" + data["job"].toString() + "/" +
                      (data["file"].toString().isEmpty() ? "artifacts.zip" : data["file"].toString());
    this->display = displayLabel->text();
    this->cacheDir = cachePath.mid(0, cachePath.lastIndexOf('/'));
    this->cachePathLock = cachePath.mid(0, cachePath.lastIndexOf('.')) + ".lock";
    this->file.setFileName(cachePath);
    this->lockFile.setFileName(cachePathLock);
    this->cacheFilename = cachePath.mid(cachePath.lastIndexOf('/') + 1);
    connect(this, &DeployListElement::done, this, [this]()
            { watcher.disconnect(this); });
}

DeployListElement::~DeployListElement()
{
    watcher.disconnect(this);
    if (connection)
        delete connection;
}

void DeployListElement::setData(const QJsonObject &data)
{
    ComplexListElement::setData(data);
    this->display = displayLabel->text();
    this->cachePath = "cache/" + data["project"].toString() + "_" + data["job"].toString() + "/" +
                      (data["file"].toString().isEmpty() ? "artifacts.zip" : data["file"].toString());
    this->directory = data["directory"].toString();
    this->cacheDir = cachePath.mid(0, cachePath.lastIndexOf('/'));
    this->cachePathLock = cachePath.mid(0, cachePath.lastIndexOf('.')) + ".lock";
    this->file.setFileName(cachePath);
    this->lockFile.setFileName(cachePathLock);
    this->cacheFilename = cachePath.mid(cachePath.lastIndexOf('/') + 1);
}

void DeployListElement::run()
{
    if (this->getData()["file"].toString().contains(";"))
    {
        QStringList files = this->getData()["file"].toString().split(";");
        for (size_t i = 0; i < files.size(); i++)
        {
            QJsonObject data = this->getData();
            data["file"] = files[i];
            auto newEl = new DeployListElement(":/ui/deploy_list_element.ui", data, DataManager::getConnection(data["machine"].toString()), this);
            connect(newEl, &DeployListElement::done, this, [newEl, this]
                    { newEl->deleteLater();
                        countFiles++;
                        if (countFiles == this->getData()["file"].toString().split(";").size()) {
                            countFiles = 0;
                            emit done();
                        } });
            connect(newEl, &DeployListElement::stateChanged, this, [newEl, this](CLEStatus status, const QString &message)
                    { this->setStatus(status, message); });
            newEl->run();
        }
        return;
    }
    this->reply = api.getArtifacts(data["project"].toString(), data["branch"].toString(), data["job"].toString(), data["file"].toString());
    reply->setParent(this);

    connect(this, &DeployListElement::filesReady, this, &DeployListElement::onFilesReady);

    dir.mkpath(cacheDir);
    if (!lockFile.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    {
        // download is in progress somewhere else
        // main file will already exist by the end of the download
        this->setStatus(CLEStatus::PENDING, "BUSY");
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, timer, [this, timer]()
                {
            if (!lockFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return;
            lockFile.close();
            lockFile.remove();
            emit filesReady();
            timer->stop();
            timer->deleteLater(); });
        timer->start(100);
        return;
    }
    else if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    {
        // full file already exists
        // lock is unnecessary
        lockFile.close();
        lockFile.remove();
        emit filesReady();
        return;
    }
    connect(reply, &ApiReply::errorOccurred, this, &DeployListElement::onErrorOccured);
    connect(reply->qReply, &QNetworkReply::downloadProgress, this, &DeployListElement::onDownloadProgress);
    connect(reply->qReply, &QNetworkReply::finished, this, &DeployListElement::onFinished);
}

void DeployListElement::onErrorOccured(QNetworkReply::NetworkError e)
{
    this->setStatus(CLEStatus::FAIL, "NETWORK ERROR");
    file.close();
    file.remove();
    lockFile.close();
    lockFile.remove();
    emit done();
}

void DeployListElement::onDownloadProgress(qint64 read, qint64 total)
{
    if (total == 0)
        return;
    QString percent = QString::number((read * 100) / total);
    this->setStatus(CLEStatus::PENDING, QString::number(read) + "/" + QString::number(total) + " (" + percent + "%)");
    file.write(reply->qReply->readAll());
}

void DeployListElement::onFinished()
{
    file.write(reply->qReply->readAll());
    file.close();
    lockFile.close();
    lockFile.remove();

    emit filesReady();
}

void DeployListElement::onFilesReady()
{
    if (file.fileName().endsWith(".zip"))
    {
        this->setStatus(CLEStatus::PENDING, "UNZIPPING");
        watcher.disconnect(this);
        connect(&watcher, &QFutureWatcher<int>::finished, this, &DeployListElement::onUnzipped);
        QFuture<int> future = QtConcurrent::run(&unzip, cacheDir.toStdString());
        watcher.setFuture(future);
    }
    else
    {
        this->setStatus(CLEStatus::PENDING, "SENDING");
        watcher.disconnect(this);
        connect(&watcher, &QFutureWatcher<int>::finished, this, [this]()
                {
            int status = watcher.result();
            if (status == -1)
            {
                this->setStatus(CLEStatus::FAIL, "SSH ERROR");
                emit done();
                return;
            }
            this->setStatus(CLEStatus::SUCCESS, "DONE");
            emit done(); });

        fs::path src = cachePath.toStdString();
        fs::path dest = (directory + "/" + cacheFilename).toStdString();
        QFuture<int> future = QtConcurrent::run(&sendFile, src, dest, data["machine"].toString());
        watcher.setFuture(future);
    }
}

void DeployListElement::onUnzipped()
{
    int status = watcher.result();
    if (status == -1)
    {
        this->setStatus(CLEStatus::FAIL, "UNZIP ERROR");
        emit done();
        return;
    }
    std::vector<fs::path> srcs;
    std::vector<fs::path> dests;
    for (const auto &entry : fs::recursive_directory_iterator(cacheDir.toStdString()))
    {
        if (entry.is_directory() || entry.path().filename() == "artifacts.zip")
            continue;
        std::string entryPath = entry.path().relative_path().generic_string();
        entryPath = entryPath.substr(cacheDir.length() + 1);
        std::string entryParentPath = entry.path().parent_path().relative_path().generic_string();
        if (entryParentPath.length() == cacheDir.length())
            entryParentPath = "";
        else
            entryParentPath = entryParentPath.substr(cacheDir.length() + 1);
        srcs.push_back(entry.path());
        dests.push_back(fs::path(directory.toStdString() + "/" + entryPath));
    }
    recursiveSendFile(srcs, dests, connection, 0);
}

void DeployListElement::showLastUpdate()
{
    if (!showingLastUpdate)
    {
        QString lastUpdate = data["last_update"].toString().isEmpty() ? "None" : data["last_update"].toString();
        this->displayLabel->setText(lastUpdate);
        this->setStatus(CLEStatus::DEFAULT, "LAST UPDATE");
        showingLastUpdate = true;
    }
    else
    {
        this->displayLabel->setText(display);
        this->setStatus(CLEStatus::DEFAULT, "");
        showingLastUpdate = false;
    }
}

void DeployListElement::recursiveSendFile(const std::vector<std::filesystem::path> &srcs, const std::vector<std::filesystem::path> &dests, SSHConnection *conn, size_t i)
{
    this->setStatus(CLEStatus::PENDING, QString::fromStdString("SENDING " + dests[i].generic_string()));
    watcher.disconnect(this);
    connect(&watcher, &QFutureWatcher<int>::finished, this, [this, srcs, dests, conn, i]()
            {
        int status = watcher.result();
        if (status == -1)
        {
            this->setStatus(CLEStatus::FAIL, "SSH ERROR");
            emit done();
            return;
        }
        if (i + 1 < srcs.size())
            recursiveSendFile(srcs, dests, conn, i + 1);
        else
        {
            this->setStatus(CLEStatus::SUCCESS, "DONE");
            emit done();
        } });

    QFuture<int> future = QtConcurrent::run(&sendFile, srcs[i], dests[i], data["machine"].toString());
    watcher.setFuture(future);
}

int sendFile(const fs::path &src, const fs::path &dest, const QString &machine)
{
    SSHConnection *conn = std::move(DataManager::getConnection(machine));
    if (conn)
    {
        try
        {
            conn->sendFile(src.generic_string(), dest.generic_string());
        }
        catch (const SshError &e)
        {
            return -1;
        }
    }
    else
    {
        try
        {
            std::filesystem::create_directories(dest.parent_path());
            fs::copy(src, dest, fs::copy_options::overwrite_existing);
        }
        catch (const fs::filesystem_error &e)
        {
            return -1;
        }
    }
    delete conn;
    return 0;
}

int unzip(const std::string &cacheDir)
{
    std::string archive = cacheDir + "/artifacts.zip";
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat sb;
    char buf[100];
    int err;
    int i, len;
    int fd;
    long long sum;

    if ((za = zip_open(archive.c_str(), 0, &err)) == NULL)
        return -1;

    for (i = 0; i < zip_get_num_entries(za, 0); i++)
    {
        if (zip_stat_index(za, i, 0, &sb) == 0)
        {
            std::string filePath = cacheDir + "/" + sb.name;
            if (filePath.back() == '/')
                continue;
            fs::create_directories(filePath.substr(0, filePath.find_last_of('/')));
            zf = zip_fopen_index(za, i, 0);
            if (!zf)
                return -1;

            fd = open(filePath.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0644);
            if (fd < 0)
                return -1;

            sum = 0;
            while (sum != sb.size)
            {
                len = zip_fread(zf, buf, 100);
                if (len < 0)
                    return -1;
                write(fd, buf, len);
                sum += len;
            }

            ::close(fd);
            zip_fclose(zf);
        }
    }

    if (zip_close(za) == -1)
        return -1;
    return 0;
}