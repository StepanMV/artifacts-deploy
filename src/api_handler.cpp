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

ApiReply *ApiHandler::getJobs(const QString &projectID, const QString &branchName)
{
  QString url = apiURL + "/projects/" + projectID + "/jobs/?scope[]=success";

  ApiReply *reply = makeRequest(url, idToToken[projectID]);
  connect(reply, &ApiReply::dataReady, reply, [this, projectID, branchName, reply](QByteArray &data)
          {
    QList<QString> jobNames;
    QJsonArray array = QJsonDocument::fromJson(data).array();
    for (const QJsonValue &val : array) {
      QString name = val["name"].toString();
      QString branch = val["ref"].toString();
      if (branch == branchName && !jobNames.contains(name))
        jobNames.append(name);
    }
    emit reply->jobsReady(jobNames); });
  return reply;
}

ApiReply *ApiHandler::getArtifacts(const QString &projectID, const QString &branchName, const QString &jobName, const QString &path)
{
  QString url;
  if (path.isEmpty())
    url = apiURL + "/projects/" + projectID + "/jobs/artifacts/" + branchName + "/download?job=" + jobName;
  else 
    url = apiURL + "/projects/" + projectID + "/jobs/artifacts/" + branchName + "/raw/" + path + "?job=" + jobName;
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
  connect(reply->qReply, &QNetworkReply::finished, reply, [this, reply, authUrl]()
          {
    if (reply->qReply->error() != QNetworkReply::NoError)
    {
      emit reply->errorOccurred(reply->qReply->error());
      return;
    }
    qDebug() << "Constructor: " << reply->qReply->url() << "\n"; 
    QByteArray data = reply->qReply->readAll();
    if (data.isEmpty())
    {
      emit reply->errorOccurred(QNetworkReply::ContentNotFoundError);
      return;
    }
    cache[authUrl] = data;
    emit reply->dataReady(data); });
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
