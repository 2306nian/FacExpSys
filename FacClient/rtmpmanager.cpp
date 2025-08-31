#include "rtmpmanager.h"
#include <QDebug>

RTMPManager* RTMPManager::m_instance = nullptr;

RTMPManager* RTMPManager::instance()
{
    if (!m_instance) {
        m_instance = new RTMPManager();
    }
    return m_instance;
}

RTMPManager::RTMPManager(QObject *parent)
    : QObject(parent)
{
}

void RTMPManager::sendStartStream(Session* session, const QString& ticketId, const QString& streamName)
{
    QJsonObject data{
        {"ticket_id", ticketId},
        {"stream_name", streamName}
    };

    QJsonObject msg{
        {"type", "rtmp_stream_start"},
        {"data", data}
    };

    emit streamStartRequested(session, msg);
}

void RTMPManager::sendStopStream(Session* session, const QString& ticketId)
{
    QJsonObject data{
        {"ticket_id", ticketId}
    };

    QJsonObject msg{
        {"type", "rtmp_stream_stop"},
        {"data", data}
    };

    emit streamStopRequested(session, msg);
}

void RTMPManager::sendStreamData(Session* session, const QString& ticketId, const QByteArray& data)
{
    QJsonObject jsonData{
        {"ticket_id", ticketId},
        {"stream_data", QString::fromLatin1(data.toBase64())}
    };

    QJsonObject msg{
        {"type", "rtmp_stream_data"},
        {"data", jsonData}
    };

    emit streamDataRequested(session, msg);
}
