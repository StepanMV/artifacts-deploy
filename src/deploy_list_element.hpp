#ifndef DEPLOY_LIST_ELEMENT_HPP
#define DEPLOY_LIST_ELEMENT_HPP

#include "complex_list_element.hpp"
#include "api_reply.hpp"
#include "ssh_connection.hpp"
#include <QDir>
#include "api_handler.hpp"
#include <filesystem>
#include <QFutureWatcher>

class ZipException : public std::runtime_error
{
public:
    ZipException();
};

class DeployListElement : public ComplexListElement
{
    Q_OBJECT
public:
    DeployListElement(const QString &filename, const QJsonObject &data, SSHConnection *connection = nullptr, QWidget *parent = nullptr);
    virtual ~DeployListElement();

    virtual void setData(const QJsonObject &data) override;
    void showLastUpdate();

public slots:
    void run();

protected slots:
    void onErrorOccured(QNetworkReply::NetworkError e);
    void onDownloadProgress(qint64 read, qint64 total);
    void onFinished();
    void onFilesReady();
    void onUnzipped();

signals:
    void done();
    void filesReady();

protected:
    void recursiveSendFile(const std::vector<std::filesystem::path> &srcs, const std::vector<std::filesystem::path> &dests, SSHConnection *conn, size_t i);

    SSHConnection *connection;
    ApiReply *reply;
    ApiHandler api;

    QDir dir;
    QFile file;
    QFile lockFile;

    QFutureWatcher<int> watcher;
    bool showingLastUpdate = false;
    int countFiles = 0;
    QString directory;
    QString cachePath;
    QString cacheFilename;
    QString cachePathLock;
    QString cacheDir;
    QString display;
};

#endif // DEPLOY_LIST_ELEMENT_HPP