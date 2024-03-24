#ifndef DEPLOY_DIALOG_HPP
#define DEPLOY_DIALOG_HPP

#include <QDialog>
#include <QJsonObject>
#include "cooler_dialog.hpp"
#include "ssh_connection.hpp"
#include <QComboBox>
#include <QLabel>

namespace Ui {
class DeployDialog;
};

class DeployDialog : public CoolerDialog
{
    Q_OBJECT

public:
    explicit DeployDialog(QWidget *parent = nullptr);
    ~DeployDialog();

    void init();

    QJsonObject getData() override;
    bool verifyData() override;
    void clearFileds() override;
    void setFields(const QJsonObject& data) override;

private slots:
    void projectSelected(const QString& text);
    void branchSelected(const QString& text);
    void pipelineSelected(const QString& text);
    void jobSelected(const QString& text);
    void machineSelected(const QString& text);
    void pathEdited(const QString& text);
    void pathSelectClicked();
    void dirSaveChecked(bool state);
    void curlClicked(bool state);
    void sftpClicked(bool state);

private:
    void updateComboBox(QComboBox* box, const QList<QString>& items);
    void smartClean(const QString& key, QComboBox* box, QLabel* label);
    bool shouldUpdate(const QString& value, const QList<QString>& checkList, QLabel* label);
    void comboSetEnabled(bool enabled);

    Ui::DeployDialog *ui;
    QJsonObject temp;
    QList<QString> branches;
    QMap<size_t, QString> pipelines;
    QHash<size_t, QString> jobs;
    SSHConnection ssh;
    QList<QString> directories;
};

#endif // DEPLOY_DIALOG_HPP
