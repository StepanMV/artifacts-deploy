#ifndef DEPLOY_LIST_HPP
#define DEPLOY_LIST_HPP

#include "complex_list.hpp"
#include "api_handler.hpp"

class DeployList : public ComplexList
{
    Q_OBJECT
public:
    explicit DeployList(QWidget *widget, ComplexDialog *dialog, QWidget *parent = nullptr);
    virtual ~DeployList() = default;
    virtual void addElement(const QJsonObject &data) override;
    virtual void init(const QJsonArray &data) override;

protected slots:
    void runAllButtonClicked();

protected:

    QPushButton *runAllButton;
};

#endif // DEPLOY_LIST_HPP