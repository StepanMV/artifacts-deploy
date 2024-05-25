#ifndef DEPLOYTLISTGROUP_HPP
#define DEPLOYTLISTGROUP_HPP

#include <QWidget>
#include <QJsonObject>
#include <QPushButton>
#include <QLabel>
#include "deploy_list_element.hpp"

class DeployListGroup : public ComplexListElement
{
    Q_OBJECT

public:
    DeployListGroup(const QString &filename, const QJsonObject &data, QWidget *parent = nullptr);
    virtual ~DeployListGroup() = default;

    void addElement(DeployListElement *element);
    QList<DeployListElement *> getElements();

protected slots:
    void runButtonClicked();

protected:
    QList<DeployListElement *> elements;
};

#endif // DEPLOYTLISTGROUP_HPP