#include "cooler_list.hpp"
#include "data_manager.hpp"

CoolerList::CoolerList(const QString& name, const QList<QPushButton*>& buttons, QListWidget* list, CoolerDialog* dialog)
{
    this->name = name;
    this->list = list;
    this->dialog = dialog;
    DataManager::addCoolerList(name);
    connect(buttons.at(0), &QPushButton::clicked, this, &CoolerList::onAddClick);
    connect(buttons.at(1), &QPushButton::clicked, this, &CoolerList::onRemoveClick);
    connect(buttons.at(2), &QPushButton::clicked, this, &CoolerList::onDuplicateClick);
    connect(buttons.at(3), &QPushButton::clicked, this, &CoolerList::onEditClick);
    connect(dialog, &CoolerDialog::accepted, this, &CoolerList::onDialogAccept);
}

CoolerDialog * const CoolerList::getDialog()
{
    return this->dialog;
}

void CoolerList::onDialogAccept()
{
    QJsonObject data = dialog->getData();
    dialog->clearFileds();
    if (data.value("display").toString().isEmpty()) return;
    if (editing == -1)
    {
        DataManager::appendCoolerList(name, data);
        list->addItem(data.value("display").toString());
    }
    else
    {
        DataManager::editCoolerList(name, editing, data);
        list->item(editing)->setText(data.value("display").toString());
        editing = -1;
    }
}

void CoolerList::onAddClick()
{
    dialog->show();
}

void CoolerList::onRemoveClick()
{
    auto temp  = list->selectedItems();
    size_t countSel = temp.count();
    if (countSel == 0) return;

    size_t i = 0;
    for (; !list->item(i)->isSelected(); ++i)
        ;

    DataManager::removeFromCoolerList(name, i, i + countSel);
    for (size_t i = 0; i < countSel; ++i)
    {
        delete temp.at(i);
    }
}

void CoolerList::onDuplicateClick()
{
    size_t countSel = list->selectedItems().count();
    if (countSel == 0) return;
    auto temp = DataManager::getData().value(name).toArray();

    size_t i = 0;
    for (; !list->item(i)->isSelected(); ++i)
        ;

    for (size_t j = i; j < i + countSel; ++j)
    {
        DataManager::appendCoolerList(name, temp.at(j).toObject());
        list->addItem(list->item(j)->text());
    }
    qDebug() << DataManager::getData();
}

void CoolerList::onEditClick()
{
    if (list->selectedItems().empty()) return;
    auto temp = DataManager::getData().value(name).toArray();
    size_t i = 0;
    for (; !list->item(i)->isSelected(); ++i)
        ;
    dialog->setFields(temp.at(i).toObject());
    dialog->show();
    editing = i;
}
