#include "deploy_object.hpp"
#include <QDir>
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
#include <QTimer>
#include <filesystem>

namespace fs = std::filesystem;

ZipException::ZipException() : std::runtime_error("") {}

DeployObject::DeployObject(ApiReply *reply, const QString &directory, const QString &cachePath, QListWidgetItem *item, QObject *parent)
    : QObject{parent}, reply(reply), directory(directory),
      cachePath(cachePath), item(item), display(item->text()),
      cacheParentPath(cachePath.mid(0, cachePath.lastIndexOf('/'))),
      cachePathLock(cachePath.mid(0, cachePath.lastIndexOf('.')) + ".lock"),
      file(cachePath), lockFile(cachePathLock),
      cacheFilename(cachePath.mid(cachePath.lastIndexOf('/') + 1))
{
    dir.mkpath(cacheParentPath);
    if (!lockFile.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    {
        // download is in progress somewhere else
        // main file will already exist by the end of the download
        item->setText("[BUSY] " + display);
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, timer, [this, timer, item]()
                {
            if (!lockFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return;
            item->setText("[DOWN] " + display);
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
        item->setText("[DOWN] " + display);
        downloaded();
        return;
    }
    connect(reply, &ApiReply::errorOccurred, this, &DeployObject::onErrorOccured);
    connect(reply->qReply, &QNetworkReply::downloadProgress, this, &DeployObject::onDownloadProgress);
    connect(reply->qReply, &QNetworkReply::finished, this, &DeployObject::onFinished);
}

DeployObject::~DeployObject()
{
    delete reply;
    if (connection)
        delete connection;
}

DeployObject::DeployObject(ApiReply *reply, const QString &directory, const QString &cachePath, QListWidgetItem *item, SSHConnection *connection, QObject *parent)
    : QObject{parent}, reply(reply), directory(directory),
      cachePath(cachePath), item(item), display(item->text()),
      cacheParentPath(cachePath.mid(0, cachePath.lastIndexOf('/'))),
      cachePathLock(cachePath.mid(0, cachePath.lastIndexOf('.')) + ".lock"),
      file(cachePath), lockFile(cachePathLock), connection(connection),
      cacheFilename(cachePath.mid(cachePath.lastIndexOf('/') + 1))
{
    dir.mkpath(cacheParentPath);
    if (!lockFile.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    {
        // download is in progress somewhere else
        // main file will already exist by the end of the download
        item->setText("[BUSY] " + display);
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, timer, [this, timer, item]()
                {
            if (!lockFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return;
            item->setText("[DOWN] " + display);
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
        item->setText("[DOWN] " + display);
        downloaded();
        return;
    }
    connect(reply, &ApiReply::errorOccurred, this, &DeployObject::onErrorOccured);
    connect(reply->qReply, &QNetworkReply::downloadProgress, this, &DeployObject::onDownloadProgress);
    connect(reply->qReply, &QNetworkReply::finished, this, &DeployObject::onFinished);
}

void DeployObject::onErrorOccured(QNetworkReply::NetworkError e)
{
    item->setText("[ERROR] " + display);
    file.close();
    file.remove();
    lockFile.close();
    lockFile.remove();
    emit done();
}

void DeployObject::onDownloadProgress(qint64 read, qint64 total)
{
    QString percent = QString::number((read * 100) / total);
    item->setText("[" + QString::number(read) + "/" + QString::number(total) + " (" + percent + "%)] " + display);
    file.write(reply->qReply->readAll());
}

void DeployObject::onFinished()
{
    item->setText("[DOWN] " + display);
    file.write(reply->qReply->readAll());
    file.close();
    lockFile.close();
    lockFile.remove();

    downloaded();
}

void DeployObject::downloaded()
{
    if (file.fileName().endsWith(".zip"))
    {
        try
        {
            unzip();
        }
        catch (const ZipException &e)
        {
            item->setText("[ERROR] " + display);
            file.remove();
            emit done();
            return;
        }
        for (const auto &entry : fs::recursive_directory_iterator(cacheParentPath.toStdString()))
        {
            if (entry.is_directory() || entry.path().filename() == "artifacts.zip")
                continue;
            std::string entryPath = entry.path().relative_path().generic_string();
            entryPath = entryPath.substr(cacheParentPath.length() + 1);
            std::string entryParentPath = entry.path().parent_path().relative_path().generic_string();
            if (entryParentPath.length() == cacheParentPath.length())
                entryParentPath = "";
            else
                entryParentPath = entryParentPath.substr(cacheParentPath.length() + 1);
            qDebug() << entryPath << entryParentPath;
            if (connection)
            {
                sendFile(QString::fromStdString(entryPath), directory + "/" + QString::fromStdString(entryPath));
            }
            else
            {
                dir.mkpath(directory + "/" + QString::fromStdString(entryParentPath));
                file.copy(directory + "/" + QString::fromStdString(entryPath));
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
    item->setText("[DONE] " + display);
    emit done();
}

void DeployObject::unzip()
{
    std::string archive = cacheParentPath.toStdString() + "/artifacts.zip";
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
            QString filePath = QString::fromStdString(cacheParentPath.toStdString()) + "/" + sb.name;
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
            close(fd);
            zip_fclose(zf);
        }
    }

    if (zip_close(za) == -1)
        throw ZipException();
}

void DeployObject::sendFile(const QString &source, const QString &dest)
{
    try
    {
        connection->sendFile(source.toStdString(), dest.toStdString());
    }
    catch (const SshError &e)
    {
        item->setText("[ERROR] " + display);
        emit done();
        return;
    }
}
