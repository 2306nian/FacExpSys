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
#include <QFileDialog>
#include<QDir>
#include<filesender.h>
#include<session.h>

ChatRoom::ChatRoom(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatRoom),
    m_recorder(new ScreenRecorder(this)),
    m_timer(new QTimer(this)),
    m_recordingTime(0),
    m_cameraStreamer(nullptr)
{
    ui->setupUi(this);
    fsender=new FileSender(this);
    freceiver=new FileReceiver(this);
    // 设置窗口标题
    setWindowTitle("聊天室");
    // 连接信号
    connect(m_recorder, &ScreenRecorder::recordingStarted,
            this, &ChatRoom::onRecordingStarted);
    connect(m_recorder, &ScreenRecorder::recordingStopped,
            this, &ChatRoom::onRecordingStopped);
    connect(m_recorder, &ScreenRecorder::recordingError,
            this, &ChatRoom::onRecordingError);

    connect(m_timer, &QTimer::timeout, this, &ChatRoom::updateRecordingTime);
    connect(MessageHandler::instance(),&MessageHandler::sendMessageToChat,this,&ChatRoom::messageData);
    connect(g_session,&Session::textUpdate,this,&ChatRoom::messageUpdate);
    connect(FileHandler::instance(),&FileHandler::sendFileidToChat,this,&ChatRoom::getFileidFromhandle);
    connect(FileHandler::instance(),&FileHandler::startFileUploadInChat,this,&ChatRoom::startUploadInChat);
    connect(g_session,&Session::fileInfoSend,this,&ChatRoom::getFileInfo);
    connect(FileHandler::instance(),&FileHandler::getfileId,this,&ChatRoom::getFileId);
    connect(FileHandler::instance(),&FileHandler::ChatDownloadStart,this,&ChatRoom::startDownload);
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
    ui->listView->viewport()->installEventFilter(this);
}

void ChatRoom::startDownload(const QJsonObject &json){
    freceiver->onSessionMessage(g_session,json);
}

void ChatRoom::getFileId(QString fileId,QJsonObject &jobj){
    this->fileId=fileId;
    fsender->onSessionMessage(g_session,jobj);
    qDebug()<<fileId;
}

void ChatRoom::getFileInfo(const QJsonObject &data){
    fileName=data["file_name"].toString();
    file_size=data["file_size"].toString();
}

void ChatRoom::startUploadInChat(const QJsonObject &data){
}

void ChatRoom::getFileidFromhandle(QString s1,QString f_name,qint64 f_size)
{
    fileId = s1;
    qDebug() << "收到文件ID: " << fileId;
    // 将当前上传的文件名与fileId关联起来
    fileIdMap[fileName] = fileId;
    qDebug() << "已将文件 " << fileName << " 与ID " << fileId << " 关联";
    appendFileMessage(f_name,f_size,true,s1);
}


