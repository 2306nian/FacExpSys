#ifndef CHATROOM_H
#define CHATROOM_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QDateTime>
#include <QTimer>
#include<session.h>
#include<filesender.h>
#include<QEvent>
#include <QMouseEvent>
#include<QMap>
#include<QTimer>
#include<filereceiver.h>

namespace Ui {
class ChatRoom;
}

// 消息结构体
struct ChatMessage {
    QString content;
    bool isSelf;
    QDateTime timestamp;
    bool isFileMessage = false;  // 是否为文件消息
    QString filePath;           // 文件路径
    QString fileName;           // 文件名
    qint64 fileSize;            // 文件大小
    QString fileId;             // 文件ID
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

    QMap<QModelIndex, QRect> getClickableAreas() const {
        return clickableAreas;
    }
    // 在 ChatRoom 类定义中添加:


private:
    mutable QMap<QModelIndex, QRect> clickableAreas;

};

class ChatRoom : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatRoom(QWidget *parent = nullptr);
    ~ChatRoom();
    void appendFileMessage(const QString &fileName,
                           qint64 fileSize, bool isSelf,const QString &fileId);

    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_pushButton_emission_clicked();

    void on_toolButton_clicked();

    void messageUpdate();

    void messageData(QString s1);

    void on_toolButton_file_clicked();

    void getFileidFromhandle(QString s1,QString f_name,qint64 f_size);

    void getFileId(QString fileId,QJsonObject &obj);

    void startUploadInChat(const QJsonObject &data);

    void getFileInfo(const QJsonObject&data);

    void startDownload(const QJsonObject&json);

private:
    Ui::ChatRoom *ui;
    QStandardItemModel *model;
    MessageDelegate *messageDelegate;
    Session *sender;
    QString receivedMsg;
    QString fileId;
    QString fileName;
    QString file_size;
    FileSender *fsender;
    FileReceiver *freceiver;
    QString pendingFilePath;
    QString pendingFileName;
    qint64 pendingFileSize;
    void appendMessage(const QString &text, bool isSelf);
    QMap<QString, QString> fileIdMap;
};

#endif // CHATROOM_H


