#include "deploy_list.hpp"

#include <QLayout>
#include <QJsonObject>
#include <QJsonArray>
#include "deploy_list_element.hpp"
#include "data_manager.hpp"
#include "mylogger.hpp"

DeployList::DeployList(QWidget *widget, ComplexDialog *dialog, QWidget *parent)
    : ComplexList("deploy", widget, dialog, parent)
{
    this->runAllButton = widget->findChild<QPushButton *>("deployRunAllButton");
    this->addGroupButton = widget->findChild<QPushButton *>("deployAddGroupButton");
    this->addButton->hide();

    if (runAllButton)
        connect(runAllButton, &QPushButton::clicked, this, &DeployList::runAllButtonClicked);
    if (addGroupButton)
    {
        connect(addGroupButton, &QPushButton::clicked, this, &DeployList::addGroup);
    }
    scrollArea->layout()->addItem(spacer);
}

void DeployList::runAllButtonClicked()
{
    for (auto element : elements)
    {
        if (auto el = dynamic_cast<DeployListElement *>(element))
        {
            el->run();
        }
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
    auto element = new DeployListElement(":/ui/deploy_list_element.ui", data, ssh, this);
    elements.insert(0, element);
    DataManager::insertCoolerList(prefix, data, 0);
    QBoxLayout *layout = dynamic_cast<QBoxLayout *>(scrollArea->layout());
    layout->insertWidget(0, element->getWidget());
    connect(element->getButton("showLastUpdateButton"), &QPushButton::clicked, this, [element, this]
            { element->showLastUpdate(); });
    connect(element->getButton("deleteButton"), &QPushButton::clicked, this, [element, this]
            { removeElement(element); });
    connect(element->getButton("editButton"), &QPushButton::clicked, this, [element, this]
            { dialog->show(element->getData()); editing = element; });
    connect(element->getButton("duplicateButton"), &QPushButton::clicked, this, [element, this]
            { addElement(element->getData()); });
    if (!sshSuccess)
    {
        element->setStatus(CLEStatus::FAIL, "SSH ERROR");
    }
    connect(element->getButton("runButton"), &QPushButton::clicked, element, &DeployListElement::run);
    connect(element, &DeployListElement::done, this, [this, element]
            {
                QJsonObject data = element->getData();
                data["last_update"] = QDateTime::currentDateTime().toString();
                element->setData(data);
                DataManager::editCoolerList(prefix, elements.indexOf(element), data);
            });
}

void DeployList::addElementToGroup(DeployListGroup *group, const QJsonObject &data)
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
    auto element = new DeployListElement(":/ui/deploy_list_element_spaced.ui", data, ssh, this);
    int groupIndex = elements.indexOf(group);
    elements.insert(groupIndex + 1, element);
    DataManager::insertCoolerList(prefix, data, groupIndex + 1);
    QBoxLayout *layout = dynamic_cast<QBoxLayout *>(scrollArea->layout());

    layout->insertWidget(groupIndex + 1, element->getWidget());
    connect(element->getButton("showLastUpdateButton"), &QPushButton::clicked, this, [element, this]
            { element->showLastUpdate(); });
    connect(element->getButton("deleteButton"), &QPushButton::clicked, this, [element, this]
            { removeElement(element); });
    connect(element->getButton("editButton"), &QPushButton::clicked, this, [element, group, this]
            { dialog->show(element->getData()); editing = element; editingGroup = group; });
    connect(element->getButton("duplicateButton"), &QPushButton::clicked, this, [element, data, this]
            {
                if(auto group = getGroup(element))
                    addElementToGroup(group, element->getData());
                else
                    addElement(element->getData()); });
    if (!sshSuccess)
    {
        element->setStatus(CLEStatus::FAIL, "SSH ERROR");
    }
    connect(element->getButton("runButton"), &QPushButton::clicked, element, &DeployListElement::run);
    connect(element, &DeployListElement::done, this, [this, element]
            {
                QJsonObject data = element->getData();
                data["last_update"] = QDateTime::currentDateTime().toString();
                element->setData(data);
                DataManager::editCoolerList(prefix, elements.indexOf(element), data);
                QString numFiles = data["file"].toString().isEmpty() ? "ALL" : QString::number(data["file"].toString().split(";").size());
                QString message = "Deployed " + numFiles + " artifacts from ";
                message += data["projectName"].toString() + "/" + data["branch"].toString() + "/" + data["job"].toString() + " (" + data["commit"].toString() + ")";
                message += " to " + data["machine"].toString() + " (" + data["directory"].toString() + ")";
                MyLogger::log(message);
            });
    group->addElement(element);
}

void DeployList::addGroup()
{
    QJsonObject data = {
        {"isGroup", true},
        {"branch", ""},
        {"machine", ""},
        {"cachePath", ""},
        {"directory", ""},
        {"display", ""},
        {"file", ""},
        {"job", ""},
        {"project", ""},
        {"projectName", ""}};
    auto group = new DeployListGroup(":/ui/deploy_list_group.ui", data, this);
    DataManager::appendCoolerList(prefix, data);
    elements.append(group);
    groupHidden[group] = false;
    scrollArea->layout()->removeItem(spacer);
    scrollArea->layout()->addWidget(group->getWidget());
    scrollArea->layout()->addItem(spacer);
    connect(group->getButton("deleteButton"), &QPushButton::clicked, this, [group, this]
            {
            for (auto element : group->getElements())
            {
                this->removeElement(element);
            }
            this->removeElement(group); });
    connect(group->getButton("addButton"), &QPushButton::clicked, this, [group, this]
            { this->dialog->show(); editingGroup = group; });
    connect(group->getButton("showHideButton"), &QPushButton::clicked, this, [group, this]
            { 
            groupHidden[group] = !groupHidden[group];
            for (auto element : group->getElements())
            {
                element->getWidget()->setVisible(!groupHidden[group]);
            } });
}

void DeployList::removeElement(ComplexListElement *element)
{
    DataManager::removeFromCoolerList(prefix, elements.indexOf(element));
    elements.removeOne(element);
    delete element->getWidget();
    delete element;
}

void DeployList::init(const QJsonArray &data)
{
    DeployListGroup *group = nullptr;
    DataManager::clearCoolerList(prefix);
    for (auto element : data)
    {
        QJsonObject obj = element.toObject();
        if (obj["isGroup"].toBool())
        {
            addGroup();
            group = dynamic_cast<DeployListGroup *>(elements.last());
        }
        else
        {
            if (group)
                addElementToGroup(group, obj);
            else
                addElement(obj);
        }
    }
}

void DeployList::onDialogReject()
{
    editing = nullptr;
    editingGroup = nullptr;
}

DeployListGroup *DeployList::getGroup(DeployListElement *element)
{
    for (int i = elements.indexOf(element); i >= 0; --i)
    {
        if (elements[i]->getData()["isGroup"].toBool())
            return dynamic_cast<DeployListGroup *>(elements[i]);
    }
    return nullptr;
}

void DeployList::onDialogAccept(const QJsonObject &data)
{
    if (editing)
    {
        editing->setData(data);
        editing->setStatus(CLEStatus::DEFAULT);
        DataManager::editCoolerList(prefix, elements.indexOf(editing), data);
        editing = nullptr;
    }
    else if (editingGroup)
    {
        addElementToGroup(editingGroup, data);
        if (dialog->isHidden())
            editingGroup = nullptr;
    }
    else
    {
        addElement(data);
    }
}
