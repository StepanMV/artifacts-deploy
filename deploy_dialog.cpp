#include "deploy_dialog.hpp"
#include "ui_deploy_dialog.h"
#include <QJsonObject>
#include "api_handler.hpp"
#include "data_manager.hpp"
#include <QJsonArray>
#include <QFileDialog>
#include <QLineEdit>

DeployDialog::DeployDialog(QWidget *parent)
    : CoolerDialog(parent)
    , ui(new Ui::DeployDialog)
{
    ui->setupUi(this);
    connect(ui->comboBoxProject, &QComboBox::textActivated, this, &DeployDialog::projectSelected);
    connect(ui->comboBoxBranch, &QComboBox::textActivated, this, &DeployDialog::branchSelected);
    connect(ui->comboBoxPipeline, &QComboBox::textActivated, this, &DeployDialog::pipelineSelected);
    connect(ui->comboBoxJob, &QComboBox::textActivated, this, &DeployDialog::jobSelected);
    connect(ui->comboBoxMachine, &QComboBox::textActivated, this, &DeployDialog::machineSelected);
    connect(ui->comboBoxDirectory, &QComboBox::currentTextChanged, this, &DeployDialog::pathEdited);
    connect(ui->checkBox, &QCheckBox::clicked, this, &DeployDialog::dirSaveChecked);
    connect(ui->radioButtonCurl, &QRadioButton::clicked, this, &DeployDialog::curlClicked);
    connect(ui->radioButtonSftp, &QRadioButton::clicked, this, &DeployDialog::sftpClicked);
    connect(ui->buttonSelectDir, &QPushButton::clicked, this, &DeployDialog::pathSelectClicked);
}

DeployDialog::~DeployDialog()
{
    delete ui;
}

void DeployDialog::updateComboBox(QComboBox* box, const QList<QString>& items)
{
    QString tempStr = box->currentText();
    box->clear();
    box->addItems(items);
    box->setEditText(tempStr);
}

bool DeployDialog::shouldUpdate(const QString& value, const QList<QString>& checkList, QLabel* label)
{
    if(value.isEmpty()) return false;
    for(auto it = checkList.begin(); it != checkList.end(); ++it)
    {
        if (*it == value)
        {
            label->setStyleSheet("color: green;");
            return true;
        }
    }
    label->setStyleSheet("color: red;");
    return false;
}

void DeployDialog::smartClean(const QString& key, QComboBox* box, QLabel* label)
{
    QString tempStr = box->currentText();
    temp.remove(key);
    box->clear();
    box->setEditText(tempStr);
    label->setStyleSheet("");
}

void DeployDialog::comboSetEnabled(bool enabled)
{
    ui->comboBoxProject->setEnabled(enabled);
    ui->comboBoxBranch->setEnabled(enabled);
    ui->comboBoxPipeline->setEnabled(enabled);
    ui->comboBoxJob->setEnabled(enabled);
}

void DeployDialog::init()
{
    ui->comboBoxProject->clear();
    ui->comboBoxProject->addItems(ApiHandler::getProjectNames());
    ui->comboBoxProject->clearEditText();
    ui->comboBoxMachine->clear();
    ui->comboBoxMachine->addItems(DataManager::getList("ssh", "name"));
    ui->comboBoxMachine->addItem("localhost");
    ui->comboBoxMachine->clearEditText();
    this->temp["mode"] = "curl";
    temp["createDir"] = false;
    machineSelected(ui->comboBoxMachine->currentText());
}

