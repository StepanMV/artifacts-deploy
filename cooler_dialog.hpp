#ifndef COOLERDIALOG_HPP
#define COOLERDIALOG_HPP

#include <QDialog>

class CoolerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CoolerDialog(QWidget *parent = nullptr);

    virtual QJsonObject getData() = 0;
    virtual bool verifyData() = 0;
    virtual void clearFileds() = 0;
    virtual void setFields(const QJsonObject& data) = 0;

protected slots:
    void accept();
    void reject();

};

#endif // COOLERDIALOG_HPP
