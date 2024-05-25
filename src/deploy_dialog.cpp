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
  connect(projectCCB, &CoolerComboBox::updated, this, &DeployDialog::projectSelected);
  connect(branchCCB, &CoolerComboBox::updated, this, &DeployDialog::branchSelected);
  connect(machineCCB, &CoolerComboBox::updated, this, &DeployDialog::machineSelected);
  connect(ui->buttonSelectDir, &QPushButton::clicked, this, &DeployDialog::pathSelectClicked);
}

DeployDialog::~DeployDialog()
{
  delete ui;
  delete projectCCB;
  delete branchCCB;
  delete jobCCB;
  delete machineCCB;
}

void DeployDialog::comboSetEnabled(bool enabled)
{
  ui->comboBoxProject->setEnabled(enabled);
  ui->comboBoxBranch->setEnabled(enabled);
  ui->comboBoxJob->setEnabled(enabled);
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
  connect(reply, &ApiReply::commitsReady, reply, [this, reply](QList<QString> &commits)
          {
    jobCommits = commits;
    reply->deleteLater(); });
  connect(reply, &ApiReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError e)
          {
    branchCCB->onError();
    comboSetEnabled(true);
    reply->deleteLater(); });
}

void DeployDialog::machineSelected(const QString &newText)
{
  ui->lineEditDirectory->setEnabled(newText != "localhost");
  ui->buttonSelectDir->setEnabled(newText == "localhost");
}

void DeployDialog::pathSelectClicked()
{
  QString path = QFileDialog::getExistingDirectory(this, tr("Select directory"));
  if (path.isEmpty())
    return;
  ui->lineEditDirectory->setText(path);
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
  temp["directory"] = ui->lineEditDirectory->text();
  temp["commit"] = jobCommits[jobCCB->getItems().indexOf(jobCCB->getSelectedItem())];
  QString tempStr = temp["directory"].toString();
  temp["directory"] = tempStr.endsWith("/") ? tempStr.left(tempStr.length() - 1) : tempStr;
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
  ui->lineEditDirectory->clear();
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
  ui->lineEditDirectory->setText(data["directory"].toString());

  // using direct input to trigger shouldUpdate() slot and request items (should use cache)
  ui->comboBoxProject->setEditText(data["projectName"].toString());

  ui->comboBoxMachine->setEditText(data["machine"].toString()); // and again
  QDialog::show();
}
