#include "token_dialog.hpp"
#include "api_handler.hpp"
#include "ui_token_dialog.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

TokenDialog::TokenDialog(QWidget *parent)
    : CoolerDialog(parent), ui(new Ui::TokenDialog), api(this)
{
  ui->setupUi(this);
}

TokenDialog::~TokenDialog() { delete ui; }

QJsonObject TokenDialog::getData()
{
  QJsonObject temp;
  temp["token"] = ui->lineEditToken->text();
  temp["display"] = ApiHandler::getProjectName(temp["token"].toString()) +
                    " (" + temp["token"].toString() + ")";
  return temp;
}

bool TokenDialog::verifyData()
{
  if (verified)
    return true;
  auto future = api.addToken(ui->lineEditToken->text());
  future
      .then([this, label = ui->labelToken]()
            {
        label->setStyleSheet("color: green;");
        this->verified = true;
        this->accept(); })
      .onFailed([label = ui->labelToken](QNetworkReply::NetworkError e)
                { label->setStyleSheet("color: red;"); });
  return false;
}

void TokenDialog::clearFileds()
{
  ui->lineEditToken->clear();
  ui->labelToken->setStyleSheet("");
  verified = false;
}

void TokenDialog::setFields(const QJsonObject &data)
{
  ui->lineEditToken->setText(data["token"].toString());
}
