#include "token_dialog.hpp"
#include "api_handler.hpp"
#include "ui_token_dialog.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QAbstractButton>

TokenDialog::TokenDialog(QWidget *parent)
    : CoolerDialog(parent), ui(new Ui::TokenDialog), api(this)
{
  ui->setupUi(this);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TokenDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &TokenDialog::reject);
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button)
          {
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
      this->apply(); });
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
  ApiReply *reply = api.addToken(ui->lineEditToken->text());
  connect(reply, &ApiReply::dataReady, reply, [this, reply, label = ui->labelToken](const QByteArray &data)
          {
    label->setStyleSheet("color: green;");
    this->verified = true;
    this->accept();
    reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [reply, label = ui->labelToken](QNetworkReply::NetworkError e)
          {
    label->setStyleSheet("color: red;");
    reply->deleteLater(); });
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
