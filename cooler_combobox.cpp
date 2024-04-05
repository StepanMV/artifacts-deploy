#include "cooler_combobox.hpp"

CoolerComboBox::CoolerComboBox(QLabel *label, QComboBox *comboBox,
                               QObject *parent, bool skipUpdateCheck)
    : QObject{parent}
{
  this->skipUpdateCheck = skipUpdateCheck;
  this->comboBox = comboBox;
  this->comboBox->setCompleter(nullptr);
  this->label = label;
  connect(this->comboBox, &QComboBox::editTextChanged, this,
          &CoolerComboBox::shouldUpdate);
}

void CoolerComboBox::clearItems()
{
  updateBlock = true;
  QString tempStr = comboBox->currentText();
  selectedItem = "";
  comboBox->clear();
  label->setStyleSheet("");
  comboBox->setEditText(tempStr);
  updateBlock = false;
}

void CoolerComboBox::clear()
{
  updateBlock = true;
  selectedItem = "";
  items.clear();
  idToItem.clear();
  comboBox->clear();
  comboBox->setEditText("");
  label->setStyleSheet("");
  updateBlock = false;
}

void CoolerComboBox::updateItems(const QMap<QString, QString> &idToItem,
                                 bool keepCurrent)
{
  updateBlock = true;
  this->idToItem = idToItem;
  this->items = idToItem.values();
  QString tempStr = comboBox->currentText();
  comboBox->clear();
  comboBox->addItems(items);
  label->setStyleSheet("");
  comboBox->setEditText("");
  updateBlock = false;
  if (keepCurrent) comboBox->setEditText(tempStr); // emits updated()
}

void CoolerComboBox::updateItems(const QList<QString> &items,
                                 bool keepCurrent)
{
  updateBlock = true;
  this->items = items;
  this->idToItem.clear();
  QString tempStr = comboBox->currentText();
  comboBox->clear();
  comboBox->addItems(items);
  label->setStyleSheet("");
  comboBox->setEditText("");
  updateBlock = false;
  if (keepCurrent) comboBox->setEditText(tempStr); // emits updated()
}

void CoolerComboBox::setSelectedItem(const QString &item)
{
  updateBlock = true;
  comboBox->setEditText(item);
  selectedItem = item;
  label->setStyleSheet("color: green;");
  updateBlock = false;
}

QList<QString> CoolerComboBox::getItems() const
{
    return items;
}

QString CoolerComboBox::getSelectedItem() const
{
  return selectedItem;
}

QString CoolerComboBox::getCurrentText() const
{
    return comboBox->currentText();
}

void CoolerComboBox::onError()
{
  selectedItem = "";
  label->setStyleSheet("color: red;");
}

QString CoolerComboBox::getSelectedItemId() const
{
  for (auto it = idToItem.cbegin(); it != idToItem.cend(); ++it)
  {
    if (it.value() == selectedItem)
    {
      return it.key();
    }
  }
  return "";
}

void CoolerComboBox::shouldUpdate(const QString &newText)
{
  if (updateBlock) return;
  if (skipUpdateCheck) emit updated(newText);
  for (auto it = items.cbegin(); it != items.cend(); ++it)
  {
    if (*it == newText)
    {
      label->setStyleSheet("color: green;");
      selectedItem = newText;
      emit updated(newText);
      return;
    }
  }
  selectedItem = "";
  label->setStyleSheet("color: red;");
}
