#ifndef RTMPMANAGER_H
#define RTMPMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include "session.h"

class RTMPManager : public QObject
{
    Q_OBJECT

public:
    static RTMPManager* instance();

    // 打包发送消息
    void sendStartStream(Session* session, const QString& ticketId, const QString& streamName);
    void sendStopStream(Session* session, const QString& ticketId);
    void sendStreamData(Session* session, const QString& ticketId, const QByteArray& data);

private:
    explicit RTMPManager(QObject *parent = nullptr);
    static RTMPManager* m_instance;

signals:
    // 接收流相关信号
    void streamAvailable(const QString& ticketId, const QString& streamUrl);
    void streamEnded(const QString& ticketId);
    void streamDataReceived(const QByteArray& data);

    // 发送流相关信号
    void streamStartRequested(Session* session, const QJsonObject& data);
    void streamStopRequested(Session* session, const QJsonObject& data);
    void streamDataRequested(Session* session, const QJsonObject& data);
};

#endif // RTMPMANAGER_H
