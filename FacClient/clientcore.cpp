#include "clientcore.h"
#include "ui_clientcore.h"
#include <QDebug>
#include <QJsonDocument>

ClientCore::ClientCore(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ClientCore)
    , wid(nullptr)
    , reg(nullptr)
    , m_main(nullptr)
    , tcp(nullptr)
{
    ui->setupUi(this);

    // 初始化网络连接
    initializeNetwork();

    // 创建堆栈部件作为中央窗口部件
    qsw = new QStackedWidget(this);
    setCentralWidget(qsw);

    // 初始化所有页面
    initializePages();

    // 连接页面信号
    connectPageSignals();

    // 默认显示登录页面
    switchToPage(PAGE_LOGIN);

    // 设置窗口标题和大小
    setWindowTitle("客户端");
    resize(800, 600);
}

ClientCore::~ClientCore()
{
    delete ui;
    // 不需要显式删除页面，因为它们作为子部件会被自动删除
}

void ClientCore::initializeNetwork()
{
    tcp = new QTcpSocket(this);
}

void ClientCore::sendRegisterRequest(const QString &username, const QString &password)
{
    tcp->connectToHost("127.0.0.1", 8888);
    if(tcp->state() == QAbstractSocket::ConnectedState){
        qDebug() << "连接成功";
    }

    if(!username.isEmpty() && !password.isEmpty()){
        QJsonObject json;
        json["type"] = "register";
        QJsonObject Qdata;
        Qdata["username"] = username;
        Qdata["password"] = password;
        json["data"] = Qdata;
        QByteArray sender = QJsonDocument(json).toJson(QJsonDocument::Compact);
        tcp->write(packMessage(sender));
    }
}

void ClientCore::initializePages()
{
    // 创建登录页面
    wid = new Widget();
    qsw->addWidget(wid);
    pages[PAGE_LOGIN] = wid;

    // 创建注册页面
    reg = new Register();
    reg->setClientCore(this);  // 设置ClientCore引用
    qsw->addWidget(reg);
    pages[PAGE_REGISTER] = reg;

    // 创建主界面
    m_main = new MainWindow_main();
    qsw->addWidget(m_main);
    pages[PAGE_MAIN] = m_main;
}


void ClientCore::connectPageSignals()
{
    // 连接登录页面信号
    connect(wid, &Widget::loginSuccess, this, &ClientCore::onLoginSuccess);
    connect(wid, &Widget::registerRequest, this, &ClientCore::onRegisterRequest);

    //连接注册页面信号
    connect(reg, &Register::registerSuccess, this, &ClientCore::onRegisterSuccess);
    connect(reg, &Register::backToLogin, this, [this]() {
        switchToPage(PAGE_LOGIN);
    });

    // 连接主界面信号
    connect(m_main, &MainWindow_main::logout, this, &ClientCore::onLogout);
}

void ClientCore::switchToPage(PageType pageType)
{
    if (pages.contains(pageType)) {
        qDebug() << "切换到页面:" << pageType;
        qsw->setCurrentWidget(pages[pageType]);
    } else {
        qDebug() << "警告: 尝试切换到未定义的页面:" << pageType;
    }
}

// 处理来自子页面的信号
void ClientCore::onLoginSuccess()
{
    qDebug() << "登录成功，切换到主界面";
    switchToPage(PAGE_MAIN);
}

void ClientCore::onRegisterRequest()
{
    qDebug() << "请求注册，切换到注册页面";
    switchToPage(PAGE_REGISTER);
}

void ClientCore::onRegisterSuccess()
{
    qDebug() << "注册成功，切换到登录页面";
    switchToPage(PAGE_LOGIN);
}

void ClientCore::onLogout()
{
    qDebug() << "用户登出，切换到登录页面";
    switchToPage(PAGE_LOGIN);
}

