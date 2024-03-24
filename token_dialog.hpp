#ifndef TOKEN_DIALOG_HPP
#define TOKEN_DIALOG_HPP

#include <QDialog>
#include "cooler_dialog.hpp"

namespace Ui {
class TokenDialog;
}

class TokenDialog : public CoolerDialog
{
    Q_OBJECT

public:
    explicit TokenDialog(QWidget *parent = nullptr);
    ~TokenDialog();

    QJsonObject getData() override;
    bool verifyData() override;
    void clearFileds() override;
    void setFields(const QJsonObject& data) override;

private:
    Ui::TokenDialog *ui;
};

#endif // TOKEN_DIALOG_HPP
