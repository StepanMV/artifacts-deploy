#include "api_reply.hpp"

ApiReply::ApiReply(QNetworkReply *qReply, QObject *parent) : QObject(parent), qReply(qReply)
{
    qReply->setParent(this);
    connect(qReply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError e)
            {
        if (e == QNetworkReply::OperationCanceledError) return;
        emit errorOccurred(e); });
}

ApiReply::~ApiReply()
{
    qDebug() << "Destructor: " << qReply->url() << "\n";
}
