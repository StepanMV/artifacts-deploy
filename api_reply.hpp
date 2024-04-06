#ifndef APIREPLY_HPP
#define APIREPLY_HPP
#include <QNetworkReply>

class ApiReply : public QObject
{
    Q_OBJECT
public:
    ApiReply(QNetworkReply *qReply);
    ~ApiReply();

    QNetworkReply * const qReply;

signals:
    void dataReady(const QByteArray &data);
    void errorOccurred(QNetworkReply::NetworkError e);
    
    void branchesReady(const QList<QString>& branches);
    void pipelinesReady(const QMap<QString, QString>& pipelines);
    void jobsReady(const QMap<QString, QString>& jobs);

    void pipelinesCheck();
};

#endif // APIREPLY_HPP