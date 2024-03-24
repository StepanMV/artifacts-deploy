#include "download_manager.hpp"
#include <QObject>
#include <QFile>

QHash<QUrl, QString> DeployObjec::cache;

DeployObjec::DeployObjec(QNetworkReply* reply, const QString& downloadDir)
{
    this->shouldDeploy = false;
    this->reply = reply;
    this->file = new QFile(downloadDir);
    this->downloadDir = downloadDir;
    file->open(QIODevice::WriteOnly);
}

DeployObjec::DeployObjec(QNetworkReply* reply, const QString& downloadDir, const SSHConnection& ssh, const QString& destinationDir)
{
    this->shouldDeploy = true;
    this->reply = reply;
    this->file = new QFile(downloadDir);
    this->destinationDir = destinationDir;
    this->downloadDir = downloadDir;
    this->ssh = ssh;
    file->open(QIODevice::WriteOnly);
}


void DeployObjec::startDownload()
{
    if(cache.contains(reply->url()))
    {
        this->downloadDir = cache[reply->url()];
        if(shouldDeploy) deploy();
        return;
    }
    QObject::connect(reply, &QNetworkReply::errorOccurred, this, &DeployObjec::onNetError);
    QObject::connect(reply, &QNetworkReply::downloadProgress, this, &DeployObjec::onNetProgress);
    QObject::connect(reply, &QNetworkReply::finished, this, &DeployObjec::onNetFinished);
}

void DeployObjec::onNetError(QNetworkReply::NetworkError e)
{
    reply->deleteLater();
}

void DeployObjec::deploy()
{
    std::thread(&SSHConnection::sendFile, ssh, downloadDir.toStdString(), destinationDir.toStdString());
}

void DeployObjec::onNetProgress(qint64 read, qint64 total)
{
        this->total = total;
        this->progress = read;
        QDataStream out(file);
        out << reply->readAll();

}

void DeployObjec::onNetFinished()
{
        reply->deleteLater();
        file->close();
        cache[reply->url()] = downloadDir;
        if(shouldDeploy) this->deploy();
}