void DeployDialog::projectSelected(const QString& text)
{
    if(!shouldUpdate(text, ApiHandler::getProjectNames(), ui->labelProject)) return;
    temp["project"] = (int) ApiHandler::getProjectId(text);
    try
    {
        comboSetEnabled(false);
        this->branches = ApiHandler::getBranches(temp["project"].toInt());
        comboSetEnabled(true);
        smartClean("branch", ui->comboBoxBranch, ui->labelBranch);
        smartClean("pipeline", ui->comboBoxPipeline, ui->labelPipeline);
        smartClean("job", ui->comboBoxJob, ui->labelJob);
        updateComboBox(ui->comboBoxBranch, branches);
        this->pipelines.clear();
        branchSelected(ui->comboBoxBranch->currentText());
    } catch (const QNetworkReply::NetworkError e)
    {
        ui->labelProject->setStyleSheet("color: red;");
    }
}

void DeployDialog::branchSelected(const QString& text)
{
    if(!shouldUpdate(text, this->branches, ui->labelBranch)) return;
    temp["branch"] = text;
    try
    {
        comboSetEnabled(false);
        this->pipelines = ApiHandler::getPipelines(temp["project"].toInt(), text);
        comboSetEnabled(true);
        QList<QString> list = pipelines.values();
        std::reverse(list.begin(), list.end());
        smartClean("pipeline", ui->comboBoxPipeline, ui->labelPipeline);
        smartClean("job", ui->comboBoxJob, ui->labelJob);
        updateComboBox(ui->comboBoxPipeline, list);
        this->jobs.clear();
        pipelineSelected(ui->comboBoxPipeline->currentText());
    } catch (const QNetworkReply::NetworkError e)
    {
        ui->labelBranch->setStyleSheet("color: red;");
    }
}

void DeployDialog::pipelineSelected(const QString& text)
{
    if(!shouldUpdate(text, this->pipelines.values(), ui->labelPipeline)) return;
    temp["pipeline"] = 0.0;
    for (auto it = pipelines.begin(); it != pipelines.end(); ++it)
    {
        if (it.value() == text)
        {
            temp["pipeline"] = (double) it.key();
            break;
        }
    }
    try
    {
        comboSetEnabled(false);
        this->jobs = ApiHandler::getJobs(temp["project"].toInt(), temp["pipeline"].toDouble());
        comboSetEnabled(true);
        smartClean("job", ui->comboBoxJob, ui->labelJob);
        updateComboBox(ui->comboBoxJob, jobs.values());
        jobSelected(ui->comboBoxJob->currentText());
    } catch (const QNetworkReply::NetworkError e)
    {
        ui->labelPipeline->setStyleSheet("color: red;");
    }
}

void DeployDialog::jobSelected(const QString& text)
{
    if(!shouldUpdate(text, this->jobs.values(), ui->labelJob)) return;
    temp["job"] = 0.0;
    for (auto it = jobs.begin(); it != jobs.end(); ++it)
    {
        if (it.value() == text)
        {
            temp["job"] = (double) it.key();
            break;
        }
    }
}

void DeployDialog::machineSelected(const QString& text)
{
    temp["machine"] = text;
    ui->labelMachine->setStyleSheet("color: green;");
    ui->comboBoxDirectory->clear();
    if(temp["machine"].toString() == "localhost")
    {
        ui->comboBoxDirectory->setEnabled(false);
        ui->checkBox->setEnabled(false);
        ui->radioButtonCurl->setEnabled(false);
        ui->radioButtonSftp->setEnabled(false);
        ui->buttonSelectDir->setEnabled(true);
        return;
    }
    ui->comboBoxDirectory->setEnabled(true);
    ui->checkBox->setEnabled(true);
    ui->radioButtonCurl->setEnabled(true);
    ui->radioButtonSftp->setEnabled(true);
    ui->buttonSelectDir->setEnabled(false);
    this->ssh = DataManager::getConnection(text);
}

void DeployDialog::pathEdited(const QString& text)
{
    if(temp["machine"].toString() == "localhost") return;
    if(text.endsWith("/") and !directories.contains(text))
    {
        QString response = ssh.sendCommand("ls -d " + text.toStdString() + "*/");
        this->directories = response.split("/\n");
        this->directories.removeAll("");
        this->directories.append(text);
        qDebug() << directories << text;
        updateComboBox(ui->comboBoxDirectory, directories);
    }
    if(!shouldUpdate(text, this->directories, ui->labelDirectory)) return;
    temp["directory"] = text;
}

