#include "deploy_list.hpp"

#include <QLayout>
#include <QJsonObject>
#include <QJsonArray>
#include "deploy_list_element.hpp"
#include "data_manager.hpp"

DeployList::DeployList(QWidget *widget, ComplexDialog *dialog, QWidget *parent)
    : ComplexList("deploy", widget, dialog, parent)
{
    this->runAllButton = widget->findChild<QPushButton *>("deployRunAllButton");

    if (runAllButton)
        connect(runAllButton, &QPushButton::clicked, this, &DeployList::runAllButtonClicked);
}

void DeployList::runAllButtonClicked()
{
    for (auto element : elements)
    {
        dynamic_cast<DeployListElement *>(element)->run();
    }
}

void DeployList::addElement(const QJsonObject &data)
{
    SSHConnection *ssh = nullptr;
    bool sshSuccess = true;
    try
    {
        ssh = DataManager::getConnection(data["machine"].toString());
    }
    catch (const SshError &e)
    {
        sshSuccess = false;
    }
    auto element = new DeployListElement(data, ssh, this);
    elements.append(element);
    DataManager::appendCoolerList(prefix, data);
    scrollArea->layout()->removeItem(spacer);
    scrollArea->layout()->addWidget(element->getWidget());
    scrollArea->layout()->addItem(spacer);
    connect(element->getButton("deleteButton"), &QPushButton::clicked, this, [element, this]
            { 
            DataManager::removeFromCoolerList(prefix, elements.indexOf(element));
            elements.removeOne(element);
            delete element->getWidget();
            delete element; });
    connect(element->getButton("editButton"), &QPushButton::clicked, this, [element, this]
            { dialog->show(element->getData()); editing = element; });
    connect(element->getButton("duplicateButton"), &QPushButton::clicked, this, [element, this]
            { addElement(element->getData()); });
    if (!sshSuccess)
    {
        element->setStatus(CLEStatus::FAIL, "SSH ERROR");
        return;
    }
    connect(element->getButton("verifyButton"), &QPushButton::clicked, this, [element, this]
            { element->setStatus(CLEStatus::PENDING, "Verifying...");
            if(dialog->verifyData(element->getData()))
            element->setStatus(CLEStatus::SUCCESS);
            else element->setStatus(CLEStatus::FAIL, "INVALID DATA"); });
    connect(element->getButton("runButton"), &QPushButton::clicked, element, &DeployListElement::run);
}

void DeployList::init(const QJsonArray &data)
{
    DataManager::clearCoolerList(prefix);
    for (auto element : data)
    {
        QJsonObject obj = element.toObject();
        // dialog->verifyData(obj);
        // obj = dialog->getData();
        addElement(obj);
    }
    dialog->clearFileds();
}
