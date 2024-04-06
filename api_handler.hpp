#ifndef APIHANDLER_HPP
#define APIHANDLER_HPP

#include <QString>
#include <QList>
#include <QHash>
#include <QMap>
#include <cstddef>
#include <QtNetwork/QNetworkReply>
#include <QFuture>
#include "api_reply.hpp"

class ApiHandler : public QObject
{
    Q_OBJECT
public:
    explicit ApiHandler(QObject *parent = nullptr);
    ApiReply *checkURL(const QString &url);
    ApiReply *addToken(const QString &token);
    ApiReply *getBranches(const QString &projectID);
    ApiReply *getPipelines(const QString &projectID, const QString &branchName);
    ApiReply *getJobs(const QString &projectID, const QString &pipelineID);
    ApiReply *getArtifacts(const QString &projectID, const QString &jobID, const QString &path = "");

    static void setURL(const QString &url);
    static const QString &getURL();
    static QString getProjectName(const QString &token);
    static QMap<QString, QString> getProjects();

    QString getArtifactsUrl(const QString &projId, const QString &jobId);

private:
    ApiReply *makeRequest(const QString &url, const QString &token = "");
    ApiReply *addTokenPaged(const QString &token, size_t i);

    QNetworkAccessManager qnam;
    static QString apiURL;
    static QMap<QString, QString> idToToken;
    static QMap<QString, QString> idToName;
    static QHash<QString, QByteArray> cache;
};

#endif // APIHANDLER_HPP
