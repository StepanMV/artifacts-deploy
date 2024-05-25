#ifndef COMPLEX_LIST_HPP
#define COMPLEX_LIST_HPP

#include <QWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include "complex_dialog.hpp"
#include "complex_list_element.hpp"

class ComplexList : public QWidget
{
    Q_OBJECT
public:
    explicit ComplexList(const QString &prefix, QWidget *widget, ComplexDialog *dialog, QWidget *parent = nullptr);
    virtual ~ComplexList() = default;
    virtual void addElement(const QJsonObject &data);
    virtual void init(const QJsonArray &data);

public slots:
    void clear();

protected slots:
    virtual void onDialogAccept(const QJsonObject &data);
    virtual void onDialogReject();

protected:

    QString prefix;
    ComplexListElement *editing = nullptr;
    QWidget *scrollArea;
    ComplexDialog *dialog;
    QPushButton *addButton;
    QPushButton *clearButton;
    QPushButton *verifyAllButton;
    QSpacerItem *spacer;
    QList<ComplexListElement *> elements;
    QString elementTemplateFilename;
};

#endif // COMPLEX_LIST_HPP