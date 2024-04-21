#ifndef TOKEN_DIALOG_HPP
#define TOKEN_DIALOG_HPP

#include <QDialog>
#include "api_handler.hpp"
#include "complex_dialog.hpp"

namespace Ui {
class TokenDialog;
}

class TokenDialog : public ComplexDialog
{
    Q_OBJECT

public:
    explicit TokenDialog(QWidget *parent = nullptr);
    ~TokenDialog();

    QJsonObject getData() override;
    bool verifyData(const QJsonObject& data, bool visual) override;
    void clearFileds() override;
    void show(const QJsonObject& data) override;

private:
    ApiHandler api;
    bool verified = false;
    Ui::TokenDialog *ui;
};

#endif // TOKEN_DIALOG_HPP
