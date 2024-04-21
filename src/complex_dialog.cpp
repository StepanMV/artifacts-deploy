#include "complex_dialog.hpp"

#include <QJsonObject>

ComplexDialog::ComplexDialog(QWidget *parent)
    : QDialog(parent) {}

void ComplexDialog::show()
{
    QDialog::show();
}

void ComplexDialog::accept()
{
    QJsonObject data = getData();
    if (verifyData(data, true))
    {
        emit acceptedData(data);
        clearFileds();
        qDebug() << "Accepted";
        QDialog::accept();
    }
}

void ComplexDialog::reject()
{
    clearFileds();
    QDialog::reject();
}

void ComplexDialog::apply()
{
    QJsonObject data = getData();
    if (verifyData(data, true))
        emit acceptedData(data);
}
