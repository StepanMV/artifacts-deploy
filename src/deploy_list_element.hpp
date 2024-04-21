#ifndef DEPLOY_LIST_ELEMENT_HPP
#define DEPLOY_LIST_ELEMENT_HPP

#include "complex_list_element.hpp"
#include "api_reply.hpp"
#include "ssh_connection.hpp"
#include <QDir>
#include "api_handler.hpp"

class ZipException : public std::runtime_error
{
public:
    ZipException();
};

class DeployListElement : public ComplexListElement
{
    Q_OBJECT
public:
    DeployListElement(const QJsonObject &data, SSHConnection *connection = nullptr, QWidget *parent = nullptr);
    virtual ~DeployListElement();

    virtual void setData(const QJsonObject &data) override;

public slots:
    void run();

protected slots:
    void onErrorOccured(QNetworkReply::NetworkError e);
    void onDownloadProgress(qint64 read, qint64 total);
    void onFinished();

signals:
    void done();

protected:
    void downloaded();
    void unzip();
    void sendFile(const QString& source, const QString& dest);

    SSHConnection *connection;
    ApiReply *reply;
    ApiHandler api;

    QDir dir;
    QFile file;
    QFile lockFile;

    QString directory;
    QString cachePath;
    QString cacheFilename;
    QString cachePathLock;
    QString cacheDir;
    QString display;
};

#endif // DEPLOY_LIST_ELEMENT_HPP