#ifndef CHATROOM_H
#define CHATROOM_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDateTime>
#include <QTimer>

namespace Ui {
class ChatRoom;
}

// 消息结构体
struct ChatMessage {
    QString content;      // 消息内容
    bool isSelf;          // 是否是自己发送的
    QDateTime timestamp;  // 时间戳
};

// 自定义委托来绘制微信风格的气泡
class MessageDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    MessageDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

class ChatRoom : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatRoom(QWidget *parent = nullptr);
    ~ChatRoom();

private slots:
    void on_pushButton_emission_clicked();

    void on_toolButton_clicked();

private:
    Ui::ChatRoom *ui;
    QStandardItemModel *model;
    MessageDelegate *messageDelegate;

    void appendMessage(const QString &text, bool isSelf);
};

#endif // CHATROOM_H


