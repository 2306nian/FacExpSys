#ifndef MAINWINDOW_MAIN_H
#define MAINWINDOW_MAIN_H

#include <QMainWindow>
#include <pageorder.h>
#include <page_mine.h>
#include <pagedevice.h>
#include "globaldatas.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow_main: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow_main(QMainWindow *parent = nullptr);
    ~MainWindow_main();

signals:
    // 添加与ClientCore交互所需的基本信号
    void logout();  // 登出信号

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();

private:
    Ui::MainWindow *ui;
    PageOrder *po1;
    Page_Mine *pm1;
    PageDevice *pd1;
};

#endif // MAINWINDOW_MAIN_H


