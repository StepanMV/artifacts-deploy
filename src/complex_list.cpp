#include "complex_list.hpp"

#include <QLayout>
#include "data_manager.hpp"
#include <QJsonArray>

ComplexList::ComplexList(const QString &prefix, QWidget *widget, ComplexDialog *dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), prefix(prefix), elementTemplateFilename("../src/ui/" + prefix + "_list_element.ui")
{
    DataManager::addCoolerList(prefix);
    this->elements = QList<ComplexListElement *>();
    this->addButton = widget->findChild<QPushButton *>(prefix + "AddButton");
    this->clearButton = widget->findChild<QPushButton *>(prefix + "ClearButton");
    this->verifyAllButton = widget->findChild<QPushButton *>(prefix + "VerifyAllButton");
    this->scrollArea = widget->findChild<QWidget *>(prefix + "ScrollAreaContents");
    this->spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    if (addButton)
        connect(addButton, &QPushButton::clicked, this, [this]
                { this->dialog->show(); });
    if (clearButton)
        connect(clearButton, &QPushButton::clicked, this, &ComplexList::clear);
    if (verifyAllButton)
        connect(verifyAllButton, &QPushButton::clicked, this, &ComplexList::verifyAllButtonClicked);
    connect(dialog, &ComplexDialog::acceptedData, this, &ComplexList::onDialogAccept);
}

void ComplexList::clear()
{
    qDebug() << "Clearing " << prefix << " list";
    DataManager::clearCoolerList(prefix);
    if (elements.isEmpty())
        qDeleteAll(elements);
    elements.clear();
}

void ComplexList::verifyAllButtonClicked()
{
    for (auto element : elements)
    {
        if (dialog->verifyData(element->getData()))
        {
            element->setStatus(CLEStatus::SUCCESS);
        }
        else
        {
            element->setStatus(CLEStatus::FAIL);
        }
    }
}

void ComplexList::onDialogAccept(const QJsonObject &data)
{
    if (editing)
    {
        editing->setData(data);
        DataManager::editCoolerList(prefix, elements.indexOf(editing), data);
        editing = nullptr;
    }
    else
    {
        addElement(data);
    }
}

void ComplexList::addElement(const QJsonObject &data)
{
    auto element = new ComplexListElement(elementTemplateFilename, data, this);
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
    connect(element->getButton("verifyButton"), &QPushButton::clicked, this, [element, this]
            {
                element->setStatus(CLEStatus::PENDING);
                if(dialog->verifyData(element->getData()))
                element->setStatus(CLEStatus::SUCCESS);
                else element->setStatus(CLEStatus::FAIL); });
}

void ComplexList::init(const QJsonArray &data)
{
    DataManager::clearCoolerList(prefix);
    for (auto element : data)
    {
        addElement(element.toObject());
    }
}
