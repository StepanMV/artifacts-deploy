#ifndef DEPLOYOBJECT_HPP
#define DEPLOYOBJECT_HPP

#include <QObject>
#include <QNetworkReply>
#include <QFile>
#include "ssh_connection.hpp"

class DeployObject : public QObject
{
    Q_OBJECT
public:
    explicit DeployObject(QObject *parent = nullptr);
    DeployObject(QNetworkAccessManager& qnam, const QUrl& url, const QString& downloadDir);
    DeployObject(QNetworkAccessManager& qnam, const QUrl& url, const QString& downloadDir, const SSHConnection& ssh, const QString& destinationDir);

    void startDownload();
    void deploy();

signals:
    void downloaded();

private slots:
    void onNetError(QNetworkReply::NetworkError e);
    void onNetProgress(qint64 read, qint64 total);
    void onNetFinished();

private:
    bool shouldDeploy;
    QString destinationDir, downloadDir;
    QNetworkReply* reply;
    QFile* file;
    size_t progress;
    size_t total;
    SSHConnection ssh;

    static QHash<QUrl, QString> cache;
};

#endif // DEPLOYOBJECT_HPP
