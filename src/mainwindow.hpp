#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkReply>
#include "api_handler.hpp"
#include "complex_list.hpp"
#include "deploy_list.hpp"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateApiUrl();
    void updateUserToken();

    void importClicked();
    void exportClicked();

signals:
    void apiUpdated();
    void tokenUpdated();

private:
    void import(const QString &filename);

    ApiHandler api;
    Ui::MainWindow *ui;
    ComplexList *tokenList;
    ComplexList *sshList;
    DeployList *deployList;
};
#endif // MAINWINDOW_H