bool ChatRoom::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->listView->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint pos = mouseEvent->pos();

        QModelIndex index = ui->listView->indexAt(pos);
        if (index.isValid()) {
            // 转换坐标到项的局部坐标系
            QRect rect = ui->listView->visualRect(index);
            QPoint itemPos = mouseEvent->pos() - rect.topLeft();

            // 检查点击是否在下载按钮区域内
            QMap<QModelIndex, QRect> areas = messageDelegate->getClickableAreas();
            if (areas.contains(index)) {
                QRect btnRect = areas[index];
                if (btnRect.contains(itemPos)) {
                    // 获取消息数据
                    QVariant var = index.data(Qt::UserRole + 1);
                    ChatMessage msg = var.value<ChatMessage>();

                    if (msg.isFileMessage) {
                        // 如果消息的fileId为空，尝试从映射中查找
                        if (msg.fileId.isEmpty() && fileIdMap.contains(msg.fileName)) {
                            // 更新消息中的fileId
                            msg.fileId = fileIdMap[msg.fileName];

                            // 将更新后的消息存回模型（可选，如果您想永久更新）
                            QVariant updatedVar;
                            updatedVar.setValue(msg);
                            model->setData(index, updatedVar, Qt::UserRole + 1);
                        }
                        QString dir = QFileDialog::getExistingDirectory(this,
                                                                        tr("选择保存目录"),
                                                                        QDir::homePath(),
                                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

                        if (!dir.isEmpty()) {
                            // 用户选择了目录
                            qDebug() << "选择的保存目录:" << dir;
                            // 使用该目录保存文件
                            // 例如: QString filePath = dir + "/myfile.txt";
                        }

                        // 现在调用下载处理
                        freceiver->startFileDownload(g_session,msg.fileName,msg.fileId,dir);
                        qDebug() << "下载按钮被点击，文件名:" << msg.fileName
                                 << "文件ID:" << msg.fileId;
                        return true; // 事件已处理
                    }
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
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
    qDebug()<<g_session->getTicketId();
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
        // if (text.contains("你好") || text.contains("hello")) {
        //     appendMessage("你好！有什么可以帮助你的吗？", false);
        // } else if (text.contains("时间") || text.contains("几点")) {
        //     appendMessage("现在时间是: " + QDateTime::currentDateTime().toString("hh:mm:ss"), false);
        // } else {
        //     appendMessage("收到你的消息: " + text, false);
        // }
    });
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    if (!index.isValid())
        return QSize(0, 0);

    // 获取消息数据
    QVariant var = index.data(Qt::UserRole + 1);
    ChatMessage msg = var.value<ChatMessage>();

    // 确定气泡最大宽度（屏幕宽度的70%）
    int maxWidth = option.rect.width() * 0.4;

    // 计算时间戳额外高度
    int timestampHeight = 0;
    if (index.row() == 0 || index.row() % 5 == 0) { // 每5条消息显示一次时间
        timestampHeight = option.fontMetrics.height() + 15; // 时间文本高度+间距
    }

    if (msg.isFileMessage) {
        // 文件消息固定高度
        return QSize(option.rect.width(), 85 + timestampHeight);
    } else {
        // 文本消息动态计算高度
        QTextDocument doc;
        doc.setDefaultFont(option.font);
        doc.setTextWidth(maxWidth - 30); // 减去padding
        doc.setHtml(Qt::convertFromPlainText(msg.content));

        int height = doc.size().height() + 20; // 文本高度 + padding
        return QSize(option.rect.width(), height + timestampHeight);
    }
}

void MessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 获取消息数据
    QVariant var = index.data(Qt::UserRole + 1);
    ChatMessage msg = var.value<ChatMessage>();

    // 设置区域
    QRect rect = option.rect;
    const int avatarSize = 40;
    const int avatarMargin = 10;
    const int bubblePadding = 12;

    // 计算时间戳
    bool showTimestamp = (index.row() == 0 || index.row() % 5 == 0);
    int timestampHeight = 0;

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

        timestampHeight = timeHeight + 15;
        rect.setTop(rect.top() + timestampHeight);
    }

    // 计算气泡最大宽度为整个区域的70%
    int maxBubbleWidth = rect.width() * 0.4;

    // 计算头像位置
    QRect avatarRect;
    if (msg.isSelf) {
        avatarRect = QRect(rect.right() - avatarSize - avatarMargin,
                           rect.top(), avatarSize, avatarSize);
    } else {
        avatarRect = QRect(rect.left() + avatarMargin,
                           rect.top(), avatarSize, avatarSize);
    }

    // 绘制头像
    painter->setPen(Qt::NoPen);
    painter->setBrush(msg.isSelf ? QColor(0, 150, 136) : QColor(158, 158, 158));
    painter->drawEllipse(avatarRect);

    // 计算气泡尺寸和位置
    QRect bubbleRect;

    if (msg.isFileMessage) {
        // 文件消息气泡固定尺寸
        int bubbleWidth = 280;
        int bubbleHeight = 70;

        if (msg.isSelf) {
            bubbleRect = QRect(avatarRect.left() - bubbleWidth - 10,
                               avatarRect.top(), bubbleWidth, bubbleHeight);
        } else {
            bubbleRect = QRect(avatarRect.right() + 10,
                               avatarRect.top(), bubbleWidth, bubbleHeight);
        }

        // 绘制气泡背景
        painter->setPen(Qt::NoPen);
        painter->setBrush(msg.isSelf ? QColor(220, 248, 198) : QColor(240, 240, 240));
        painter->drawRoundedRect(bubbleRect, 10, 10);

        // 绘制文件图标
        QRect iconRect(bubbleRect.left() + bubblePadding,
                       bubbleRect.top() + bubblePadding,
                       40, 40);
        painter->setPen(Qt::gray);
        painter->setBrush(QColor(200, 200, 200));
        painter->drawRect(iconRect);
        painter->drawText(iconRect, Qt::AlignCenter, "📄");

        // 绘制文件名（限制宽度，过长时截断）
        QFont nameFont = option.font;
        nameFont.setBold(true);
        painter->setFont(nameFont);
        QFontMetrics nameFm(nameFont);
        QString elidedName = nameFm.elidedText(msg.fileName, Qt::ElideMiddle, 150);

        QRect nameRect(iconRect.right() + 10,
                       bubbleRect.top() + bubblePadding,
                       150, 20);
        painter->setPen(Qt::black);
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

        // 绘制文件大小
        QFont sizeFont = option.font;
        sizeFont.setPointSize(sizeFont.pointSize() - 1);
        painter->setFont(sizeFont);

        QString sizeStr;
        if (msg.fileSize < 1024) {
            sizeStr = QString("%1 B").arg(msg.fileSize);
        } else if (msg.fileSize < 1024*1024) {
            sizeStr = QString("%1 KB").arg(msg.fileSize / 1024.0, 0, 'f', 2);
        } else {
            sizeStr = QString("%1 MB").arg(msg.fileSize / (1024.0*1024.0), 0, 'f', 2);
        }

        QRect sizeRect(nameRect.left(), nameRect.bottom() + 5,
                       nameRect.width(), 20);
        painter->setPen(Qt::darkGray);
        painter->drawText(sizeRect, Qt::AlignLeft | Qt::AlignVCenter, sizeStr);

        // 绘制下载按钮
        QRect btnRect(bubbleRect.right() - 70, bubbleRect.bottom() - 30,
                      60, 24);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 150, 136, 180));
        painter->drawRoundedRect(btnRect, 12, 12);

        painter->setPen(Qt::white);
        painter->setFont(option.font);
        painter->drawText(btnRect, Qt::AlignCenter, "下载");

        // 保存点击区域
        const_cast<MessageDelegate*>(this)->clickableAreas[index] = bubbleRect;

    } else {
        // 文本消息动态计算气泡大小
        QTextDocument doc;
        doc.setDefaultFont(option.font);
        doc.setTextWidth(maxBubbleWidth - 2 * bubblePadding);
        doc.setHtml(Qt::convertFromPlainText(msg.content));

        QSizeF docSize = doc.size();
        int bubbleWidth = docSize.width() + 2 * bubblePadding;
        int bubbleHeight = docSize.height() + 2 * bubblePadding;

        if (msg.isSelf) {
            bubbleRect = QRect(avatarRect.left() - bubbleWidth - 10,
                               avatarRect.top(), bubbleWidth, bubbleHeight);
        } else {
            bubbleRect = QRect(avatarRect.right() + 10,
                               avatarRect.top(), bubbleWidth, bubbleHeight);
        }

        // 绘制气泡背景
        painter->setPen(Qt::NoPen);
        painter->setBrush(msg.isSelf ? QColor(220, 248, 198) : QColor(240, 240, 240));
        painter->drawRoundedRect(bubbleRect, 10, 10);

        // 绘制文本内容
        painter->setPen(Qt::black);
        painter->setFont(option.font);

        // 创建一个临时的矩形用于文本绘制
        QRect textRect = bubbleRect.adjusted(bubblePadding, bubblePadding,
                                             -bubblePadding, -bubblePadding);

        // 使用 QTextDocument 绘制文本，支持换行
        painter->save();
        painter->translate(textRect.topLeft());
        QRect clip(0, 0, textRect.width(), textRect.height());
        doc.drawContents(painter, clip);
        painter->restore();
    }

    painter->restore();
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


