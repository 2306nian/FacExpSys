#ifndef REGISTER_H
#define REGISTER_H

#include <QWidget>

namespace Ui {
class Register;
}

// 前向声明ClientCore
class ClientCore;

class Register : public QWidget
{
    Q_OBJECT

public:
    explicit Register(QWidget *parent = nullptr);
    ~Register();

    // 设置ClientCore引用
    void setClientCore(ClientCore *core);

signals:
    // 添加与ClientCore交互所需的基本信号
    void registerSuccess();    // 注册成功信号
    void backToLogin();        // 返回登录页面信号

private slots:
    void on_pushButton_register_clicked();

    // 如果有返回按钮，添加对应的槽
    void on_pushButton_back_clicked();

private:
    Ui::Register *ui;
    ClientCore *m_core;  // 持有ClientCore的引用
};

#endif // REGISTER_H

