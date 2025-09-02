#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "clientcore.h"
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
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    // 添加与ClientCore交互所需的基本信号
    void logout();  // 登出信号
    void joinTicketSend(const QJsonObject &response); // 新增信号
    void acceptTicketSend(QJsonObject &data);//承接工单
    void completeTicketSend(QJsonObject &data);
    void create_chatroom();

private slots:
    void on_joinTicket_clicked(); // 新增槽函数
    void on_acceptTicket_clicked();
    void on_completeTicket_clicked();
    // void on_pushButton_clicked();
    // void on_pushButton_2_clicked();
    // void on_pushButton_3_clicked();


    // 声明槽函数，参数需与信号一致（const QByteArray &）
    void onTableInitial(const Session *,QJsonArray &data);
    void onAddTicket(const Session *,const QJsonObject &data);
    void onConfirmCompleted(const QString &ticketId);  // 响应信号，删除表格行
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    PageOrder *po1;
    Page_Mine *pm1;
    PageDevice *pd1;
    QString currentTicket;//保存承接的工单号，要初始化
};

#endif // MAINWINDOW_H
