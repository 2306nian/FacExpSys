#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include "globaldatas.h"
#include <QTcpSocket>
#include <QString>

class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(QTcpSocket *socket, QObject *parent = nullptr);
    ~Session();

    QTcpSocket *socket() const;
    void sendMessage(const QByteArray &data);
    QString getTicketId();
    static Session* instance();
    QString getTickedId();
    void setTickedId(QString s1);
signals:
    void disconnected(Session *session);
    void textUpdate();
    //发包
    void textMessageSend(Session *sender, QJsonObject &message);
    void loginRequestSend(Session *sender, QByteArray &data);
    void regeisterRequestSend(Session *sender, QByteArray &data);
    void createTicketSend(Session *sender, QJsonObject &data);
    void joinTicketSend(Session *sender, QJsonObject &data);
    void acceptTicketSend(Session *sender, QJsonObject &data);
    void completeTicketSend(Session *sender, QJsonObject &data);
    void confirmCompleted(const QString &ticketId);  // 发出完成确认信号

    // void fileUploadRequest(ClientSession* sender, const QJsonObject& uploadRequest); // 文件上传请求
    void fileUploadStart(Session*,  QJsonObject&);
    void fileUploadChunk(Session*, const QJsonObject&);
    void fileInfoSend(const QJsonObject&);
    void fileDownloadRequest(Session* requester, const QJsonObject& downlodaRequest); // 文件下载请求
    //RTMP
    void rtmpStreamStartSend(Session *, const QString &);
    void rtmpStreamStopSend(Session *, const QString &);

    //收包
    void registerResultRecv(QByteArray &data);
    void loginResultRecv(QByteArray &data);
    void ticketCreateRecv(Session *, QJsonArray &data);
    void joinedTicketRecv(QJsonObject &data);
    void tableInitial(Session* session,QJsonArray &data);//新增信号发往mainwindow
    void addTicket(Session* session,QJsonObject &data);//发给mainwindow

    void fileMetaRecv(QJsonObject &data);
    void fileChunkRecv(QJsonObject &data);
    void uploadStartedRecv(QJsonObject &data);
    void fileUploadedRecv(QJsonObject &data);
    //RTMP
    void rtmpStreamAvailableRecv(QByteArray &data);
    void rtmpStreamStartedRecv(QByteArray &);
    void rtmpStreamEndedRecv(QByteArray &);
    void rtmpStreamDataRecv(QByteArray &);

    void createChatRoom();

    //TODO:与ClientCore通信
    void registerResult(bool);
    void loginResult(bool);

    void fileDownloadStart(const QJsonObject &json);

    void deviceDataArrayReceived(const QJsonArray &arr);

public slots:
    void handleAcceptTicket(QJsonObject &data); // 自己接收信号并发送
    void handleCompleteTicket(QJsonObject &data);
    void create_ChatRoom();


private slots:
    void onReadyRead();
    void onDisconnected();
    void setTickedFromHandel(QString s1);

private:
    void handleMessage(const QByteArray &data);
    static Session* m_instance;

    QTcpSocket *m_socket;
    QByteArray m_buffer;

    QString ticketId;
};

#endif // SESSION_H
