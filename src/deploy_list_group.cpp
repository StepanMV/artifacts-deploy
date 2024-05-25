#include "deploy_list_group.hpp"

#include <QUiLoader>
#include <QFile>
#include <QLayout>
#include "complex_list_element.hpp"

DeployListGroup::DeployListGroup(const QString & filename, const QJsonObject &data, QWidget * parent)
    : ComplexListElement(filename, data, parent)
{
    connect(widget->findChild<QPushButton *>("runButton"), &QPushButton::clicked, this, &DeployListGroup::runButtonClicked);
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
