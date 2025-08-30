#ifndef CHATROOM_H
#define CHATROOM_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDateTime>

// 自定义代理类用于绘制消息气泡
class MessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MessageDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

namespace Ui {
class ChatRoom;
}

class ChatRoom : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatRoom(QWidget *parent = nullptr);
    ~ChatRoom();

public slots:
    // 添加消息到聊天区域
    void addMessage(const QString &text, bool isSent, const QString &sender = QString());

private slots:
    // 发送按钮点击事件
    void on_sendButton_clicked();

    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::ChatRoom *ui;
    QStandardItemModel *m_messageModel;
    QString m_username;  // 当前用户名

    // 初始化UI
    void initUI();

    // 初始化信号槽连接
    void setupConnections();

    // 格式化时间
    QString formatTime(const QDateTime &time) const;
};

#endif // CHATROOM_H
