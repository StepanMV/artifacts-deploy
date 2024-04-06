#include "mainwindow.hpp"
#include "api_handler.hpp"
#include "cooler_list.hpp"
#include "data_manager.hpp"
#include "deploy_dialog.hpp"
#include "ssh_dialog.hpp"
#include "token_dialog.hpp"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), api(this)
{
  ui->setupUi(this);
  // ApiHandler::setURL("https://vgit.mirea.ru/api/v4");
  // SSHConnection connection = SSHConnection("", 22, "root",
  // "/home/twinkboy42/id_ed25519", ""); qDebug() << connection.sendCommand("ls
  // -d /*"); qDebug() << connection.sendCommand("ls"); qDebug() <<
  // ApiHandler::getRepositories({""}); qDebug() <<
  // ApiHandler::getBranches(606); qDebug() << ApiHandler::getPipelines(606,
  // "main"); qDebug() << ApiHandler::getJobs(606, 6324);

  this->tokenList = new CoolerList("projects",
                                   {ui->buttonTokensAdd, ui->buttonTokensRemove,
                                    ui->buttonTokensDuplicate, ui->buttonTokensEdit},
                                   ui->listTokens, new TokenDialog(this), this);
  this->sshList = new CoolerList("ssh",
                                 {ui->buttonSshAdd, ui->buttonSshRemove,
                                  ui->buttonSshDuplicate, ui->buttonSshEdit},
                                 ui->listSsh, new SshDialog(this), this);
  this->deployList = new CoolerList("deploy",
                                    {ui->buttonDeployAdd, ui->buttonDeployRemove,
                                     ui->buttonDeployDuplicate, ui->buttonDeployEdit},
                                    ui->listDeploy, new DeployDialog(this), this);
  connect(ui->lineEditApi, &QLineEdit::editingFinished, this, &MainWindow::updateApiUrl);
  connect(ui->lineEditUserToken, &QLineEdit::editingFinished, this, &MainWindow::updateUserToken);
  connect(ui->buttonNext, &QPushButton::clicked, this, &MainWindow::nextPage);
  connect(ui->buttonBack, &QPushButton::clicked, this, &MainWindow::prevPage);
  connect(ui->buttonProceed, &QPushButton::clicked, this, &MainWindow::proceedClicked);
  connect(ui->buttonImport, &QPushButton::clicked, this, &MainWindow::importClicked);
  connect(ui->buttonExport, &QPushButton::clicked, this, &MainWindow::exportClicked);
  connect(this, &MainWindow::apiUpdated, this, &MainWindow::updateUserToken);
}

MainWindow::~MainWindow()
{
  delete ui;
  delete tokenList;
  delete sshList;
  delete deployList;
}

void MainWindow::updateApiUrl()
{
  QString newUrl = ui->lineEditApi->text();
  ApiReply *reply = api.checkURL(newUrl);
  connect(reply, &ApiReply::dataReady, reply, [this, newUrl, label = ui->labelApi, reply](const QByteArray &data)
          {
    ApiHandler::setURL(newUrl);
    DataManager::setApiUrl(newUrl);
    label->setStyleSheet("color: green;");
    reply->deleteLater();
    emit apiUpdated(); });
  connect(reply, &ApiReply::errorOccurred, reply, [label = ui->labelApi, reply](QNetworkReply::NetworkError e)
          {
    label->setStyleSheet("color: red;");
    reply->deleteLater(); });
}

void MainWindow::updateUserToken()
{
  ui->labelUserToken->setStyleSheet("");
  QString token = ui->lineEditUserToken->text();
  ApiReply *reply = api.addToken(token);
  connect(reply, &ApiReply::dataReady, reply, [token, label = ui->labelUserToken, reply](const QByteArray &data)
          {
    DataManager::setUserToken(token);
    label->setStyleSheet("color: green;");
    reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [label = ui->labelUserToken, reply](QNetworkReply::NetworkError e)
          {
    label->setStyleSheet("color: red;");
    reply->deleteLater(); });
}

void MainWindow::nextPage()
{
  ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
  dynamic_cast<DeployDialog *const>(this->deployList->getDialog())->init();
}

void MainWindow::prevPage()
{
  ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);
}

void MainWindow::proceedClicked()
{
  QJsonArray deploy = DataManager::getData().value("deploy").toArray();
  size_t i = 0;
  for (const QJsonValue &val : deploy)
  {
    ApiReply *reply = api.getArtifacts(val["project"].toString(), val["job"].toString(), val["path"].toString());
    QString directory = val["directory"].toString();
    QString cachePath = val["cachePath"].toString();
    QListWidgetItem *item = ui->listDeploy->item(i);
    item->setText(val["display"].toString());

    DeployObject *obj;
    if (val["machine"].toString() == "localhost")
    {
      obj = new DeployObject(reply, directory, cachePath, item, this);
    }
    else
    {
      try
      {
      SSHConnection *ssh = DataManager::getConnection(val["machine"].toString());
      obj = new DeployObject(reply, directory, cachePath, item, ssh, this);
      }
      catch (const SshError &e)
      {
        item->setText("[ERROR] " + item->text());
        continue;
      }
    }
    connect(obj, &DeployObject::done, obj, [obj]()
            { obj->deleteLater(); });
    i++;
  }
}

void MainWindow::importClicked()
{
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open File"), QDir::homePath(), tr("JSON (*.json)"));
  if (fileName.isEmpty())
    return;
  DataManager::loadData(fileName);
  ui->listTokens->clear();
  ui->listSsh->clear();
  ui->listDeploy->clear();
  ui->lineEditApi->setText(DataManager::getData().value("apiUrl").toString());
  ui->lineEditUserToken->setText(DataManager::getData().value("userToken").toString());
  emit ui->lineEditApi->editingFinished();

  for (const QJsonValue &val : DataManager::getData().value("projects").toArray())
  {
    ApiReply *reply = api.addToken(val.toObject().value("token").toString());
    connect(reply, &ApiReply::dataReady, reply, [this, reply](const QByteArray &data)
            { reply->deleteLater(); });
    ui->listTokens->addItem(val.toObject().value("display").toString());
  }

  ui->listSsh->addItems(DataManager::getList("ssh", "display"));
  ui->listDeploy->addItems(DataManager::getList("deploy", "display"));
}

void MainWindow::exportClicked()
{
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save File"), QDir::homePath(), tr("JSON (*.json)"));
  if (fileName.isEmpty())
    return;
  DataManager::dumpData(fileName);
}
