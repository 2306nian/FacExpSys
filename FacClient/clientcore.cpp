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
{
    ui->setupUi(this);

    m_session = initializeNetwork();

    // // RTMP测试
    // m_cameraStreamer = new CameraStreamer(m_session, true);

    // // 创建简单的视频窗口
    // QWidget* videoWindow = new QWidget();
    // videoWindow->setWindowTitle("视频播放");
    // videoWindow->resize(800, 600);

    // VideoPlayer* videoPlayer = new VideoPlayer(100, videoWindow);

    // QVBoxLayout* layout = new QVBoxLayout(videoWindow);
    // layout->addWidget(videoPlayer);

    // videoWindow->show();

    // // 启动推流和拉流
    // m_cameraStreamer->startStreaming("rtmp://localhost/live/stream123");
    // // videoPlayer->playStream("rtmp://localhost/live/stream123");
    // 创建堆栈部件作为中央窗口部件
    qsw = new QStackedWidget(this);
    setCentralWidget(qsw);

    // 初始化所有页面
    initializePages();

    // 连接页面信号
    connectPageSignals();

    // 默认显示登录页面
    switchToPage(PAGE_LOGIN);
    this->resize(300,1000);

    // 设置窗口标题和大小
    setWindowTitle("客户端");

    connect(m_session, &Session::loginResult, this, [this](bool result){
        if(result){
            switchToPage(PAGE_MAIN);
            resize(1300,900);
        }else{
            //TODO:登陆失败处理
        }
    });
    connect(m_session, &Session::registerResult, this, [](bool result){
        if (result){
            //TODO：注册成功处理
        }else{
            //TODO:注册失败处理
        }
    });
}

ClientCore::~ClientCore()
{
    delete ui;
    // 不需要显式删除页面，因为它们作为子部件会被自动删除
}

void ClientCore::onChatroomShow(){
    chat = new ChatRoom(this);
    chat->show();
}

Session* ClientCore::initializeNetwork()
{
    QTcpSocket* tcp = new QTcpSocket(this);

    // 添加连接超时处理
    QTimer* connectionTimer = new QTimer(this);
    connectionTimer->setSingleShot(true);
    connect(connectionTimer, &QTimer::timeout, this, [tcp]() {
        if (tcp->state() != QAbstractSocket::ConnectedState) {
            qDebug() << "连接超时！尝试重新连接...";
            tcp->abort();
            tcp->connectToHost("127.0.0.1", 8888);
        }
    });

    connect(tcp, &QTcpSocket::connected, [connectionTimer](){
        qDebug() << "连接成功！";
        connectionTimer->stop();
    });

    connect(tcp, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            [tcp, this](QAbstractSocket::SocketError error){
                qDebug() << "连接失败，错误代码：" << error;
                qDebug() << "错误信息：" << tcp->errorString();

                // 根据错误类型决定是否重连
                if (error == QAbstractSocket::ConnectionRefusedError ||
                    error == QAbstractSocket::NetworkError) {
                    qDebug() << "尝试5秒后重新连接...";
                    QTimer::singleShot(5000, this, [tcp]() {
                        tcp->connectToHost("127.0.0.1", 8888);
                    });
                }
            });

    tcp->connectToHost("127.0.0.1", 8888);
    connectionTimer->start(5000); // 5秒超时
    Session* session = new Session(tcp);
    g_session = session;
    return g_session;
}


void ClientCore::sendRegisterRequest(const QString &username, const QString &password)
{
    qDebug()<<"注册成功";
    if(!username.isEmpty() && !password.isEmpty()){
        QJsonObject json;
        json["type"] = "register";
        QJsonObject Qdata;
        Qdata["username"] = username;
        Qdata["password"] = password;
        Qdata["user_type"]="client";
        json["data"] = Qdata;
        QByteArray sender = QJsonDocument(json).toJson(QJsonDocument::Compact);
        m_session->sendMessage(sender);
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
    connect(g_session,&Session::createChatRoom,this, &ClientCore::onChatroomShow);
    connect(wid, &Widget::loginSuccess, this, &ClientCore::onLoginSuccess);
    connect(wid, &Widget::registerRequest, this, &ClientCore::onRegisterRequest);
    connect(ordersender,&ordersdetail::createChatByOrder,this,&ClientCore::onChatroomShow);
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
void ClientCore::onLoginSuccess(const QString &username,const QString &password)
{
    g_username=username;
    if(!username.isEmpty() && !password.isEmpty()){
        QJsonObject json;
        json["type"] = "login";
        QJsonObject Qdata;
        Qdata["username"] = username;
        Qdata["password"] = password;
        json["data"] = Qdata;
        QByteArray sender = QJsonDocument(json).toJson(QJsonDocument::Compact);
        m_session->sendMessage(sender);
        qDebug()<<username<<password;
    }
}

void ClientCore::onRegisterRequest()
{
    qDebug() << "请求注册，切换到注册页面";
    switchToPage(PAGE_REGISTER);
    resize(300,400);
}

void ClientCore::onRegisterSuccess()
{
    qDebug() << "请求返回登录";
    switchToPage(PAGE_LOGIN);
    resize(300,400);
}

void ClientCore::onLogout()
{
    qDebug() << "用户登出，切换到登录页面";
    switchToPage(PAGE_LOGIN);
    resize(480,672);
}

