#ifndef CLIENTCORE_H
#define CLIENTCORE_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMap>
#include <QTcpSocket>
#include <QJsonObject>
#include "MainWindow_main.h"
#include "camerastreamer.h"
#include "register.h"
#include "videoplayer.h"
#include "widget.h"
#include "session.h"
#include<QJsonDocument>
#include "globaldatas.h"
#include "chatroom.h"
#include "ordersdetail.h"

namespace Ui {
class ClientCore;
}

class ClientCore : public QMainWindow
{
    Q_OBJECT

public:
    // 定义页面类型枚举
    enum PageType {
        PAGE_LOGIN,      // 登录页面
        PAGE_REGISTER,   // 注册页面
        PAGE_MAIN        // 主界面
    };

    explicit ClientCore(QWidget *parent = nullptr);
    ~ClientCore();

    // 发送注册请求
    void sendRegisterRequest(const QString &username, const QString &password);

public slots:
    // 切换到指定页面
    void switchToPage(PageType pageType);

private slots:
    // 处理来自子页面的信号
    void onLoginSuccess(const QString &username,const QString &password);
    void onRegisterRequest();
    void onRegisterSuccess();
    void onLogout();
    void onChatroomShow();

private:
    Ui::ClientCore *ui;

    // 页面实例
    Widget *wid;            // 登录页面
    Register *reg;          // 注册页面
    MainWindow_main *m_main; // 主界面
    ChatRoom *chat;
    QStackedWidget *qsw;    // 堆栈窗口部件

    ordersdetail *ordersender;

    // 页面映射表，用于方便页面切换
    QMap<PageType, QWidget*> pages;
    QByteArray receiver;
    QString user_name;

    // TCP Socket
    Session *m_session;

    //rtmp
    CameraStreamer* m_cameraStreamer;
    VideoPlayer* m_videoPlayer;

    // 初始化所有页面
    void initializePages();

    // 连接页面信号
    void connectPageSignals();

    // 初始化网络连接
    Session* initializeNetwork();
};

#endif // CLIENTCORE_H


