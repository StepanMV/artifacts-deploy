#ifndef SSH_DIALOG_HPP
#define SSH_DIALOG_HPP

#include <QDialog>
#include <QJsonObject>
#include "cooler_dialog.hpp"

namespace Ui {
class SshDialog;
}

class SshDialog : public CoolerDialog
{
    Q_OBJECT

public:
    explicit SshDialog(QWidget *parent = nullptr);
    ~SshDialog();

    QJsonObject getData() override;
    bool verifyData() override;
    void clearFileds() override;
    void setFields(const QJsonObject& data) override;

private slots:
    void selectKeyDir();

private:
    Ui::SshDialog *ui;
};

#endif // SSH_DIALOG_HPP
