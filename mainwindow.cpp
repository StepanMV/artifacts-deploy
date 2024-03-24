#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "api_handler.hpp"
#include "cooler_list.hpp"
#include "token_dialog.hpp"
#include "data_manager.hpp"
#include "deploy_dialog.hpp"
#include "ssh_dialog.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //ApiHandler::setURL("https://vgit.mirea.ru/api/v4");
    //SSHConnection connection = SSHConnection("62.84.103.68", 22, "root", "/home/twinkboy42/id_ed25519", "");
    //qDebug() << connection.sendCommand("ls -d /*");
    //qDebug() << connection.sendCommand("ls");
    //qDebug() << ApiHandler::getRepositories({"glpat-fERYjqiti4yf-4Hpcau5"});
    //qDebug() << ApiHandler::getBranches(606);
    //qDebug() << ApiHandler::getPipelines(606, "main");
    //qDebug() << ApiHandler::getJobs(606, 6324);

    this->tokenList = new CoolerList("projects",
                                     {ui->buttonTokensAdd, ui->buttonTokensRemove, ui->buttonTokensDuplicate, ui->buttonTokensEdit},
                                     ui->listTokens, new TokenDialog());
    this->sshList = new CoolerList("ssh",
                                     {ui->buttonSshAdd, ui->buttonSshRemove, ui->buttonSshDuplicate, ui->buttonSshEdit},
                                     ui->listSsh, new SshDialog());
    this->deployList = new CoolerList("deploy",
                                   {ui->buttonDeployAdd, ui->buttonDeployRemove, ui->buttonDeployDuplicate, ui->buttonDeployEdit},
                                   ui->listDeploy, new DeployDialog());
    connect(ui->lineEditApi, &QLineEdit::editingFinished, this, &MainWindow::updateApiUrl);
    connect(ui->lineEditUserToken, &QLineEdit::editingFinished, this, &MainWindow::updateUserToken);
    connect(ui->buttonNext, &QPushButton::clicked, this, &MainWindow::nextPage);
    connect(ui->buttonBack, &QPushButton::clicked, this, &MainWindow::prevPage);
    connect(ui->buttonProceed, &QPushButton::clicked, this, &MainWindow::startDownload);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateApiUrl()
{
    try
    {
        ApiHandler::setURL(ui->lineEditApi->text());
        DataManager::setApiUrl(ui->lineEditApi->text());
        ui->labelApi->setStyleSheet("color: green;");
    }
    catch (const QNetworkReply::NetworkError& e)
    {
        ui->labelApi->setStyleSheet("color: red;");
    }
}

void MainWindow::updateUserToken()
{
    try
    {
        ApiHandler::checkToken(ui->lineEditUserToken->text());
        DataManager::setUserToken(ui->lineEditUserToken->text());
        ui->labelUserToken->setStyleSheet("color: green;");
    }
    catch (const QNetworkReply::NetworkError& e)
    {
        ui->labelUserToken->setStyleSheet("color: red;");
    }
}

void MainWindow::nextPage()
{
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() + 1);
    dynamic_cast<DeployDialog * const>(this->deployList->getDialog())->init();
}

void MainWindow::prevPage()
{
    ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex() - 1);
}

void MainWindow::startDownload()
{
    QJsonArray deploy = DataManager::getData().value("deploy").toArray();
    for(const QJsonValue& val : deploy)
    {
        QUrl url = ApiHandler::getArtifactsUrl(val["project"].toInteger(), val["job"].toInteger());
        QString machine = val["machine"].toString();
        QString fileName = "artifacts_" + QString::number(val["job"].toInteger()) + ".zip";
        QString downloadPath = val["directory"].toString() + "/" + fileName;
        QString mode = val["mode"].toString();

        if (machine == "localhost")
        {
            DeployObject* obj = new DeployObject(qnam, url, downloadPath);
            obj->startDownload();
            deployObjets.push_back(obj);
        }
        else if (mode == "sftp")
        {
            SSHConnection ssh = DataManager::getConnection(machine);
            DeployObject* obj = new DeployObject(qnam, url, fileName, ssh, downloadPath);
            obj->startDownload();
            deployObjets.push_back(obj);
        }
        else if (mode == "curl")
        {
            SSHConnection ssh = DataManager::getConnection(machine);
            ssh.sendCommand("curl --location --output " + downloadPath.toStdString() + " " + url.toString().toStdString());
        }
    }
}
