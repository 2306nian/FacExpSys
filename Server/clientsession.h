#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include "qdatetime.h"
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

class WorkOrder;

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientSession();

    QTcpSocket *socket() const;
    WorkOrder *currentTicket() const;
    void setCurrentTicket(WorkOrder *ticket);
    void sendMessage(const QByteArray &data);
    QString clientIp() const;
    int clientPort() const;

    // RTMP相关
    void setRtmpStreamUrl(const QString &url);
    QString rtmpStreamUrl() const;
    bool isStreaming() const;

signals:
    void disconnected(ClientSession *session);
    void readyForMedia(const QString &ticketId);
    void textMessageReceived(ClientSession *sender, const QByteArray &message);
    void mediaDataReceived(ClientSession *sender, const QByteArray &data);
    void deviceDataRequest(ClientSession *sender, const QJsonObject &request);
    void deviceControlRequest(ClientSession* sender, const QJsonObject &command);
    void controlCommandReceived(const QJsonObject &command);

    // void fileUploadRequest(ClientSession* sender, const QJsonObject& uploadRequest); // 文件上传请求
    void fileUploadStart(ClientSession*, const QJsonObject&);
    void fileUploadChunk(ClientSession*, const QJsonObject&);
    void fileUploadEnd(ClientSession*, const QJsonObject&);

    void fileDownloadRequest(ClientSession* requester, const QJsonObject& downlodaRequest); // 文件下载请求

    // RTMP流信号
    void rtmpStreamStarted(ClientSession *sender, const QString &ticketId, const QString &streamUrl);
    void rtmpStreamStopped(ClientSession *sender, const QString &ticketId);
    void rtmpStreamDataReceived(ClientSession *sender, const QByteArray &data);

    void newTicketCreated(const QString &ticketId);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void handleMessage(const QByteArray &data);

    QTcpSocket *m_socket;
    WorkOrder *m_currentTicket;
    QByteArray m_buffer;
    QString m_clientIp;
    int m_clientPort;

    // RTMP相关
    QString m_rtmpStreamUrl;
    bool m_isStreaming;
    QDateTime m_streamStartTime;
};

#endif // CLIENTSESSION_H
