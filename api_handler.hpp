#ifndef APIHANDLER_HPP
#define APIHANDLER_HPP

#include <QString>
#include <QList>
#include <QHash>
#include <QMap>
#include <cstddef>
#include <QtNetwork/QNetworkReply>
#include <QFuture>

class ApiHandler : QObject
{
    Q_OBJECT
public:
    explicit ApiHandler(QObject *parent = nullptr);
    QFuture<bool> checkURL(const QString& url);
    QFuture<void> addToken(const QString& token);
    QFuture<QList<QString>> getBranches(const QString& projectID);
    QFuture<QMap<QString, QString>> getPipelines(const QString& projectID, const QString& branchName);
    QFuture<QMap<QString, QString>> getJobs(const QString& projectID, const QString& pipelineID);

    static void setURL(const QString& url);
    static const QString& getURL();
    static QString getProjectName(const QString& token);
    static QMap<QString, QString> getProjects();

    QString getArtifactsUrl(const QString& projId, const QString& jobId);

private:
    QFuture<QByteArray> makeRequest(const QString& url, const QString& token = "");
    QFuture<void> addTokenPaged(const QString& token, size_t i);

    QNetworkAccessManager qnam;
    static QString apiURL;
    static QMap<QString, QString> idToToken;
    static QMap<QString, QString> idToName;
    static QHash<QString, QByteArray> cache;
};

#endif // APIHANDLER_HPP
