#include "api_handler.hpp"
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QUrl>
#include <QObject>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

QString ApiHandler::apiURL;
QHash<size_t, QString> ApiHandler::idToToken;
QHash<size_t, QString> ApiHandler::idToName;
QHash<QString, QByteArray> ApiHandler::cache;

QString ApiHandler::checkToken(const QString& token)
{
    QString firstName;
    for(size_t page = 1; ; ++page)
    {
        QByteArray reply = makeRequest(apiURL + "/projects?simple=true&min_access_level=20&order_by=last_activity_at&per_page=100&page=" + QString::number(page), token);
        QJsonArray data = QJsonDocument::fromJson(reply).array();
        if(data.isEmpty()) break;
        if(firstName.isEmpty()) firstName = data.at(0)["name"].toString();
        for (auto it = data.constBegin(); it != data.constEnd(); ++it)
        {
            QJsonObject val = it->toObject();
            size_t id = val.value("id").toInt();
            QString name = val.value("name").toString();
            idToToken[id] = token;
            idToName[id] = name;
        }
    }

    return firstName;
}

QList<QString> ApiHandler::getBranches(size_t projectID)
{
    QList<QString> branchNames;
    QString url = apiURL + "/projects/" + QString::number(projectID) + "/repository/branches";
    QByteArray reply = makeRequest(url, idToToken[projectID]);
    QJsonArray array = QJsonDocument::fromJson(reply).array();
    for (const QJsonValue& val : array) {
        branchNames.append(val["name"].toString());
    }

    return branchNames;
}

QMap<size_t, QString> ApiHandler::getPipelines(size_t projectID, const QString& branchName)
{
    QMap<size_t, QString> pipelineIdToCommit;
    QMap<QString, QString> commitHashToDescription;
    QString pipelineUrl = apiURL + "/projects/" + QString::number(projectID) + "/pipelines?ref=" + branchName + "&status=success";
    QString commitsUrl = apiURL + "/projects/" + QString::number(projectID) + "/repository/commits/?ref_name=" + branchName;

    QByteArray pipelineReply = makeRequest(pipelineUrl, idToToken[projectID]);
    QByteArray commitsReply = makeRequest(commitsUrl, idToToken[projectID]);

    QJsonArray pipelineArray = QJsonDocument::fromJson(pipelineReply).array();
    QJsonArray commitsArray = QJsonDocument::fromJson(commitsReply).array();

    for (const QJsonValue& val : pipelineArray)
    {
        size_t id = val["id"].toInt();
        pipelineIdToCommit[id] = val["sha"].toString();
    }
    for (const QJsonValue& val : commitsArray)
    {
        QString hash = val["id"].toString();
        commitHashToDescription[hash] = val["title"].toString();
    }
    for (auto it = pipelineIdToCommit.begin(); it != pipelineIdToCommit.end(); ++it)
    {
        pipelineIdToCommit[it.key()] = "#" + QString::number(it.key()) + " " + commitHashToDescription[it.value()];
    }

    return pipelineIdToCommit;
}

QHash<size_t, QString> ApiHandler::getJobs(size_t projectID, size_t pipelineID)
{
    QHash<size_t, QString> jobIdToName;
    QString url = apiURL + "/projects/" + QString::number(projectID) + "/pipelines/" + QString::number(pipelineID) + "/jobs?scope[]=success";

    QByteArray reply = makeRequest(url, idToToken[projectID]);
    QJsonArray array = QJsonDocument::fromJson(reply).array();

    for (auto it = array.begin(); it != array.end(); ++it)
    {
        QJsonObject val = it->toObject();
        size_t id = val.value("id").toDouble();
        jobIdToName[id] = val.value("name").toString();
    }
    return jobIdToName;
}

QByteArray ApiHandler::makeRequest(const QString& url, const QString& token)
{
    QString authUrl = url + (url.contains('?') ? "&" : "?") + "private_token=" + token;
    if(cache.contains(authUrl)) return cache[authUrl];
    QNetworkRequest request = QNetworkRequest(QUrl(authUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) throw reply->error();
    QByteArray data = reply->readAll();
    cache[authUrl] = data;
    return data;
}

QString ApiHandler::getArtifactsUrl(size_t projId, size_t jobId)
{
    QString url = apiURL + "/projects/" + QString::number(projId) + "/jobs/" + QString::number(jobId) + "/artifacts";
    return url + "?private_token=" + idToToken[projId];
}

void ApiHandler::setURL(const QString& apiURL)
{
    ApiHandler::apiURL = apiURL;
    makeRequest(apiURL + "/projects?per_page=1");
}

const QString& ApiHandler::getURL()
{
    return apiURL;
}

QList<QString> ApiHandler::getProjectNames()
{
    return idToName.values();
}

size_t ApiHandler::getProjectId(const QString& name)
{
    for (auto it = idToName.begin(); it != idToName.end(); ++it)
    {
        if (it.value() == name) return it.key();
    }
    return 0;
}

QString ApiHandler::getProjectName(const QString& token)
{
    for (auto it = idToToken.begin(); it != idToToken.end(); ++it)
    {
        if (it.value() == token) return idToName[it.key()];
    }
    return "";
}

QString ApiHandler::getProjectName(size_t id)
{
    return idToName[id];
}
