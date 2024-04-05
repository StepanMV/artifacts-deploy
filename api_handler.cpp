#include "api_handler.hpp"
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

QString ApiHandler::apiURL;
QMap<QString, QString> ApiHandler::idToToken;
QMap<QString, QString> ApiHandler::idToName;
QHash<QString, QByteArray> ApiHandler::cache;

QFuture<void> ApiHandler::addToken(const QString &token)
{
  return ApiHandler::addTokenPaged(token, 0);
}

QFuture<void> ApiHandler::addTokenPaged(const QString &token, size_t i)
{
  auto future =
      makeRequest(apiURL +
                      "/projects?simple=true&min_access_level=20&order_by="
                      "last_activity_at&per_page=100&page=" +
                      QString::number(i),
                  token);
  return future.then([this, token, i](QByteArray data)
                     {
    QJsonArray arr = QJsonDocument::fromJson(data).array();
    if (arr.isEmpty())
      return;
    for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
      QJsonObject val = it->toObject();
      QString id = QString::number(val.value("id").toInteger());
      QString name = val.value("name").toString();
      idToToken[id] = token;
      idToName[id] = name;
    }
    addTokenPaged(token, i + 1); });
}

QFuture<QList<QString>>
ApiHandler::getBranches(const QString &projectID)
{

  QString url = apiURL + "/projects/" + projectID + "/repository/branches";
  auto future = makeRequest(url, idToToken[projectID]);
  return future.then([](QByteArray data)
                     {
    QList<QString> branchNames;
    QJsonArray array = QJsonDocument::fromJson(data).array();
    for (const QJsonValue &val : array) {
      QString name = val["name"].toString();
      branchNames.append(name);
    }
    return branchNames; });
}

QFuture<QMap<QString, QString>>
ApiHandler::getPipelines(const QString &projectID, const QString &branchName)
{
  QString pipelineUrl = apiURL + "/projects/" + projectID +
                        "/pipelines?ref=" + branchName + "&status=success";
  QString commitsUrl = apiURL + "/projects/" + projectID +
                       "/repository/commits/?ref_name=" + branchName;

  auto commitsFuture = makeRequest(commitsUrl, idToToken[projectID])
    .then(QtFuture::Launch::Async, [](QByteArray data){
      QMap<QString, QString> commitHashToDescription;
      QJsonArray commitsArray = QJsonDocument::fromJson(data).array();

      for (const QJsonValue &val : commitsArray) {
        QString hash = val["id"].toString();
        commitHashToDescription[hash] = val["title"].toString();
      }
      return commitHashToDescription;
    });

  return makeRequest(pipelineUrl, idToToken[projectID])
    .then(QtFuture::Launch::Async, [commitsFuture](QByteArray data) {
      QMap<QString, QString> pipelineIdToCommit;
      QJsonArray pipelineArray = QJsonDocument::fromJson(data).array();

      for (const QJsonValue &val : pipelineArray) {
        QString id = QString::number(val["id"].toInteger());
        pipelineIdToCommit[id] = val["sha"].toString();
      }
      QMap<QString, QString> commitHashToDescription = commitsFuture.result();
      for (auto it = pipelineIdToCommit.begin(); it != pipelineIdToCommit.end();
          ++it) {
        pipelineIdToCommit[it.key()] =
            "#" + it.key() + " " + commitHashToDescription[it.value()];
      }
      return pipelineIdToCommit;
    }).then(this, [](QMap<QString, QString> pipelineIdToCommit) {
      return pipelineIdToCommit;
    });
}

QFuture<QMap<QString, QString>> ApiHandler::getJobs(const QString &projectID,
                                                    const QString &pipelineID)
{
  QString url = apiURL + "/projects/" + projectID + "/pipelines/" + pipelineID +
                "/jobs?scope[]=success";

  auto future = makeRequest(url, idToToken[projectID]);
  return future.then([](QByteArray data)
                     {
    QMap<QString, QString> jobIdToName;
    QJsonArray array = QJsonDocument::fromJson(data).array();

    for (auto it = array.begin(); it != array.end(); ++it) {
      QJsonObject val = it->toObject();
      QString id = QString::number(val.value("id").toInteger());
      jobIdToName[id] = val.value("name").toString();
    }
    return jobIdToName; });
}

ApiHandler::ApiHandler(QObject *parent)
  :QObject{parent} {}

QFuture<bool> ApiHandler::checkURL(const QString &url)
{
  auto future = makeRequest(url + "/projects?per_page=1");
  return future.then([](QByteArray data) { return true; });
}

QFuture<QByteArray> ApiHandler::makeRequest(const QString &url,
                                            const QString &token)
{
  QString authUrl =
      url + (url.contains('?') ? "&" : "?") + "private_token=" + token;
  // if(cache.contains(authUrl)) return cache[authUrl];
  QNetworkRequest request = QNetworkRequest(QUrl(authUrl));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

  QNetworkReply *reply = qnam.get(request);
  // emits signal with all data in QByteArray... and reply..?
  // ... so it should return reply?
  // so other methods connect to that signal and perform their lambdas if signal.reply == returned reply
  // and then... other... signals?
  return QtFuture::connect(reply, &QNetworkReply::finished)
    .then([reply, authUrl]() {
      if (reply->error() != QNetworkReply::NoError)
        throw reply->error();
      QByteArray data = reply->readAll();
        cache[authUrl] = data;
        reply->deleteLater();
        return data; });
}

QString ApiHandler::getArtifactsUrl(const QString &projId,
                                    const QString &jobId)
{
  QString url =
      apiURL + "/projects/" + projId + "/jobs/" + jobId + "/artifacts";
  return url + "?private_token=" + idToToken[projId];
}

void ApiHandler::setURL(const QString &apiURL) { ApiHandler::apiURL = apiURL; }

const QString &ApiHandler::getURL() { return apiURL; }

QString ApiHandler::getProjectName(const QString &token)
{
  for (auto it = idToToken.begin(); it != idToToken.end(); ++it)
  {
    if (it.value() == token)
      return idToName[it.key()];
  }
  return "";
}

QMap<QString, QString> ApiHandler::getProjects() { return idToName; }
