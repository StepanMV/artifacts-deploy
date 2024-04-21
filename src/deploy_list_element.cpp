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

namespace fs = std::filesystem;

ZipException::ZipException() : std::runtime_error("") {}

DeployListElement::DeployListElement(const QJsonObject &data, SSHConnection *connection, QWidget *parent)
    : ComplexListElement(":/ui/deploy_list_element.ui", data, parent), connection(connection)
{
    this->directory = data["directory"].toString();
    this->cachePath = data["cachePath"].toString();
    this->display = displayLabel->text();
    this->cacheDir = cachePath.mid(0, cachePath.lastIndexOf('/'));
    this->cachePathLock = cachePath.mid(0, cachePath.lastIndexOf('.')) + ".lock";
    this->file.setFileName(cachePath);
    this->lockFile.setFileName(cachePathLock);
    this->cacheFilename = cachePath.mid(cachePath.lastIndexOf('/') + 1);
}

DeployListElement::~DeployListElement()
{
    if (connection)
        delete connection;
}

void DeployListElement::setData(const QJsonObject &data)
{
    ComplexListElement::setData(data);
    this->display = displayLabel->text();
    this->cachePath = data["cachePath"].toString();
    this->directory = data["directory"].toString();
    this->cacheDir = cachePath.mid(0, cachePath.lastIndexOf('/'));
    this->cachePathLock = cachePath.mid(0, cachePath.lastIndexOf('.')) + ".lock";
    this->file.setFileName(cachePath);
    this->lockFile.setFileName(cachePathLock);
    this->cacheFilename = cachePath.mid(cachePath.lastIndexOf('/') + 1);
}

void DeployListElement::run()
{
    this->reply = api.getArtifacts(data["project"].toString(), data["branch"].toString(), data["job"].toString(), data["file"].toString());
    reply->setParent(this);
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
            this->setStatus(CLEStatus::SUCCESS, "DOWN");
            lockFile.close();
            lockFile.remove();
            downloaded();
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
        this->setStatus(CLEStatus::SUCCESS, "DOWN");
        downloaded();
        return;
    }
    connect(reply, &ApiReply::errorOccurred, this, &DeployListElement::onErrorOccured);
    connect(reply->qReply, &QNetworkReply::downloadProgress, this, &DeployListElement::onDownloadProgress);
    connect(reply->qReply, &QNetworkReply::finished, this, &DeployListElement::onFinished);
}

void DeployListElement::onErrorOccured(QNetworkReply::NetworkError e)
{
    qDebug() << e;
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
    displayLabel->setText("[" + QString::number(read) + "/" + QString::number(total) + " (" + percent + "%)] " + display);
    file.write(reply->qReply->readAll());
}

void DeployListElement::onFinished()
{
    qDebug() << "Finished " << reply->qReply->url();
    this->setStatus(CLEStatus::SUCCESS, "DOWN");
    file.write(reply->qReply->readAll());
    file.close();
    lockFile.close();
    lockFile.remove();

    downloaded();
}

void DeployListElement::downloaded()
{
    if (file.fileName().endsWith(".zip"))
    {
        try
        {
            unzip();
        }
        catch (const ZipException &e)
        {
            this->setStatus(CLEStatus::FAIL, "ZIP ERROR");
            file.remove();
            emit done();
            return;
        }
        for (const auto &entry : fs::recursive_directory_iterator(cacheDir.toStdString()))
        {
            if (entry.is_directory() || entry.path().filename() == "artifacts.zip")
                continue;
            std::string entryPath = entry.path().relative_path().generic_string();
            entryPath = entryPath.substr(cacheDir.length() + 1);
            std::string entryParentPath = entry.path().parent_path().relative_path().generic_string();
            qDebug() << entryPath;
            qDebug() << entryParentPath;
            qDebug() << directory;
            qDebug() << "\n";
            if (entryParentPath.length() == cacheDir.length())
                entryParentPath = "";
            else
                entryParentPath = entryParentPath.substr(cacheDir.length() + 1);
            if (connection)
            {
                sendFile(QString::fromStdString(entry.path()), directory + "/" + QString::fromStdString(entryPath));
            }
            else
            {
                try
                {
                    dir.mkpath(directory + "/" + QString::fromStdString(entryParentPath));
                    fs::copy(entry.path(), fs::path(directory.toStdString() + "/" + entryPath), fs::copy_options::overwrite_existing);
                }
                catch (const fs::filesystem_error &e)
                {
                    this->setStatus(CLEStatus::FAIL, "FILESYSTEM ERROR");
                    emit done();
                    return;
                }
            }
        }
    }
    else
    {
        if (connection)
        {
            sendFile(cachePath, directory);
        }
        else
        {
            dir.mkpath(directory);
            file.copy(directory + "/" + cacheFilename);
        }
    }
    this->setStatus(CLEStatus::SUCCESS, "DONE");
    emit done();
}

void DeployListElement::unzip()
{
    std::string archive = cacheDir.toStdString() + "/artifacts.zip";
    struct zip *za;
    struct zip_file *zf;
    struct zip_stat sb;
    char buf[100];
    int err;
    int i, len;
    int fd;
    long long sum;

    if ((za = zip_open(archive.c_str(), 0, &err)) == NULL)
        throw ZipException();

    for (i = 0; i < zip_get_num_entries(za, 0); i++)
    {
        if (zip_stat_index(za, i, 0, &sb) == 0)
        {
            QString filePath = QString::fromStdString(cacheDir.toStdString()) + "/" + sb.name;
            dir.mkpath(filePath.mid(0, filePath.lastIndexOf('/')));
            zf = zip_fopen_index(za, i, 0);
            if (!zf)
                throw ZipException();

            fd = open(filePath.toStdString().c_str(), O_RDWR | O_TRUNC | O_CREAT, 0644);
            if (fd < 0)
                throw ZipException();

            sum = 0;
            while (sum != sb.size)
            {
                len = zip_fread(zf, buf, 100);
                if (len < 0)
                    throw ZipException();
                write(fd, buf, len);
                sum += len;
            }

            ::close(fd);
            zip_fclose(zf);
        }
    }

    if (zip_close(za) == -1)
        throw ZipException();
}

void DeployListElement::sendFile(const QString &source, const QString &dest)
{
    try
    {
        connection->sendFile(source.toStdString(), dest.toStdString());
    }
    catch (const SshError &e)
    {
        this->setStatus(CLEStatus::FAIL, "SSH ERROR");
        emit done();
        return;
    }
}
