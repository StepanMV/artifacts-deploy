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
    void errorOccurred(const QNetworkReply::NetworkError &error);
    
    void branchesReady(QList<QString>& branches);
    void jobsReady(QList<QString>& jobs);

    void pipelinesCheck();
};

#endif // APIREPLY_HPP