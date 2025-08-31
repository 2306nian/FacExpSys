#include "chatroom.h"
#include "ui_chatroom.h"
#include"handlers.h"
#include <QStandardItem>
#include <QVariant>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextOption>
#include<QJsonObject>
#include <QJsonDocument>

ChatRoom::ChatRoom(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatRoom)
{
    ui->setupUi(this);

    // 设置窗口标题
    setWindowTitle("聊天室");

    connect(MessageHandler::instance(),&MessageHandler::sendMessageToChat,this,&ChatRoom::messageData);
    connect(g_session,&Session::textUpdate,this,&ChatRoom::messageUpdate);
    // 初始化model，并绑定到listView
    model = new QStandardItemModel(this);
    ui->listView->setModel(model);

    // 设置自定义代理
    messageDelegate = new MessageDelegate(this);
    ui->listView->setItemDelegate(messageDelegate);

    // 设置listView属性
    ui->listView->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listView->setSpacing(10);  // 设置消息间距

    // 设置textEdit属性
    ui->textEdit_chat->setPlaceholderText("请输入消息...");

    // 设置图标按钮工具提示
    ui->toolButton_file->setToolTip("文件传输");
    ui->toolButton_video->setToolTip("视频通话");
    ui->toolButton_share->setToolTip("屏幕共享");
    ui->pushButton_emission->setToolTip("发送消息");

    // 连接信号和槽
    connect(ui->pushButton_emission, &QPushButton::clicked,
            this, &ChatRoom::on_pushButton_emission_clicked);

    // 监听Enter键发送消息
    connect(ui->textEdit_chat, &QTextEdit::textChanged, [this]() {
        if (ui->textEdit_chat->document()->toPlainText().contains("\n")) {
            QString text = ui->textEdit_chat->toPlainText().trimmed();
            ui->textEdit_chat->clear();
            if (!text.isEmpty()) {
                appendMessage(text.replace("\n", ""), true);
            }
        }
    });
}

void ChatRoom::appendMessage(const QString &text, bool isSelf)
{
    QStandardItem *item = new QStandardItem();

    // 创建消息对象
    ChatMessage msg;
    msg.content = text;
    msg.isSelf = isSelf;
    msg.timestamp = QDateTime::currentDateTime();

    // 将消息对象存储在item的数据中
    QVariant v;
    v.setValue(msg);
    item->setData(v, Qt::UserRole + 1);

    // 添加到模型
    model->appendRow(item);

    // 滚动到底部
    ui->listView->scrollToBottom();
}

ChatRoom::~ChatRoom()
{
    delete ui;
}

void ChatRoom::messageData(QString s1){
    receivedMsg=s1;
}

void ChatRoom::on_pushButton_emission_clicked()
{
    QString text = ui->textEdit_chat->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    QJsonObject json_message{
        {"type","text_msg"},
        {"data",QJsonObject{
                {"message",text},
            }}
    };
    // 先将QJsonObject转换为QJsonDocument
    g_session->sendMessage(QJsonDocument(json_message).toJson(QJsonDocument::Compact));
    // 添加自己的消息
    appendMessage(text, true);

    // 清空输入框
    ui->textEdit_chat->clear();

    // 模拟回复
    QTimer::singleShot(1000, this, [this, text]() {
        // 简单的回复逻辑
        if (text.contains("你好") || text.contains("hello")) {
            appendMessage("你好！有什么可以帮助你的吗？", false);
        } else if (text.contains("时间") || text.contains("几点")) {
            appendMessage("现在时间是: " + QDateTime::currentDateTime().toString("hh:mm:ss"), false);
        } else {
            appendMessage("收到你的消息: " + text, false);
        }
    });
}

