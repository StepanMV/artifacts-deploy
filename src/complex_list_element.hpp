#ifndef COMPLEXTLISTELEMENT_HPP
#define COMPLEXTLISTELEMENT_HPP

#include <QWidget>
#include <QJsonObject>
#include <QPushButton>
#include <QLabel>

enum class CLEStatus
{
    DEFAULT,
    PENDING,
    SUCCESS,
    FAIL
};

class ComplexListElement : public QWidget
{
    Q_OBJECT

public:
    ComplexListElement(const QString &filename, const QJsonObject &data, QWidget *parent = nullptr);
    virtual ~ComplexListElement() = default;

    void setStatus(CLEStatus status, const QString &message = "");
    const QJsonObject &getData();
    virtual void setData(const QJsonObject &data);
    QPushButton *getButton(const QString &name);
    QWidget *getWidget();

protected:
    QLabel *displayLabel;
    QJsonObject data;
    QWidget *widget;
};

#endif // COMPLEXTLISTELEMENT_HPP