void DeployDialog::pathSelectClicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select directory"));
    if (path.isEmpty()) return;
    ui->comboBoxDirectory->addItem(path);
    ui->labelDirectory->setStyleSheet("color: green;");
    temp["directory"] = ui->comboBoxDirectory->currentText();
}

void DeployDialog::dirSaveChecked(bool state)
{
    if (state && !ui->comboBoxDirectory->currentText().isEmpty())
    {
        temp["directory"] = ui->comboBoxDirectory->currentText();
        temp["createDir"] = true;
        ui->labelDirectory->setStyleSheet("color: green;");
    }
    else if (!state)
    {
        temp["directory"] = "";
        temp["createDir"] = false;
        pathEdited(ui->comboBoxDirectory->currentText());
    }
}

void DeployDialog::curlClicked(bool state)
{
    temp["mode"] = "curl";
}

void DeployDialog::sftpClicked(bool state)
{
    temp["mode"] = "sftp";
}

QJsonObject DeployDialog::getData()
{
    temp["display"] = ApiHandler::getProjectName(temp["project"].toInt()) + "/" + temp["branch"].toString() + "/" + jobs.value(temp["job"].toDouble()) +
        " -> " + temp["machine"].toString() + " (" + temp["directory"].toString() + ")";
    return temp;
}

bool DeployDialog::verifyData()
{
    qDebug() << temp;
    return temp.contains("project") && temp.contains("branch") && temp.contains("pipeline") &&
           temp.contains("job") && temp.contains("machine") && temp.contains("directory");
}

void DeployDialog::clearFileds()
{
    ui->comboBoxProject->clearEditText();
    ui->labelProject->setStyleSheet("");
    ui->comboBoxMachine->clearEditText();
    ui->labelMachine->setStyleSheet("");
    ui->comboBoxBranch->clear();
    ui->labelBranch->setStyleSheet("");
    ui->comboBoxPipeline->clear();
    ui->labelPipeline->setStyleSheet("");
    ui->comboBoxJob->clear();
    ui->labelJob->setStyleSheet("");
    ui->comboBoxDirectory->clear();
    ui->labelDirectory->setStyleSheet("");
    ui->comboBoxDirectory->setEnabled(false);
    this->temp = QJsonObject();
    this->branches.clear();
    this->pipelines.clear();
    this->jobs.clear();
    this->temp["mode"] = "curl";
    temp["createDir"] = false;
    machineSelected(ui->comboBoxMachine->currentText());
}

void DeployDialog::setFields(const QJsonObject& data)
{
    ui->comboBoxProject->setCurrentText(ApiHandler::getProjectName(data["project"].toInt()));
    ui->comboBoxBranch->setCurrentText(data["branch"].toString());
    auto tempPipelnes = ApiHandler::getPipelines(data["project"].toInt(), data["branch"].toString());
    ui->comboBoxPipeline->setCurrentText(tempPipelnes.value(data["pipeline"].toDouble()));
    auto tempJobs = ApiHandler::getJobs(data["project"].toInt(), data["pipeline"].toDouble());
    ui->comboBoxJob->setCurrentText(tempJobs.value(data["job"].toDouble()));
    ui->comboBoxMachine->setCurrentText(data["machine"].toString());
    ui->comboBoxDirectory->setCurrentText(data["directory"].toString());
    ui->checkBox->setChecked(data["createDir"].toBool());
    if (data["mode"].toString() == "curl") ui->radioButtonCurl->setChecked(true);
    else if (data["mode"].toString() == "sftp") ui->radioButtonSftp->setChecked(true);
    projectSelected(ui->comboBoxProject->currentText());
    dirSaveChecked(data["createDir"].toBool());
    curlClicked(data["mode"].toString() == "curl");
}
