#ifndef SSH_DIALOG_HPP
#define SSH_DIALOG_HPP

#include <QDialog>
#include <QJsonObject>
#include "complex_dialog.hpp"

namespace Ui {
class SshDialog;
}

class SshDialog : public ComplexDialog
{
    Q_OBJECT

public:
    explicit SshDialog(QWidget *parent = nullptr);
    ~SshDialog();

    QJsonObject getData() override;
    bool verifyData(const QJsonObject& data, bool visual) override;
    void clearFileds() override;
    void show(const QJsonObject& data) override;

private slots:
    void selectKeyDir();

private:
    Ui::SshDialog *ui;
};

#endif // SSH_DIALOG_HPP
