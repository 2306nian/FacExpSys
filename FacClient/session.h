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

    // void fileUploadRequest(ClientSession* sender, const QJsonObject& uploadRequest); // 文件上传请求
    void fileUploadStart(Session*, const QJsonObject&);
    void fileUploadChunk(Session*, const QJsonObject&);

    void fileDownloadRequest(Session* requester, const QJsonObject& downlodaRequest); // 文件下载请求

    //收包
    void registerResultRecv(Session *client, QByteArray &data);
    void loginResultRecv(Session *client, QByteArray &data);
    void ticketCreateRecv(Session *client, QJsonArray &data);
    void joinedTicketRecv(Session *client, QJsonObject &data);

    void fileMetaRecv(Session *client, QJsonObject &data);
    void fileChunkRecv(Session *client, QJsonObject &data);
    void uploadStartedRecv(Session *client, QJsonObject &data);
    void fileUploadedRecv(Session *client, QJsonObject &data);
    void createChatRoom();
    //TODO:与ClientCore通信
    void registerResult(bool);
    void loginResult(bool);

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
