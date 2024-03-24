#include "token_dialog.hpp"
#include "ui_token_dialog.h"
#include <QDebug>
#include "api_handler.hpp"
#include <QJsonObject>
#include <QJsonArray>

TokenDialog::TokenDialog(QWidget *parent)
    : CoolerDialog(parent)
    , ui(new Ui::TokenDialog)
{
    ui->setupUi(this);
}

TokenDialog::~TokenDialog()
{
    delete ui;
}

QJsonObject TokenDialog::getData()
{
    QJsonObject temp;
    temp["token"] = ui->lineEditToken->text();
    temp["display"] = ApiHandler::getProjectName(temp["token"].toString()) + " (" + temp["token"].toString() + ")";
    return temp;
}

bool TokenDialog::verifyData()
{
    try
    {
        ApiHandler::checkToken(ui->lineEditToken->text());
    }
    catch (const QNetworkReply::NetworkError& e)
    {
        ui->labelToken->setStyleSheet("color: red;");
        return false;
    }
    ui->labelToken->setStyleSheet("color: green;");
    return true;
}

void TokenDialog::clearFileds()
{
    ui->lineEditToken->clear();
    ui->labelToken->setStyleSheet("");
}

void TokenDialog::setFields(const QJsonObject& data)
{
    ui->lineEditToken->setText(data["token"].toString());
}
