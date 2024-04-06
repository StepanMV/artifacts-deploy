#include "deploy_object.hpp"
#include <QObject>
#include <QFile>

DeployObject::DeployObject(QObject *parent)
    : QObject{parent}
{}

QHash<QUrl, QString> DeployObject::cache;


DeployObject::DeployObject(QNetworkAccessManager& qnam, const QUrl& url, const QString& downloadDir)
{
    this->shouldDeploy = false;
    this->file = new QFile(downloadDir);
    this->downloadDir = downloadDir;
    file->open(QIODevice::WriteOnly);

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    this->reply = qnam.get(request);
}

DeployObject::DeployObject(QNetworkAccessManager& qnam, const QUrl& url, const QString& downloadDir, const SSHConnection& ssh, const QString& destinationDir)
{
    this->shouldDeploy = true;
    this->file = new QFile(downloadDir);
    this->destinationDir = destinationDir;
    this->downloadDir = downloadDir;
    this->ssh = ssh;
    file->open(QIODevice::WriteOnly);

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    this->reply = qnam.get(request);
}


void DeployObject::startDownload()
{
    if(cache.contains(reply->url()))
    {
        this->downloadDir = cache[reply->url()];
        if(shouldDeploy) deploy();
        return;
    }
    QObject::connect(reply, &QNetworkReply::errorOccurred, this, &DeployObject::onNetError);
    QObject::connect(reply, &QNetworkReply::downloadProgress, this, &DeployObject::onNetProgress);
    QObject::connect(reply, &QNetworkReply::finished, this, &DeployObject::onNetFinished);
}

void DeployObject::onNetError(QNetworkReply::NetworkError e)
{
    reply->deleteLater();
}

void DeployObject::deploy()
{
    ssh.sendFile(downloadDir.toStdString(), destinationDir.toStdString());
}

void DeployObject::onNetProgress(qint64 read, qint64 total)
{
    this->total = total;
    this->progress = read;
//    QDataStream out(file);
//    out << reply->readAll();

}

void DeployObject::onNetFinished()
{
    reply->deleteLater();
    file->close();
    cache[reply->url()] = downloadDir;
    if(shouldDeploy) this->deploy();
}
