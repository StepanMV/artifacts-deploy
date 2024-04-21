#include "complex_list_element.hpp"

#include <QUiLoader>
#include <QFile>
#include "data_manager.hpp"

ComplexListElement::ComplexListElement(const QString &filename, const QJsonObject &data, QWidget *parent)
    : QWidget(parent), data(data)
{
    QUiLoader loader;
    QFile file(filename);
    file.open(QFile::ReadOnly);
    this->widget = loader.load(&file, this);
    file.close();

    this->displayLabel = widget->findChild<QLabel *>("displayLabel");
    displayLabel->setText(data["display"].toString());
}

void ComplexListElement::setStatus(CLEStatus status, const QString &message)
{
    switch (status)
    {
    case CLEStatus::DEFAULT:
        displayLabel->setStyleSheet("");
        break;
    case CLEStatus::PENDING:
        displayLabel->setStyleSheet("color: blue;");
        break;
    case CLEStatus::SUCCESS:
        displayLabel->setStyleSheet("color: green;");
        break;
    case CLEStatus::FAIL:
        displayLabel->setStyleSheet("color: red;");
        break;
    }
    QString text = displayLabel->text();
    if (text.startsWith("["))
        text = text.mid(text.indexOf("]") + 2);
    if (!message.isEmpty())
        displayLabel->setText("[" + message + "] " + text);
    else
        displayLabel->setText(text);
}

const QJsonObject &ComplexListElement::getData()
{
    return data;
}

void ComplexListElement::setData(const QJsonObject &data)
{
    this->data = data;
    displayLabel->setText(data["display"].toString());
}

QPushButton *ComplexListElement::getButton(const QString &name)
{
    return widget->findChild<QPushButton *>(name);
}

QWidget *ComplexListElement::getWidget()
{
    return widget;
}
