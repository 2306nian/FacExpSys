#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

// 不再直接包含MainWindow_main和Register
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

signals:
    // 添加与ClientCore交互所需的基本信号
    void loginSuccess(const QString &username,const QString &password);      // 登录成功信号
    void registerRequest();   // 请求注册信号

private slots:
    void on_pushButton_login_clicked();
    void checkInputs();
    void on_pushButton_more_clicked();

    void on_toolButton_close_clicked();

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H

