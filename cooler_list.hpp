#ifndef COOLER_LIST_HPP
#define COOLER_LIST_HPP

#include <QPushButton>
#include <QList>
#include <QListView>
#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QListWidget>
#include "cooler_dialog.hpp"


class CoolerList : public QObject
{
public:
    CoolerList(const QString& name, const QList<QPushButton*>& buttons, QListWidget* listView, CoolerDialog* dialog);

    CoolerDialog * const getDialog();

private slots:
    void onDialogAccept();
    void onAddClick();
    void onRemoveClick();
    void onDuplicateClick();
    void onEditClick();

private:
    QString name;
    QListWidget* list;
    CoolerDialog* dialog;
    int editing = -1;
};

#endif // COOLER_LIST_HPP
