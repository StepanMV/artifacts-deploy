#ifndef DEPLOYOBJECT_HPP
#define DEPLOYOBJECT_HPP

#include <QObject>
#include <QNetworkReply>
#include <QFile>
#include <QListWidgetItem>
#include "ssh_connection.hpp"
#include "api_reply.hpp"
#include <stdexcept>
#include <QDir>

class ZipException : public std::runtime_error
{
public:
    ZipException();
};

class DeployObject : public QObject
{
    Q_OBJECT
public:
    explicit DeployObject(ApiReply *reply, const QString& directory, const QString& cachePath, QListWidgetItem *item, QObject *parent = nullptr);
    explicit DeployObject(ApiReply *reply, const QString& directory, const QString& cachePath, QListWidgetItem *item, SSHConnection *connection, QObject *parent = nullptr);
    ~DeployObject();

private slots:
    void onErrorOccured(QNetworkReply::NetworkError e);
    void onDownloadProgress(qint64 read, qint64 total);
    void onFinished();

signals:
    void done();

private:
    void downloaded();
    void unzip();
    void sendFile(const QString& source, const QString& dest);
    
    ApiReply *reply;
    QString directory;
    QString cachePath;
    QString cacheFilename;
    QString cachePathLock;
    QString cacheParentPath;
    QListWidgetItem *item;
    SSHConnection *connection = nullptr;

    QFile file;
    QFile lockFile;
    QString display;
    QDir dir;
};

#endif // DEPLOYOBJECT_HPP
