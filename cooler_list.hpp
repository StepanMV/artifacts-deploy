#ifndef COOLER_LIST_HPP
#define COOLER_LIST_HPP

#include "cooler_dialog.hpp"
#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QListView>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>

class CoolerList : public QObject {
  Q_OBJECT
public:
  CoolerList(const QString &name, const QList<QPushButton *> &buttons, QListWidget *listView,
             CoolerDialog *dialog, QObject *parent = nullptr);

  CoolerDialog *const getDialog();

private slots:
  void onDialogAccept();
  void onDialogApply();
  void onAddClick();
  void onRemoveClick();
  void onDuplicateClick();
  void onEditClick();

private:
  QString name;
  QListWidget *list;
  CoolerDialog *dialog;
  int editing = -1;
};

#endif // COOLER_LIST_HPP
