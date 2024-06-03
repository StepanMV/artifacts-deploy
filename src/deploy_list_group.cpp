#include "deploy_list_group.hpp"

#include <QUiLoader>
#include <QFile>
#include <QLayout>
#include <QLineEdit>
#include "complex_list_element.hpp"

DeployListGroup::DeployListGroup(const QString & filename, const QJsonObject &data, QWidget * parent)
    : ComplexListElement(filename, data, parent)
{
    widget->findChild<QLineEdit *>("nameEdit")->hide();
    connect(widget->findChild<QPushButton *>("runButton"), &QPushButton::clicked, this, &DeployListGroup::runButtonClicked);
    connect(widget->findChild<QPushButton *>("renameButton"), &QPushButton::clicked, this, &DeployListGroup::renameButtonClicked);
}

void DeployListGroup::addElement(DeployListElement *element)
{
    elements.append(element);
}

QList<DeployListElement *> DeployListGroup::getElements()
{
    return elements;
}
void DeployListGroup::runButtonClicked()
{
    for (auto element : elements)
    {
        element->run();
    }
}

void DeployListGroup::renameButtonClicked()
{
    if (editMode)
    {
        widget->findChild<QPushButton *>("renameButton")->setText("✎");
        widget->findChild<QLabel *>("nameLabel")->setText(widget->findChild<QLineEdit *>("nameEdit")->text());
        widget->findChild<QLineEdit *>("nameEdit")->hide();
        widget->findChild<QLabel *>("nameLabel")->show();
        data["display"] = widget->findChild<QLabel *>("nameLabel")->text();
    }
    else
    {
        widget->findChild<QPushButton *>("renameButton")->setText("✔️");
        widget->findChild<QLabel *>("nameLabel")->hide();
        widget->findChild<QLineEdit *>("nameEdit")->setText(widget->findChild<QLabel *>("nameLabel")->text());
        widget->findChild<QLineEdit *>("nameEdit")->show();
        data["display"] = widget->findChild<QLineEdit *>("nameEdit")->text();
    }
    editMode = !editMode;
}
