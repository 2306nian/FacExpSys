#include "chatroom.h"
#include "ui_chatroom.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QScrollBar>
#include <QKeyEvent>
#include <QDebug>

// MessageDelegate 实现
MessageDelegate::MessageDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void MessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    // 获取消息数据
    bool isSent = index.data(Qt::UserRole).toBool();
    QString sender = index.data(Qt::UserRole + 1).toString();
    QString time = index.data(Qt::UserRole + 2).toString();
    QString text = index.data(Qt::DisplayRole).toString();

    // 计算绘制区域
    QRect contentRect = option.rect.adjusted(10, 10, -10, -10);

    painter->save();

    // 绘制发送者和时间
    QFont nameFont = painter->font();
    nameFont.setBold(true);
    painter->setFont(nameFont);

    QRect nameRect = contentRect.adjusted(0, 0, 0, -contentRect.height() + 20);

    // 根据消息类型确定气泡位置和颜色
    QColor bubbleColor = isSent ? QColor(149, 236, 105) : QColor(255, 255, 255);
    QRect bubbleRect;

    if (isSent) {
        // 发送的消息靠右
        nameRect.moveLeft(contentRect.right() - nameRect.width() - 80);
        painter->drawText(nameRect, Qt::AlignRight, sender);

        // 时间在用户名左侧
        QRect timeRect = nameRect;
        timeRect.moveLeft(nameRect.left() - 80);
        painter->setFont(option.font);
        painter->setPen(Qt::gray);
        painter->drawText(timeRect, Qt::AlignLeft, time);

        // 气泡靠右
        QFontMetrics fm(option.font);
        int textWidth = qMin(fm.horizontalAdvance(text) + 30, contentRect.width() - 60);
        bubbleRect = QRect(contentRect.right() - textWidth - 5,
                           nameRect.bottom() + 5,
                           textWidth,
                           contentRect.height() - nameRect.height() - 10);
    } else {
        // 接收的消息靠左
        nameRect.moveLeft(contentRect.left());
        painter->drawText(nameRect, Qt::AlignLeft, sender);

        // 时间在用户名右侧
        QRect timeRect = nameRect;
        timeRect.moveLeft(nameRect.right() + 10);
        painter->setFont(option.font);
        painter->setPen(Qt::gray);
        painter->drawText(timeRect, Qt::AlignLeft, time);

        // 气泡靠左
        QFontMetrics fm(option.font);
        int textWidth = qMin(fm.horizontalAdvance(text) + 30, contentRect.width() - 60);
        bubbleRect = QRect(contentRect.left() + 5,
                           nameRect.bottom() + 5,
                           textWidth,
                           contentRect.height() - nameRect.height() - 10);
    }

    // 绘制气泡
    painter->setPen(Qt::NoPen);
    painter->setBrush(bubbleColor);
    painter->drawRoundedRect(bubbleRect, 10, 10);

    // 绘制文本
    painter->setPen(Qt::black);
    painter->drawText(bubbleRect.adjusted(10, 5, -10, -5),
                      Qt::AlignLeft | Qt::TextWordWrap,
                      text);

    painter->restore();
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QFontMetrics fm(option.font);

    // 计算文本占用的空间
    QRect textRect = fm.boundingRect(
        QRect(0, 0, option.rect.width() - 100, 1000),
        Qt::TextWordWrap,
        text
        );

    // 返回所需高度（标题栏 + 气泡 + 间距）
    return QSize(option.rect.width(), textRect.height() + 60);
}

// ChatRoom 实现
ChatRoom::ChatRoom(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatRoom)
    , m_username("我")  // 默认用户名
{
    ui->setupUi(this);
    initUI();
    setupConnections();

    // 设置窗口标题
    setWindowTitle("聊天室");
}

ChatRoom::~ChatRoom()
{
    delete ui;
}

void ChatRoom::initUI()
{
    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 添加聊天对象标题
    QLabel *titleLabel = new QLabel("聊天对象", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("background-color: #F5F5F5; padding: 10px; font-weight: bold;");
    titleLabel->setMinimumHeight(40);
    mainLayout->addWidget(titleLabel);

    // 添加分隔线
    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line1);

    // 创建消息列表视图
    QListView *messageListView = new QListView(this);
    messageListView->setObjectName("messageListView");
    messageListView->setStyleSheet(
        "QListView#messageListView {"
        "  background-color: #F5F5F5;"
        "  border: none;"
        "}"
        );
    messageListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    messageListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    messageListView->setSelectionMode(QAbstractItemView::NoSelection);

    // 创建并设置模型和代理
    m_messageModel = new QStandardItemModel(this);
    messageListView->setModel(m_messageModel);
    messageListView->setItemDelegate(new MessageDelegate(this));

    mainLayout->addWidget(messageListView, 1);  // 1是伸展系数

    // 添加分隔线
    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line2);

    // 创建输入区域
    QWidget *inputWidget = new QWidget(this);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(10, 10, 10, 10);

    // 创建文本输入框
    QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setObjectName("textEdit");
    textEdit->setStyleSheet(
        "QTextEdit#textEdit {"
        "  border: 1px solid #E0E0E0;"
        "  border-radius: 4px;"
        "  padding: 5px;"
        "}"
        );
    textEdit->setMinimumHeight(60);
    textEdit->setMaximumHeight(120);
    textEdit->setPlaceholderText("请输入消息...");
    textEdit->installEventFilter(this);  // 安装事件过滤器捕获回车键

    // 创建发送按钮
    QPushButton *sendButton = new QPushButton("发送", this);
    sendButton->setObjectName("sendButton");
    sendButton->setStyleSheet(
        "QPushButton#sendButton {"
        "  background-color: #07C160;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 5px 15px;"
        "  min-width: 80px;"
        "}"
        "QPushButton#sendButton:hover {"
        "  background-color: #06AD56;"
        "}"
        );

    inputLayout->addWidget(textEdit, 1);  // 1是伸展系数
    inputLayout->addWidget(sendButton);

    mainLayout->addWidget(inputWidget, 0);  // 0是伸展系数

    // 存储控件引用到UI以便后续访问
    ui->messageListView = messageListView;
    ui->textEdit = textEdit;
    ui->sendButton = sendButton;

    // 设置初始大小
    resize(450, 650);
}

void ChatRoom::setupConnections()
{
    // 连接发送按钮点击信号
    connect(ui->sendButton, &QPushButton::clicked, this, &ChatRoom::on_sendButton_clicked);

    // 连接文本输入框的回车键处理（在事件过滤器中实现）
}

bool ChatRoom::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            // 回车发送，Shift+回车换行
            on_sendButton_clicked();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void ChatRoom::on_sendButton_clicked()
{
    QString text = ui->textEdit->toPlainText().trimmed();
    if (!text.isEmpty()) {
        addMessage(text, true, m_username);
        ui->textEdit->clear();
    }

    // 将焦点返回到输入框
    ui->textEdit->setFocus();
}

void ChatRoom::addMessage(const QString &text, bool isSent, const QString &sender)
{
    QStandardItem *item = new QStandardItem(text);

    // 存储消息属性
    item->setData(isSent, Qt::UserRole);  // 是否为发送的消息
    item->setData(sender.isEmpty() ? (isSent ? m_username : "对方") : sender, Qt::UserRole + 1);  // 发送者
    item->setData(formatTime(QDateTime::currentDateTime()), Qt::UserRole + 2);  // 时间

    m_messageModel->appendRow(item);

    // 滚动到底部
    ui->messageListView->scrollToBottom();
}

QString ChatRoom::formatTime(const QDateTime &time) const
{
    return time.toString("HH:mm:ss");
}
