#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkReply>
#include "api_handler.hpp"
#include "cooler_list.hpp"
#include "download_manager.hpp"
#include "deploy_object.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
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
    void nextPage();
    void prevPage();

    void startDownload();

    void importClicked();
    void exportClicked();

signals:
    void apiUpdated();

private:
    ApiHandler api;
    //QNetworkAccessManager qnam;
    std::vector<DeployObject*> deployObjets;
    Ui::MainWindow* ui;
    CoolerList* tokenList;
    CoolerList* sshList;
    CoolerList* deployList;
};
#endif // MAINWINDOW_H
