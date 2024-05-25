#ifndef DEPLOY_LIST_HPP
#define DEPLOY_LIST_HPP

#include "complex_list.hpp"
#include "api_handler.hpp"
#include "deploy_list_group.hpp"

class DeployList : public ComplexList
{
    Q_OBJECT
public:
    explicit DeployList(QWidget *widget, ComplexDialog *dialog, QWidget *parent = nullptr);
    virtual ~DeployList() = default;
    virtual void addElement(const QJsonObject &data) override;
    void addElementToGroup(DeployListGroup *group, const QJsonObject &data);
    void addGroup();
    void removeElement(ComplexListElement *element);
    virtual void init(const QJsonArray &data) override;

protected slots:
    void runAllButtonClicked();
    virtual void onDialogAccept(const QJsonObject &data) override;
    virtual void onDialogReject() override;
    DeployListGroup *getGroup(DeployListElement *element);

protected:
    QPushButton *runAllButton;
    QPushButton *addGroupButton;
    QMap<DeployListGroup *, bool> groupHidden;
    DeployListGroup *editingGroup = nullptr;
};

#endif // DEPLOY_LIST_HPP