void ChatRoom::on_toolButton_file_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("选择要发送的文件"),
        QDir::homePath(),  // 起始目录，也可以用QStandardPaths::DocumentsLocation
        tr("所有文件 (*.*)") // 文件过滤器，可以根据需要调整
        );

    fsender->startFileUpload(g_session,filePath);

    // if (!filePath.isEmpty()) {
    //     QFileInfo fileInfo(filePath);
    //     QString fileName = fileInfo.fileName();      // 获取文件名（带扩展名）
    //     QString baseName = fileInfo.baseName();      // 获取文件名（不带扩展名）
    //     QString suffix = fileInfo.suffix();          // 获取文件扩展名
    //     qint64 fileSize = fileInfo.size();           // 获取文件大小（字节）
    //     qDebug() << "文件路径:" << filePath;
    //     qDebug() << "文件名:" << fileName;
    //     qDebug() << "文件大小:" << fileSize << "字节";
    //     qDebug() << "文件扩展名:" << suffix;
    //     appendFileMessage(filePath, fileName, fileSize, true,fileId);
    // }
}

void ChatRoom::appendFileMessage(const QString &fileName,
                                 qint64 fileSize, bool isSelf,const QString &fileId)
{
    QStandardItem *item = new QStandardItem();

    // 创建文件消息对象
    ChatMessage msg;
    msg.isFileMessage = true;
    msg.fileName = fileName;
    msg.fileSize = fileSize;
    msg.fileId = fileId; // 使用从服务器获取的fileId
    msg.isSelf = isSelf;
    msg.timestamp = QDateTime::currentDateTime();

    qDebug()<<msg.fileId;
    // 设置显示内容（将在delegate中使用）
    QString sizeStr;
    if (fileSize < 1024) {
        sizeStr = QString("%1 B").arg(fileSize);
    } else if (fileSize < 1024*1024) {
        sizeStr = QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 2);
    } else {
        sizeStr = QString("%1 MB").arg(fileSize / (1024.0*1024.0), 0, 'f', 2);
    }

    msg.content = QString("文件: %1 (%2)").arg(fileName).arg(sizeStr);

    // 将消息对象存储在item的数据中
    QVariant v;
    v.setValue(msg);
    item->setData(v, Qt::UserRole + 1);

    // 添加到模型
    model->appendRow(item);

    // 滚动到底部
    ui->listView->scrollToBottom();
}



