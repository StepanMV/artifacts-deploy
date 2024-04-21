#include "ssh_dialog.hpp"
#include "ui/ui_ssh_dialog.h"
#include <QNetworkReply>
#include "ssh_connection.hpp"
#include <QFileDialog>

SshDialog::SshDialog(QWidget *parent)
    : ComplexDialog(parent), ui(new Ui::SshDialog)
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

bool SshDialog::verifyData(const QJsonObject &data, bool visual)
{
    if (data["ip"].toString().isEmpty())
    {
        if (visual)
            ui->labelIp->setStyleSheet("color: red;");
        return false;
    }
    else if (visual)
        ui->labelIp->setStyleSheet("color: green;");
    if (data["port"].toInt() == 0)
    {
        if (visual)
            ui->labelPort->setStyleSheet("color: red;");
        return false;
    }
    else if (visual)
        ui->labelPort->setStyleSheet("color: green;");
    if (data["user"].toString().isEmpty())
    {
        if (visual)
            ui->labelUser->setStyleSheet("color: red;");
        return false;
    }
    else if (visual)
        ui->labelUser->setStyleSheet("color: green;");
    if (data["keyPath"].toString().isEmpty() && data["password"].toString().isEmpty())
    {
        if (visual)
        {
            ui->labelKeyPath->setStyleSheet("color: red;");
            ui->labelKeyPass->setStyleSheet("color: red;");
            ui->labelPassword->setStyleSheet("color: red;");
        }
        return false;
    }
    else if (visual)
    {
        ui->labelKeyPath->setStyleSheet("color: green;");
        ui->labelKeyPass->setStyleSheet("color: green;");
        ui->labelPassword->setStyleSheet("color: green;");
    }
    try
    {
        if (data["password"].toString().isEmpty())
        {
            SSHConnection(data["ip"].toString().toStdString(), data["port"].toInt(), data["user"].toString().toStdString(),
                          data["keyPath"].toString().toStdString(), data["keyPass"].toString().toStdString());
        }
        else
        {
            SSHConnection(data["ip"].toString().toStdString(), data["port"].toInt(), data["user"].toString().toStdString(),
                          data["password"].toString().toStdString());
        }
    }
    catch (const AllocationError &e)
    {
        return false;
    }
    catch (const ConnectionError &e)
    {
        if (visual)
        {
            ui->labelIp->setStyleSheet("color: red;");
            ui->labelPort->setStyleSheet("color: red;");
            ui->labelUser->setStyleSheet("color: red;");
        }
        return false;
    }
    catch (const VerificationError &e)
    {
        if (visual)
            ui->labelIp->setStyleSheet("color: red;");
        return false;
    }
    catch (const AuthenticationError &e)
    {
        if (visual)
        {
            ui->labelKeyPath->setStyleSheet("color: red;");
            ui->labelKeyPass->setStyleSheet("color: red;");
            ui->labelPassword->setStyleSheet("color: red;");
        }
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

void SshDialog::show(const QJsonObject &data)
{
    ui->lineEditIp->setText(data["ip"].toString());
    ui->lineEditUser->setText(data["user"].toString());
    ui->lineEditKeyPath->setText(data["keyPath"].toString());
    ui->lineEditKeyPass->setText(data["keyPass"].toString());
    ui->lineEditPassword->setText(data["password"].toString());
    QDialog::show();
}
