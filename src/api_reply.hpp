#ifndef APIREPLY_HPP
#define APIREPLY_HPP
#include <QNetworkReply>

class ApiReply : public QObject
{
    Q_OBJECT
public:
    ApiReply(QNetworkReply *qReply, QObject *parent = nullptr);
    ~ApiReply();

    QNetworkReply * const qReply;

signals:
    void dataReady(QByteArray &data);
    void errorOccurred(QNetworkReply::NetworkError e);
    
    void branchesReady(QList<QString>& branches);
    void pipelinesReady(QMap<QString, QString>& pipelines);
    void jobsReady(QMap<QString, QString>& jobs);

    void pipelinesCheck();
};

#endif // APIREPLY_HPP