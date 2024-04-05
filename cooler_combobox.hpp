#ifndef COOLER_COMBOBOX_HPP
#define COOLER_COMBOBOX_HPP

#include <QComboBox>
#include <QLabel>
#include <QObject>

class CoolerComboBox : public QObject {
  Q_OBJECT
public:
  explicit CoolerComboBox(QLabel *label, QComboBox *comboBox,
                          QObject *parent, bool skipUpdateCheck = false);
  void clearItems();
  void clear();
  void updateItems(const QMap<QString, QString> &idToItem, bool keepCurrent = true);
  void updateItems(const QList<QString> &items, bool keepCurrent = true);
  void setSelectedItem(const QString& item);
  void onError();

  QList<QString> getItems() const;
  QString getSelectedItem() const;
  QString getCurrentText() const;
  QString getSelectedItemId() const;

signals:
  void updated(const QString &newText);

private slots:
  void shouldUpdate(const QString &newText);

private:
  bool skipUpdateCheck = false;
  bool updateBlock = false;
  QString selectedItem;
  QMap<QString, QString> idToItem; // keys (ids) to text
  QList<QString> items;
  QLabel *label;
  QComboBox *comboBox;
};

#endif // COOLER_COMBOBOX_HPP
