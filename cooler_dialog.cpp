#include "cooler_dialog.hpp"

CoolerDialog::CoolerDialog(QWidget *parent)
    : QDialog(parent) { }

void CoolerDialog::accept()
{
    if(this->verifyData()) QDialog::accept();
}

void CoolerDialog::reject()
{
    this->clearFileds();
    QDialog::reject();
}

void CoolerDialog::apply()
{
    if(this->verifyData()) emit applied();
}
