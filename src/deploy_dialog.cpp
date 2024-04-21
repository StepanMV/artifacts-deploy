#include "deploy_dialog.hpp"
#include "api_handler.hpp"
#include "cooler_combobox.hpp"
#include "data_manager.hpp"
#include "ui/ui_deploy_dialog.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>

DeployDialog::DeployDialog(QWidget *parent)
    : ComplexDialog(parent), ui(new Ui::DeployDialog), api(this)
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
  jobCCB = new CoolerComboBox(ui->labelJob, ui->comboBoxJob, this);
  machineCCB = new CoolerComboBox(ui->labelMachine, ui->comboBoxMachine, this);
  directoryCCB = new CoolerComboBox(ui->labelDirectory, ui->comboBoxDirectory, this, true);
  connect(projectCCB, &CoolerComboBox::updated, this, &DeployDialog::projectSelected);
  connect(branchCCB, &CoolerComboBox::updated, this, &DeployDialog::branchSelected);
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
  qDebug() << "DeployDialog::init()";
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
  jobCCB->clearItems();
  ApiReply *reply = api.getBranches(projectCCB->getSelectedItemId());
  connect(reply, &ApiReply::branchesReady, reply, [this, reply](QList<QString> &branches)
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
  jobCCB->clearItems();
  ApiReply *reply = api.getJobs(projectCCB->getSelectedItemId(), branchCCB->getSelectedItem());
  connect(reply, &ApiReply::jobsReady, reply, [this, reply](QList<QString> &jobs)
          {
            comboSetEnabled(true);
            jobCCB->updateItems(jobs); // triggers shouldUpdate()
            reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError e)
          {
    branchCCB->onError();
    comboSetEnabled(true);
    reply->deleteLater(); });
}

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
  temp["name"] = ui->lineEditName->text();
  temp["project"] = projectCCB->getSelectedItemId();
  temp["projectName"] = projectCCB->getSelectedItem();
  temp["branch"] = branchCCB->getSelectedItem();
  temp["job"] = jobCCB->getSelectedItem();
  temp["file"] = ui->lineEditFile->text();
  temp["machine"] = machineCCB->getSelectedItem();
  temp["directory"] = directoryCCB->getSelectedItem();
  QString tempStr = temp["directory"].toString();
  temp["directory"] = tempStr.endsWith("/") ? tempStr.left(tempStr.length() - 1) : tempStr;
  temp["createDir"] = ui->checkBox->isChecked();
  temp["cachePath"] = "cache/" + temp["project"].toString() + "_" + temp["job"].toString() + "/" +
                      (temp["file"].toString().isEmpty() ? "artifacts.zip" : temp["file"].toString());
  if (!temp["name"].toString().isEmpty())
    temp["display"] = temp["name"].toString();
  else
    temp["display"] = temp["projectName"].toString() + "/" + temp["branch"].toString() +
                      "/" + temp["job"].toString() + " (" +
                      (temp["file"].toString().isEmpty() ? "*" : temp["file"].toString()) +
                      ") -> " + temp["machine"].toString() +
                      "(" + temp["directory"].toString() + ")";
  return temp;
}

bool DeployDialog::verifyData(const QJsonObject &data, bool visual)
{
  return !data["project"].toString().isEmpty() &&
         !data["branch"].toString().isEmpty() &&
         !data["job"].toString().isEmpty() &&
         !data["machine"].toString().isEmpty() &&
         !data["directory"].toString().isEmpty();
}

void DeployDialog::clearFileds()
{
  projectCCB->clear();
  branchCCB->clear();
  jobCCB->clear();
  machineCCB->clear();
  directoryCCB->clear();
  ui->checkBox->setChecked(false);
  ui->lineEditFile->clear();
  ui->lineEditName->clear();
}

void DeployDialog::show()
{
  this->init();
  QDialog::show();
}

void DeployDialog::show(const QJsonObject &data)
{
  this->init();
  branchCCB->setSelectedItem(data["branch"].toString());
  jobCCB->setSelectedItem(data["job"].toString());
  ui->lineEditFile->setText(data["file"].toString());
  ui->lineEditName->setText(data["name"].toString());

  // using direct input to trigger shouldUpdate() slot and request items (should use cache)
  ui->comboBoxProject->setEditText(data["projectName"].toString());

  ui->comboBoxMachine->setEditText(data["machine"].toString()); // and again
  directoryCCB->setSelectedItem(data["directory"].toString());

  ui->checkBox->setChecked(data["createDir"].toBool());
  QDialog::show();
}