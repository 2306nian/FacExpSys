#include "rtmpmanager.h"
#include "clientsession.h"
#include "workordermanager.h"
#include "workorder.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

RTMPManager *RTMPManager::m_instance = nullptr;

RTMPManager *RTMPManager::instance()
{
    if (!m_instance) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!m_instance)
            m_instance = new RTMPManager;
    }
    return m_instance;
}

RTMPManager::RTMPManager(QObject *parent)
    : QObject(parent)
{
}

void RTMPManager::onStreamStarted(ClientSession *sender, const QString &ticketId, const QString &streamUrl)
{
    QMutexLocker locker(&m_mutex);

    StreamInfo info;
    info.publisher = sender;
    info.streamUrl = streamUrl;

    // 获取工单中的所有客户端作为订阅者
    WorkOrder *order = WorkOrderManager::instance()->getWorkOrder(ticketId);
    if (order) {
        for (ClientSession *client : order->clients) {
            if (client != sender) { // 排除发布者自己
                // 通知订阅者有新流开始
                QJsonObject notification{
                    {"type", "rtmp_stream_available"},
                    {"data", QJsonObject{
                                 {"ticket_id", ticketId},
                                 {"stream_url", streamUrl},
                                 {"publisher_ip", sender->clientIp()},
                                 {"publisher_port", sender->clientPort()}
                             }}
                };

                client->sendMessage(QJsonDocument(notification).toJson(QJsonDocument::Compact));
            }
        }
    }

    m_streams[ticketId] = info;
    emit streamStarted(ticketId, streamUrl);

    qDebug() << "RTMP stream started for ticket:" << ticketId;
}

void RTMPManager::onStreamStopped(ClientSession *sender, const QString &ticketId)
{
    QMutexLocker locker(&m_mutex);
    WorkOrder* order = sender->currentTicket();

    if (m_streams.contains(ticketId)) {
        StreamInfo &info = m_streams[ticketId];

        if (info.publisher == sender) {
            // 通知所有订阅者流已停止
            QJsonObject notification{
                {"type", "rtmp_stream_ended"},
                {"data", QJsonObject{
                             {"ticket_id", ticketId},
                             {"message", "Stream has ended"}
                         }}
            };

            for (ClientSession *client : order->clients) {
                if (client == sender) continue;
                client->sendMessage(QJsonDocument(notification).toJson(QJsonDocument::Compact));
            }

            m_streams.remove(ticketId);
            emit streamStopped(ticketId);

            qDebug() << "RTMP stream stopped for ticket:" << ticketId;
        }
    }
}

void RTMPManager::relayStreamData(ClientSession *sender, const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    WorkOrder *order = sender->currentTicket();
    if (order && m_streams.contains(order->ticketId)) {
        StreamInfo &info = m_streams[order->ticketId];

        if (info.publisher == sender) {
            // 将流数据转发给所有订阅者
            for (ClientSession *client : order->clients) {
                if (client == sender) continue;
                QJsonObject notification{
                    {"type", "rtmp_stream_data"},
                    {"data", QJsonObject{
                                {"ticket_id", order->ticketId},
                                {"data_size", data.size()},
                                {"stream_data", QString::fromLatin1(data.toBase64())},
                                {"timestamp", QDateTime::currentMSecsSinceEpoch()}
                             }}
                };

                client->sendMessage(QJsonDocument(notification).toJson(QJsonDocument::Compact));
            }

            emit streamDataAvailable(order->ticketId, data);
        }
    }
}

bool RTMPManager::isTicketStreaming(const QString &ticketId) const
{
    QMutexLocker locker(&m_mutex);
    return m_streams.contains(ticketId);
}

QString RTMPManager::getStreamUrl(const QString &ticketId) const
{
    QMutexLocker locker(&m_mutex);
    return m_streams.value(ticketId).streamUrl;
}
