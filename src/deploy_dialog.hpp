#ifndef DEPLOY_DIALOG_HPP
#define DEPLOY_DIALOG_HPP

#include "api_handler.hpp"
#include "cooler_combobox.hpp"
#include "complex_dialog.hpp"
#include "ssh_connection.hpp"
#include <QComboBox>
#include <QDialog>
#include <QJsonObject>
#include <QLabel>

namespace Ui {
class DeployDialog;
};

class DeployDialog : public ComplexDialog
{
  Q_OBJECT

public:
  explicit DeployDialog(QWidget *parent = nullptr);
  ~DeployDialog();

  void init();

  QJsonObject getData() override;
  bool verifyData(const QJsonObject& data, bool visual = false) override;
  void clearFileds() override;
  virtual void show() override;
  void show(const QJsonObject &data) override;

private slots:
  void projectSelected(const QString &newText);
  void branchSelected(const QString &newText);
  void machineSelected(const QString &newText);

  void pathSelectClicked();

private:
  void comboSetEnabled(bool enabled);

  ApiHandler api;
  Ui::DeployDialog *ui;
  QList<QString> jobCommits; // idk where else to put it
  CoolerComboBox *projectCCB;
  CoolerComboBox *branchCCB;
  CoolerComboBox *jobCCB;
  CoolerComboBox *machineCCB;
};

#endif // DEPLOY_DIALOG_HPP
