#include "deploy_dialog.hpp"
#include "api_handler.hpp"
#include "cooler_combobox.hpp"
#include "data_manager.hpp"
#include "ui_deploy_dialog.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>

DeployDialog::DeployDialog(QWidget *parent)
    : CoolerDialog(parent), ui(new Ui::DeployDialog), api(this)
{
  ui->setupUi(this);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DeployDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DeployDialog::reject);
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button)
          {
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
      this->apply(); });
  projectCCB = new CoolerComboBox(ui->labelProject, ui->comboBoxProject, this);
  branchCCB = new CoolerComboBox(ui->labelBranch, ui->comboBoxBranch, this);
  pipelineCCB = new CoolerComboBox(ui->labelPipeline, ui->comboBoxPipeline, this);
  jobCCB = new CoolerComboBox(ui->labelJob, ui->comboBoxJob, this);
  machineCCB = new CoolerComboBox(ui->labelMachine, ui->comboBoxMachine, this);
  directoryCCB = new CoolerComboBox(ui->labelDirectory, ui->comboBoxDirectory, this, true);
  connect(projectCCB, &CoolerComboBox::updated, this, &DeployDialog::projectSelected);
  connect(branchCCB, &CoolerComboBox::updated, this, &DeployDialog::branchSelected);
  connect(pipelineCCB, &CoolerComboBox::updated, this, &DeployDialog::pipelineSelected);
  connect(jobCCB, &CoolerComboBox::updated, this, &DeployDialog::jobSelected);
  connect(machineCCB, &CoolerComboBox::updated, this, &DeployDialog::machineSelected);
  connect(directoryCCB, &CoolerComboBox::updated, this, &DeployDialog::directorySelected);
  connect(ui->buttonSelectDir, &QPushButton::clicked, this, &DeployDialog::pathSelectClicked);
  connect(ui->checkBox, &QCheckBox::stateChanged, this, &DeployDialog::dirSaveChecked);
}

DeployDialog::~DeployDialog()
{
  delete ui;
  if (ssh)
    delete ssh;
  delete projectCCB;
  delete branchCCB;
  delete pipelineCCB;
  delete jobCCB;
  delete machineCCB;
  delete directoryCCB;
}

void DeployDialog::comboSetEnabled(bool enabled)
{
  ui->comboBoxProject->setEnabled(enabled);
  ui->comboBoxBranch->setEnabled(enabled);
  ui->comboBoxPipeline->setEnabled(enabled);
  ui->comboBoxJob->setEnabled(enabled);
}

void DeployDialog::localhostSetEnabled(bool enabled)
{
  ui->comboBoxDirectory->setEnabled(!enabled);
  ui->checkBox->setEnabled(!enabled);
  ui->buttonSelectDir->setEnabled(enabled);
}

void DeployDialog::init()
{
  // doesn't trigger shouldUpdate()
  projectCCB->updateItems(ApiHandler::getProjects(), false);
  machineCCB->updateItems(DataManager::getList("ssh", "name") +
                              QList<QString>({"localhost"}),
                          false);
  // directoryCCB->updateItems(QList<QString>({"/"}), false);
}

void DeployDialog::projectSelected(const QString &newText)
{
  comboSetEnabled(false);
  branchCCB->clearItems();
  pipelineCCB->clearItems();
  jobCCB->clearItems();
  ApiReply *reply = api.getBranches(projectCCB->getSelectedItemId());
  connect(reply, &ApiReply::branchesReady, reply, [this, reply](const QList<QString> &branches)
          {
            comboSetEnabled(true);
            branchCCB->updateItems(branches); // triggers shouldUpdate()
            reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError e)
          {
    projectCCB->onError();
    comboSetEnabled(true);
    reply->deleteLater(); });
}

void DeployDialog::branchSelected(const QString &newText)
{
  comboSetEnabled(false);
  pipelineCCB->clearItems();
  jobCCB->clearItems();
  ApiReply *reply = api.getPipelines(projectCCB->getSelectedItemId(), branchCCB->getSelectedItem());
  connect(reply, &ApiReply::pipelinesReady, reply, [this, reply](const QMap<QString, QString> &pipelines)
          {
            comboSetEnabled(true);
            pipelineCCB->updateItems(pipelines); // triggers shouldUpdate()
            reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError e)
          {
    branchCCB->onError();
    comboSetEnabled(true);
    reply->deleteLater(); });
}

void DeployDialog::pipelineSelected(const QString &newText)
{
  comboSetEnabled(false);
  jobCCB->clearItems();
  ApiReply *reply = api.getJobs(projectCCB->getSelectedItemId(), pipelineCCB->getSelectedItemId());
  connect(reply, &ApiReply::jobsReady, reply, [this, reply](const QMap<QString, QString> &jobs)
          {
            comboSetEnabled(true);
            jobCCB->updateItems(jobs); // triggers shouldUpdate()
            reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError e)
          {
    pipelineCCB->onError();
    comboSetEnabled(true);
    reply->deleteLater(); });
}

void DeployDialog::jobSelected(const QString &newText) {}