// MessageDelegate的实现
void MessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    painter->save();

    // 获取消息数据
    QVariant var = index.data(Qt::UserRole + 1);
    ChatMessage msg = var.value<ChatMessage>();

    // 获取视图矩形
    QRect rect = option.rect;

    // 绘制时间戳（每5条消息显示一次时间）
    static QDateTime lastTimestamp;
    bool showTimestamp = false;

    if (!lastTimestamp.isValid() ||
        lastTimestamp.secsTo(msg.timestamp) > 300 || // 5分钟显示一次
        (index.row() % 5 == 0)) {  // 或者每5条消息显示一次
        showTimestamp = true;
        lastTimestamp = msg.timestamp;
    }

    if (showTimestamp) {
        QString timeStr = msg.timestamp.toString("yyyy-MM-dd hh:mm:ss");
        QFontMetrics fm(option.font);
        int timeWidth = fm.horizontalAdvance(timeStr);
        int timeHeight = fm.height();

        // 画时间背景
        QRect timeRect(rect.left() + (rect.width() - timeWidth) / 2 - 10,
                       rect.top() + 5,
                       timeWidth + 20,
                       timeHeight + 6);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(200, 200, 200, 120));
        painter->drawRoundedRect(timeRect, 10, 10);

        // 画时间文字
        painter->setPen(Qt::black);
        painter->drawText(timeRect, Qt::AlignCenter, timeStr);

        // 调整消息显示位置
        rect.setTop(rect.top() + timeHeight + 15);
    }

    // 气泡最大宽度为整个区域的70%
    int maxBubbleWidth = rect.width() * 0.7;

    // 计算文本大小
    QTextDocument doc;
    doc.setDefaultFont(option.font);
    doc.setTextWidth(maxBubbleWidth - 30);  // 30是文本内边距
    doc.setHtml(Qt::convertFromPlainText(msg.content));

    QSize docSize(doc.idealWidth(), doc.size().height());

    // 气泡大小和位置
    int bubbleWidth = docSize.width() + 30;  // 加上内边距
    int bubbleHeight = docSize.height() + 20;

    QRect bubbleRect;
    if (msg.isSelf) {  // 自己的消息靠右
        bubbleRect = QRect(rect.right() - bubbleWidth - 20,
                           rect.top() + 5,
                           bubbleWidth,
                           bubbleHeight);
    } else {  // 对方消息靠左
        bubbleRect = QRect(rect.left() + 20,
                           rect.top() + 5,
                           bubbleWidth,
                           bubbleHeight);
    }

    // 画头像（简化为圆形）
    int avatarSize = 40;
    QRect avatarRect;
    if (msg.isSelf) {
        avatarRect = QRect(rect.right() - 10 - avatarSize,
                           bubbleRect.top(),
                           avatarSize,
                           avatarSize);
    } else {
        avatarRect = QRect(rect.left() + 10,
                           bubbleRect.top(),
                           avatarSize,
                           avatarSize);
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(msg.isSelf ? QColor(50, 200, 100) : QColor(200, 200, 200));
    painter->drawEllipse(avatarRect);

    // 画气泡
    painter->setPen(Qt::NoPen);
    painter->setBrush(msg.isSelf ? QColor(95, 233, 152) : QColor(245, 245, 245));  // 绿色/白色气泡
    painter->drawRoundedRect(bubbleRect, 10, 10);

    // 画文本
    painter->translate(msg.isSelf ? bubbleRect.left() + 15 : bubbleRect.left() + 15,
                       bubbleRect.top() + 10);
    QRect clipRect(0, 0, bubbleRect.width() - 30, bubbleRect.height() - 20);
    painter->setClipRect(clipRect);

    doc.drawContents(painter);

    painter->restore();
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (!index.isValid())
        return QSize();

    // 获取消息数据
    QVariant var = index.data(Qt::UserRole + 1);
    ChatMessage msg = var.value<ChatMessage>();

    // 计算文本大小
    QTextDocument doc;
    doc.setDefaultFont(option.font);

    // 气泡最大宽度为整个区域的70%
    int maxBubbleWidth = option.rect.width() * 0.7;
    doc.setTextWidth(maxBubbleWidth - 30);
    doc.setHtml(Qt::convertFromPlainText(msg.content));

    QSize docSize(doc.idealWidth(), doc.size().height());

    // 添加足够的高度来容纳气泡、头像和可能的时间戳
    int height = docSize.height() + 40;  // 基本高度

    // 每5条消息显示一次时间，需要额外高度
    if (index.row() % 5 == 0) {
        height += 30;
    }

    return QSize(option.rect.width(), height);
}


void ChatRoom::messageUpdate(){
    appendMessage(receivedMsg,false);
}

// 必须在CPP文件中注册自定义类型，以便QVariant能使用
void ChatRoom::on_toolButton_clicked()
{
    this->close();
}


Q_DECLARE_METATYPE(ChatMessage)

