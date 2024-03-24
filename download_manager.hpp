#ifndef DOWNLOADMANAGER_HPP
#define DOWNLOADMANAGER_HPP

#include <QNetworkReply>
#include <QFile>
#include "ssh_connection.hpp"

class DeployObjec : public QObject
{
public:
    DeployObjec(QNetworkReply* reply, const QString& downloadDir);
    DeployObjec(QNetworkReply* reply, const QString& downloadDir, const SSHConnection& ssh, const QString& destinationDir);

    void startDownload();
    float getSendProgress();

private slots:
    void onNetError(QNetworkReply::NetworkError e);
    void onNetProgress(qint64 read, qint64 total);
    void onNetFinished();

private:
    void deploy();

    bool shouldDeploy;
    QString destinationDir, downloadDir;
    QNetworkReply* reply;
    size_t progress;
    size_t total;
    QFile* file;
    SSHConnection ssh;

    static QHash<QUrl, QString> cache;
};

#endif // DOWNLOADMANAGER_HPP
