#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->pushButton_login->setEnabled(false);
    connect(ui->lineEdit_account, &QLineEdit::textChanged, this, &Widget::checkInputs);
    connect(ui->lineEdit_password, &QLineEdit::textChanged, this, &Widget::checkInputs);
}

void Widget::checkInputs()
{
    QString s1 = ui->lineEdit_account->text();
    QString s2 = ui->lineEdit_password->text();

    ui->pushButton_login->setEnabled(!s1.isEmpty() && !s2.isEmpty());
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_login_clicked()
{
    QString username = ui->lineEdit_account->text();
    QString password= ui->lineEdit_password->text();
    // 发出登录成功信号
    emit loginSuccess(username,password);
}

void Widget::on_pushButton_more_clicked()
{
    // 发出注册请求信号
    emit registerRequest();
}



void Widget::on_toolButton_close_clicked()
{

}