void DeployDialog::machineSelected(const QString &newText)
{
  directoryCCB->clearItems();
  if (newText == "localhost")
  {
    localhostSetEnabled(true);
    directoryCCB->clear();
  }
  else
  {
    localhostSetEnabled(false);
    try
    {
      this->ssh = DataManager::getConnection(newText);
    }
    catch (const SshError &e)
    {
      machineCCB->onError();
    }
  }
}

// should be called on every change..?
void DeployDialog::directorySelected(const QString &newText)
{
  if (machineCCB->getSelectedItem() == "localhost")
    return;
  if (!newText.endsWith("/") || directoryCCB->getSelectedItem() == newText)
    return;

  // if ends with a slash -> check response with this exact directory and set
  QString responseMain;
  try
  {
    responseMain = ssh->sendCommand("ls -d " + newText.toStdString());
  }
  catch (const SshError &e)
  {
    directoryCCB->onError();
    return;
  }
  
  // failure (selected=none, color=red), return
  if (!responseMain.split("\n").value(0).startsWith("/"))
  {
    directoryCCB->onError();
    return;
  }
  QString responseExtra;
  try
  {
    responseExtra = ssh->sendCommand("ls -d " + newText.toStdString() + "*/ " + newText.toStdString() + ".*/");
  }
  catch (const SshError &e)
  {
    directoryCCB->onError();
    return;
  }
  QList<QString> items = responseMain.split("\n") + responseExtra.split("\n");
  for (size_t i = 0; i < items.size(); i++)
    if (!items[i].startsWith("/") || items[i] == "")
      items.removeAt(i--);
  // update without triggering shouldUpdate()
  directoryCCB->updateItems(items, false);
  directoryCCB->setSelectedItem(newText);
}

void DeployDialog::pathSelectClicked()
{
  QString path = QFileDialog::getExistingDirectory(this, tr("Select directory"));
  if (path.isEmpty())
    return;
  directoryCCB->setSelectedItem(path);
}

void DeployDialog::dirSaveChecked(bool state)
{
  QString currentText = directoryCCB->getCurrentText();
  if (state && !currentText.isEmpty())
    directoryCCB->setSelectedItem(currentText); // doesn't trigger shouldUpdate()
  else if (!state)
  {
    directoryCCB->clear();
    ui->comboBoxDirectory->setEditText(currentText); // triggers shouldUpdate()
  }
}

QJsonObject DeployDialog::getData()
{
  QJsonObject temp;
  temp["project"] = projectCCB->getSelectedItemId();
  temp["projectName"] = projectCCB->getSelectedItem();
  temp["branch"] = branchCCB->getSelectedItem();
  temp["pipeline"] = pipelineCCB->getSelectedItemId();
  temp["pipelineName"] = pipelineCCB->getSelectedItem();
  temp["job"] = jobCCB->getSelectedItemId();
  temp["jobName"] = jobCCB->getSelectedItem();
  temp["path"] = ui->lineEdit->text();
  temp["machine"] = machineCCB->getSelectedItem();
  temp["directory"] = directoryCCB->getSelectedItem();
  QString tempStr = temp["directory"].toString();
  temp["directory"] = tempStr.endsWith("/") ? tempStr.left(tempStr.length() - 1) : tempStr;
  temp["createDir"] = ui->checkBox->isChecked();
  temp["cachePath"] = "cache/" + temp["project"].toString() + "_" + temp["job"].toString() + "/" +
                      (temp["path"].toString().isEmpty() ? "artifacts.zip" : temp["path"].toString());
  temp["display"] = temp["projectName"].toString() + "/" + temp["branch"].toString() +
                    "/" + temp["jobName"].toString() + " (" +
                    (temp["path"].toString().isEmpty() ? "*" : temp["path"].toString()) +
                    ") -> " + temp["machine"].toString() +
                    "(" + temp["directory"].toString() + ")";
  return temp;
}

bool DeployDialog::verifyData()
{
  return !projectCCB->getSelectedItem().isEmpty() &&
         !branchCCB->getSelectedItem().isEmpty() &&
         !pipelineCCB->getSelectedItem().isEmpty() &&
         !jobCCB->getSelectedItem().isEmpty() &&
         !machineCCB->getSelectedItem().isEmpty() &&
         !directoryCCB->getSelectedItem().isEmpty();
}

void DeployDialog::clearFileds()
{
  projectCCB->clear();
  branchCCB->clear();
  pipelineCCB->clear();
  jobCCB->clear();
  machineCCB->clear();
  directoryCCB->clear();
  ui->checkBox->setChecked(false);
  ui->lineEdit->clear();
  this->init();
}

void DeployDialog::setFields(const QJsonObject &data)
{
  branchCCB->setSelectedItem(data["branch"].toString());
  pipelineCCB->setSelectedItem(data["pipelineName"].toString());
  jobCCB->setSelectedItem(data["jobName"].toString());
  ui->lineEdit->setText(data["path"].toString());

  // using direct input to trigger shouldUpdate() slot and request items (should use cache)
  ui->comboBoxProject->setEditText(data["projectName"].toString());

  ui->comboBoxMachine->setEditText(data["machine"].toString()); // and again
  directoryCCB->setSelectedItem(data["directory"].toString());

  ui->checkBox->setChecked(data["createDir"].toBool());
}
