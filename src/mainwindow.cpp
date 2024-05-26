#include "mainwindow.hpp"
#include "api_handler.hpp"
#include "data_manager.hpp"
#include "deploy_dialog.hpp"
#include "ssh_dialog.hpp"
#include "token_dialog.hpp"
#include "ui/ui_mainwindow.h"
#include <QFileDialog>
#include <QUiLoader>
#include <QJsonArray>
#include "mylogger.hpp"
#include <filesystem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), api(this)
{
  ui->setupUi(this);
  this->tokenList = new ComplexList("token", ui->tokenList, new TokenDialog(this), this);
  this->sshList = new ComplexList("ssh", ui->sshList, new SshDialog(this), this);
  this->deployList = new DeployList(ui->deployList, new DeployDialog(this), this);
  connect(ui->apiLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateApiUrl);
  connect(ui->userTokenLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateUserToken);
  connect(ui->settingsButton, &QPushButton::clicked, this, [this]()
          { ui->stackedWidget->setCurrentIndex(1); });
  connect(ui->settingsBackButton, &QPushButton::clicked, this, [this]()
          { ui->stackedWidget->setCurrentIndex(0); });
  connect(ui->loadButton, &QPushButton::clicked, this, &MainWindow::importClicked);
  connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::exportClicked);
  connect(this, &MainWindow::apiUpdated, this, &MainWindow::updateUserToken);
  connect(ui->apiButtonUpdate, &QPushButton::clicked, this, &MainWindow::updateApiUrl);
  connect(ui->userTokenButtonUpdate, &QPushButton::clicked, this, &MainWindow::updateUserToken);
  import("data.json");
  std::filesystem::create_directory("logs");
  MyLogger::setLogPath("logs/log_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".txt");
}

MainWindow::~MainWindow()
{
  DataManager::dumpData("data.json");
  delete ui;
}

void MainWindow::updateApiUrl()
{
  QString newUrl = ui->apiLineEdit->text();
  ApiReply *reply = api.checkURL(newUrl);
  connect(reply, &ApiReply::dataReady, reply, [this, newUrl, label = ui->apiLabel, reply](const QByteArray &data)
          {
    ApiHandler::setURL(newUrl);
    DataManager::setApiUrl(newUrl);
    label->setStyleSheet("color: green;");
    reply->deleteLater();
    emit apiUpdated(); });
  connect(reply, &ApiReply::errorOccurred, reply, [labelApi = ui->apiLabel, labelToken = ui->userTokenLabel, reply](QNetworkReply::NetworkError e)
          {
    labelApi->setStyleSheet("color: red;");
    labelToken->setStyleSheet("");
    reply->deleteLater(); });
}

void MainWindow::updateUserToken()
{
  ui->userTokenLabel->setStyleSheet("");
  QString token = ui->userTokenLineEdit->text();
  ApiReply *reply = api.addToken(token);
  connect(reply, &ApiReply::dataReady, reply, [this, token, label = ui->userTokenLabel, reply](const QByteArray &data)
          {
    DataManager::setUserToken(token);
    label->setStyleSheet("color: green;");
    reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [label = ui->userTokenLabel, reply](QNetworkReply::NetworkError e)
          {
    label->setStyleSheet("color: red;");
    reply->deleteLater(); });
}

void MainWindow::importClicked()
{
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open File"), QDir::homePath(), tr("JSON (*.json)"));
  if (fileName.isEmpty())
    return;
  import(fileName);
}

void MainWindow::exportClicked()
{
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save File"), QDir::homePath(), tr("JSON (*.json)"));
  if (fileName.isEmpty())
    return;
  DataManager::dumpData(fileName);
}

void MainWindow::import(const QString &filename)
{
  if (!QFile::exists(filename))
    return;
  tokenList->clear();
  sshList->clear();
  deployList->clear();
  DataManager::loadData(filename);
  ui->apiLineEdit->setText(DataManager::getData().value("apiUrl").toString());
  ui->userTokenLineEdit->setText(DataManager::getData().value("userToken").toString());
  emit ui->apiLineEdit->editingFinished();

  for (const QJsonValue &val : DataManager::getData().value("token").toArray())
  {
    ApiReply *reply = api.addToken(val.toObject().value("token").toString());
    connect(reply, &ApiReply::dataReady, reply, [this, reply](const QByteArray &data)
            { reply->deleteLater(); });
  }
  tokenList->init(DataManager::getData().value("token").toArray());
  sshList->init(DataManager::getData().value("ssh").toArray());
  deployList->init(DataManager::getData().value("deploy").toArray());
}
