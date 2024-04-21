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

ApiReply *ApiHandler::addToken(const QString &token)
{
  return ApiHandler::addTokenPaged(token, 0);
}

ApiReply *ApiHandler::addTokenPaged(const QString &token, size_t i)
{
  ApiReply *reply = makeRequest(apiURL + "/projects?simple=true&min_access_level=20" +
                                    "&order_by=last_activity_at&per_page=100&page=" +
                                    QString::number(i),
                                token);
  connect(reply, &ApiReply::dataReady, reply, [this, reply, token, i](QByteArray &data)
          {
    QJsonArray arr = QJsonDocument::fromJson(data).array();
    if (arr.isEmpty())
    {
      reply->deleteLater();
      return;
    }
    for (auto it = arr.constBegin(); it != arr.constEnd(); ++it) {
      QJsonObject val = it->toObject();
      QString id = QString::number((size_t) val.value("id").toDouble());
      QString name = val.value("name").toString();
      idToToken[id] = token;
      idToName[id] = name;
    }
    addTokenPaged(token, i + 1);
    if(i > 0) reply->deleteLater(); });
  return reply;
}

ApiReply *ApiHandler::getBranches(const QString &projectID)
{

  QString url = apiURL + "/projects/" + projectID + "/repository/branches";
  ApiReply *reply = makeRequest(url, idToToken[projectID]);
  qDebug() << "CONNECTING\n";
  connect(reply, &ApiReply::dataReady, reply, [this, projectID, reply](QByteArray &data)
          {
    QList<QString> branchNames;
    QJsonArray array = QJsonDocument::fromJson(data).array();
    for (const QJsonValue &val : array) {
      QString name = val["name"].toString();
      branchNames.append(name);
    }
    emit reply->branchesReady(branchNames); });
  return reply;
}

ApiReply *ApiHandler::getPipelines(const QString &projectID, const QString &branchName)
{
  QString pipelineUrl = apiURL + "/projects/" + projectID +
                        "/pipelines?ref=" + branchName + "&status=success";
  QString commitsUrl = apiURL + "/projects/" + projectID +
                       "/repository/commits/?ref_name=" + branchName;

  std::map<std::string, std::string> *pipelineIdToSha = new std::map<std::string, std::string>();
  std::map<std::string, std::string> *commitHashToDescription = new std::map<std::string, std::string>();

  ApiReply *shaReply = makeRequest(pipelineUrl, idToToken[projectID]);
  ApiReply *commitsReply = makeRequest(commitsUrl, idToToken[projectID]);
  qDebug("Constructed requests");

  connect(shaReply, &ApiReply::dataReady, shaReply, [this, pipelineIdToSha, shaReply](QByteArray &data)
          {
    QJsonArray pipelineArray = QJsonDocument::fromJson(data).array();
    for (const QJsonValue &val : pipelineArray) {
      std::string id = QString::number((size_t) val["id"].toDouble()).toStdString();
      pipelineIdToSha->emplace(id, val["sha"].toString().toStdString());
    }
    qDebug() << "Inflated SHA to size: " + QString::number(pipelineIdToSha->size());
    emit shaReply->pipelinesCheck(); });

  connect(commitsReply, &ApiReply::dataReady, shaReply, [this, commitHashToDescription, commitsReply, shaReply](QByteArray &data)
          {
    QJsonArray commitsArray = QJsonDocument::fromJson(data).array();
    for (const QJsonValue &val : commitsArray) {
      std::string hash = val["id"].toString().toStdString();
      commitHashToDescription->emplace(hash, val["title"].toString().toStdString());
    }
    qDebug() << "Inflated commits to size: " + QString::number(commitHashToDescription->size());
    emit shaReply->pipelinesCheck(); });

  connect(commitsReply, &ApiReply::errorOccurred, shaReply, [this, commitsReply, shaReply](QNetworkReply::NetworkError e)
          { emit shaReply->errorOccurred(e); });

  connect(shaReply, &ApiReply::pipelinesCheck, shaReply, [this, pipelineIdToSha, commitHashToDescription, shaReply, commitsReply]()
          { 
    if (!commitsReply->qReply->isFinished() || !shaReply->qReply->isFinished()) return;
    QMap<QString, QString> pipelineIdToCommit;
    for (auto it = pipelineIdToSha->begin(); it != pipelineIdToSha->end(); ++it) {
      QString key = QString::fromStdString(it->first);
      QString value = QString::fromStdString(commitHashToDescription->at(it->second));
      pipelineIdToCommit.insert(key, "#" + key + " " + value);
    }
    emit shaReply->pipelinesReady(pipelineIdToCommit);
    delete commitHashToDescription;
    delete pipelineIdToSha;
    commitsReply->deleteLater(); });
  return shaReply;
}

ApiReply *ApiHandler::getJobs(const QString &projectID, const QString &pipelineID)
{
  QString url = apiURL + "/projects/" + projectID + "/pipelines/" + pipelineID + "/jobs?scope[]=success";

  ApiReply *reply = makeRequest(url, idToToken[projectID]);
  connect(reply, &ApiReply::dataReady, reply, [this, projectID, reply](QByteArray &data)
          {
    QMap<QString, QString> jobIdToName;
    QJsonArray array = QJsonDocument::fromJson(data).array();
    for (auto it = array.begin(); it != array.end(); ++it) {
      QJsonObject val = it->toObject();
      QString id = QString::number((size_t) val.value("id").toDouble());
      jobIdToName[id] = val.value("name").toString();
    }
    emit reply->jobsReady(jobIdToName); });
  return reply;
}

ApiReply *ApiHandler::getArtifacts(const QString &projectID, const QString &jobID, const QString &path)
{
  QString url = apiURL + "/projects/" + projectID + "/jobs/" + jobID + "/artifacts";
  if (!path.isEmpty())
    url += "/" + path;
  ApiReply *reply = makeRequest(url, idToToken[projectID]);
  reply->qReply->disconnect();
  return reply;
}

ApiHandler::ApiHandler(QObject *parent)
    : QObject{parent} {}

ApiReply *ApiHandler::checkURL(const QString &url)
{
  ApiReply *reply = makeRequest(url + "/projects?per_page=1");
  return reply;
}

ApiReply *ApiHandler::makeRequest(const QString &url, const QString &token)
{
  QString authUrl = url + (url.contains('?') ? "&" : "?") + "private_token=" + token;
  QNetworkRequest request = QNetworkRequest(QUrl(authUrl));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

  ApiReply *reply = new ApiReply(qnam.get(request));
  if (cache.contains(authUrl))
  {
    QTimer::singleShot(100, reply, [this, reply, authUrl]()
                       {
      reply->qReply->abort();
      emit reply->dataReady(cache[authUrl]); });
    return reply;
  }
  replies.append(reply);
  connect(reply->qReply, &QNetworkReply::finished, reply, [this, reply, authUrl]()
          {
    if (reply->qReply->error() != QNetworkReply::NoError)
    {
      return;
    }
    QByteArray data = reply->qReply->readAll();
    replies.removeOne(reply);
    qDebug() << "Constructor: " << reply->qReply->url() << "\n"; 
    cache[authUrl] = data;
    emit reply->dataReady(data);
    if (replies.isEmpty())
      emit allRepliesFinished(); });
  return reply;
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
