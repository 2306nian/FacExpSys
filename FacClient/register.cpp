#include "register.h"
#include "ui_register.h"
#include "clientcore.h"  // 添加ClientCore头文件
#include <qstring.h>
#include <QDebug>

Register::Register(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Register)
    , m_core(nullptr)
{
    ui->setupUi(this);

    // 如果UI中有返回按钮，连接它的点击信号
    // 假设有这个按钮，如果没有可以删除此代码
    // connect(ui->pushButton_back, &QPushButton::clicked, this, &Register::on_pushButton_back_clicked);
}

Register::~Register()
{
    delete ui;
}

void Register::setClientCore(ClientCore *core)
{
    m_core = core;
}

void Register::on_pushButton_register_clicked()
{
    QString s_a = ui->lineEdit_account->text();
    QString s_p = ui->lineEdit_password->text();

    if(!s_a.isEmpty() && !s_p.isEmpty()){
        // 使用ClientCore发送注册请求
        if (m_core) {
            m_core->sendRegisterRequest(s_a, s_p);
        }

        // 发出注册成功信号
        emit registerSuccess();
    }
}

// 如果有返回按钮，实现对应的槽函数
void Register::on_pushButton_back_clicked()
{
    emit backToLogin();
}



