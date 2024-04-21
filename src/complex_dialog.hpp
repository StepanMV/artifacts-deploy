#ifndef COMPLEX_DIALOG_HPP
#define COMPLEX_DIALOG_HPP

#include <QDialog>

class ComplexDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ComplexDialog(QWidget *parent = nullptr);
    virtual ~ComplexDialog() = default;

    virtual QJsonObject getData() = 0;
    virtual void clearFileds() = 0;
    virtual bool verifyData(const QJsonObject& data, bool visual = false) = 0;
    virtual void show();
    virtual void show(const QJsonObject& data) = 0;

signals:
    void acceptedData(const QJsonObject& data);

protected slots:
    void accept();
    void reject();
    void apply();
};

#endif // COMPLEX_DIALOG_HPP