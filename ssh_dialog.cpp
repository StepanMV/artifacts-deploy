#include "ssh_dialog.hpp"
#include "ui_ssh_dialog.h"
#include <QNetworkReply>
#include "ssh_connection.hpp"
#include <QFileDialog>

SshDialog::SshDialog(QWidget *parent)
    : CoolerDialog(parent)
    , ui(new Ui::SshDialog)
{
    ui->setupUi(this);
    connect(ui->buttonKeyDir, &QPushButton::clicked, this, &SshDialog::selectKeyDir);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SshDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SshDialog::reject);
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button)
          {
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
      this->apply(); });
}

SshDialog::~SshDialog()
{
    delete ui;
}

void SshDialog::selectKeyDir()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select private key file"));
    ui->lineEditKeyPath->setText(path);
}

QJsonObject SshDialog::getData()
{
    QJsonObject temp;
    temp["ip"] = ui->lineEditIp->text();
    temp["port"] = ui->lineEditPort->text().toInt();
    temp["user"] = ui->lineEditUser->text();
    temp["name"] = temp["user"].toString() + "@" + temp["ip"].toString();
    temp["keyPath"] = ui->lineEditKeyPath->text();
    temp["keyPass"] = ui->lineEditKeyPass->text();
    temp["password"] = ui->lineEditPassword->text();
    temp["display"] = temp["name"].toString();
    return temp;
}

bool SshDialog::verifyData()
{
    if(ui->lineEditIp->text().isEmpty())
    {
        ui->labelIp->setStyleSheet("color: red;");
        return false;
    }
    else ui->labelIp->setStyleSheet("color: green;");
    if(ui->lineEditPort->text().isEmpty())
    {
        ui->labelPort->setStyleSheet("color: red;");
        return false;
    }
    else ui->labelPort->setStyleSheet("color: green;");
    if(ui->lineEditUser->text().isEmpty())
    {
        ui->labelUser->setStyleSheet("color: red;");
        return false;
    }
    else ui->labelUser->setStyleSheet("color: green;");
    if(ui->lineEditKeyPath->text().isEmpty() && ui->lineEditPassword->text().isEmpty())
    {
        ui->labelKeyPath->setStyleSheet("color: red;");
        ui->labelKeyPass->setStyleSheet("color: red;");
        ui->labelPassword->setStyleSheet("color: red;");
        return false;
    }
    else
    {
        ui->labelKeyPath->setStyleSheet("color: green;");
        ui->labelKeyPass->setStyleSheet("color: green;");
        ui->labelPassword->setStyleSheet("color: green;");
    }
    try
    {
        if (ui->lineEditPassword->text().isEmpty())
        {
            SSHConnection(ui->lineEditIp->text().toStdString(), ui->lineEditPort->text().toInt(), ui->lineEditUser->text().toStdString(),
                          ui->lineEditKeyPath->text().toStdString(), ui->lineEditKeyPass->text().toStdString()).connect();
        }
        else
        {
            SSHConnection(ui->lineEditIp->text().toStdString(), ui->lineEditPort->text().toInt(), ui->lineEditUser->text().toStdString(),
                          ui->lineEditPassword->text().toStdString()).connect();
        }
    }
    catch (const AllocationError& e)
    {
        return false;
    }
    catch (const ConnectionError& e)
    {
        ui->labelIp->setStyleSheet("color: red;");
        ui->labelPort->setStyleSheet("color: red;");
        ui->labelUser->setStyleSheet("color: red;");
        return false;
    }
    catch (const VerificationError& e)
    {
        ui->labelIp->setStyleSheet("color: red;");
        return false;
    }
    catch (const AuthenticationError& e)
    {
        ui->labelKeyPath->setStyleSheet("color: red;");
        ui->labelKeyPass->setStyleSheet("color: red;");
        ui->labelPassword->setStyleSheet("color: red;");
        return false;
    }
    return true;
}

void SshDialog::clearFileds()
{
    ui->lineEditIp->clear();
    ui->lineEditUser->clear();
    ui->lineEditKeyPath->clear();
    ui->lineEditKeyPass->clear();
    ui->lineEditPassword->clear();
    ui->labelIp->setStyleSheet("");
    ui->labelPort->setStyleSheet("");
    ui->labelUser->setStyleSheet("");
    ui->labelKeyPath->setStyleSheet("");
    ui->labelKeyPass->setStyleSheet("");
    ui->labelPassword->setStyleSheet("");
}

void SshDialog::setFields(const QJsonObject& data)
{
    ui->lineEditIp->setText(data["ip"].toString());
    ui->lineEditUser->setText(data["user"].toString());
    ui->lineEditKeyPath->setText(data["keyPath"].toString());
    ui->lineEditKeyPass->setText(data["keyPass"].toString());
    ui->lineEditPassword->setText(data["password"].toString());
}
