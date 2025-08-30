#include "MainWindow_main.h"
#include "ui_MainWindow_main.h"
#include "pageorder.h"
#include "page_mine.h"
#include "pagedevice.h"

MainWindow_main::MainWindow_main(QMainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 创建子页面
    po1 = new PageOrder(this);
    pm1 = new Page_Mine(this);
    pd1 = new PageDevice(this);

    // 添加到堆栈窗口部件
    ui->stackedWidget->addWidget(po1);
    ui->stackedWidget->addWidget(pm1);
    ui->stackedWidget->addWidget(pd1);

    // 默认显示订单页面
    ui->stackedWidget->setCurrentWidget(po1);

    // 如果UI中有logoutButton，连接它的点击信号
    // 假设有这个按钮，如果没有可以删除此代码
    // connect(ui->logoutButton, &QPushButton::clicked, this, &MainWindow_main::on_logoutButton_clicked);
}

MainWindow_main::~MainWindow_main()
{
    delete ui;
}

void MainWindow_main::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(po1);
}

void MainWindow_main::on_pushButton_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(pd1);
}

void MainWindow_main::on_pushButton_3_clicked()
{
    ui->stackedWidget->setCurrentWidget(pm1);  // 注意：原代码这里设置为pd1，可能是错误
}