void ChatRoom::on_pushButton_clicked()
{
    QString filePath = video_filepath;
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择保存文件路径");
        return;
    }

    if (!filePath.endsWith(".mp4", Qt::CaseInsensitive)) {
        filePath += ".mp4";
    }

    if (m_recorder->startRecording(filePath)) {
        // ui->startButton->setEnabled(false);
        // ui->stopButton->setEnabled(true);
        m_timer->start(1000); // 每秒更新一次
        m_recordingTime = 0;
    }
}


void ChatRoom::on_toolButton_video_clicked()
{
    // 创建简单的视频窗口
    QWidget* videoWindow = new QWidget();
    videoWindow->setWindowTitle("视频播放");
    videoWindow->resize(800, 600);

    VideoPlayer* videoPlayer = new VideoPlayer(100, videoWindow);

    QVBoxLayout* layout = new QVBoxLayout(videoWindow);
    layout->addWidget(videoPlayer);

    videoWindow->show();
}


void ChatRoom::on_pushButton_3_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this, "选择保存目录",
                                                         QDir::homePath(),
                                                         QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
    fileName += "/record.mp4";
    video_filepath=fileName;
}


void ChatRoom::on_pushButton_2_clicked()
{
    m_recorder->stopRecording();
}

void ChatRoom::onRecordingStarted()
{
    // ui->statusLabel->setText("状态: 录制中...");
}

void ChatRoom::onRecordingStopped()
{
    // ui->statusLabel->setText("状态: 录制完成");
    // ui->startButton->setEnabled(true);
    // ui->stopButton->setEnabled(false);
    m_timer->stop();
    QMessageBox::information(this, "完成", "录制已完成！文件已保存。");
}

void ChatRoom::onRecordingError(const QString& error)
{
    // ui->statusLabel->setText("状态: 错误 - " + error);
    // ui->startButton->setEnabled(true);
    // ui->stopButton->setEnabled(false);
    m_timer->stop();
    QMessageBox::critical(this, "错误", "录制失败: " + error);
}

void ChatRoom::updateRecordingTime()
{
    m_recordingTime++;
    ui->TimeLabel->setText(QString("录制时间: %1秒").arg(m_recordingTime));
}

void ChatRoom::on_pushButton_4_clicked()
{

}


void ChatRoom::on_toolButton_2_clicked()
{
    if (!m_cameraStreamer){
        m_cameraStreamer = new CameraStreamer(g_session, true);
    }
    qDebug() << m_cameraStreamer;
    QString rtmpUrl = "rtmp://localhost/live/" + g_username;
    m_cameraStreamer->startStreaming(rtmpUrl);
}


void ChatRoom::on_toolButton_share_clicked()
{
    if (!m_cameraStreamer){
        m_cameraStreamer = new CameraStreamer(g_session, true);
    }
    QString rtmpUrl = "rtmp://localhost/live/" + g_username + "_screen";
    m_cameraStreamer->startScreenStreaming(rtmpUrl);
}
