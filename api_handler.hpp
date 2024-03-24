#ifndef APIHANDLER_HPP
#define APIHANDLER_HPP

#include <QString>
#include <QList>
#include <QHash>
#include <QMap>
#include <cstddef>
#include <QtNetwork/QNetworkReply>

class ApiHandler
{
public:
    static void setURL(const QString& url);
    static QString checkToken(const QString& token);
    static QList<QString> getBranches(size_t projectID);
    static QMap<size_t, QString> getPipelines(size_t projectID, const QString& branchName);
    static QHash<size_t, QString> getJobs(size_t projectID, size_t pipelineID);

    static const QString& getURL();
    static QList<QString> getProjectNames();
    static size_t getProjectId(const QString& name);
    static QString getProjectName(const QString& token);
    static QString getProjectName(size_t id);

    static QString getArtifactsUrl(size_t projId, size_t jobId);

private:
    static QByteArray makeRequest(const QString& url, const QString& token = "");
    static QString apiURL;
    static QHash<size_t, QString> idToToken;
    static QHash<size_t, QString> idToName;
    static QHash<QString, QByteArray> cache;
};

#endif // APIHANDLER_HPP
