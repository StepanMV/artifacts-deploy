#include "api_reply.hpp"

ApiReply::ApiReply(QNetworkReply *qReply, QObject *parent) : QObject(parent), qReply(qReply)
{
    qReply->setParent(this);
}

ApiReply::~ApiReply()
{
    qDebug() << "Destructor: " << qReply->url() << "\n";
}