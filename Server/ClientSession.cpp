// ClientSession.cpp
#include "ClientSession.h"
#include "WorkOrderManager.h"
#include "MessageRouter.h"
#include <QJsonDocument>
#include <QJsonObject>

ClientSession::ClientSession(QTcpSocket* socket, QObject *parent)
    : QObject(parent), m_socket(socket), m_currentTicket(nullptr)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onSocketDisconnected);
}

QTcpSocket* ClientSession::socket() const { return m_socket; }

WorkOrder* ClientSession::currentTicket() const { return m_currentTicket; }

void ClientSession::setCurrentTicket(WorkOrder* ticket) { m_currentTicket = ticket; }

void ClientSession::sendMessage(const QByteArray &data)
{
    // 发送带长度头的消息
    quint32 len = data.size();
    char header[4];
    qToBigEndian<quint32>(len, reinterpret_cast<uchar*>(header));
    m_socket->write(header, 4);
    m_socket->write(data);
}

void ClientSession::onReadyRead()
{
    while (m_socket->bytesAvailable() > 0) {
        if (m_buffer.size() < 4) {
            m_buffer.append(m_socket->read(4 - m_buffer.size()));
            if (m_buffer.size() < 4) return;
        }

        quint32 totalSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_buffer.data()));
        int bodySize = totalSize - m_buffer.size() + 4;

        if (m_socket->bytesAvailable() < bodySize) {
            m_buffer.append(m_socket->read(m_socket->bytesAvailable()));
            return;
        }

        m_buffer.append(m_socket->read(bodySize));
        QByteArray msg = m_buffer.mid(4);

        // 处理消息
        handleMessage(msg);

        m_buffer.clear();
    }
}

void ClientSession::onSocketDisconnected()
{
    emit clientDisconnected(this);
}

void ClientSession::handleMessage(const QByteArray &data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Invalid JSON:" << data;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "create_ticket") {
        QString devId = obj["data"]["device_id"].toString();
        QString ticketId = WorkOrderManager::instance()->createTicket(devId);
        QJsonObject resp{
            {"type", "ticket_created"},
            {"data", QJsonObject{{"ticket_id", ticketId}}}
        };
        sendMessage(QJsonDocument(resp).toJson(QJsonDocument::Compact));
    }
    else if (type == "join_ticket") {
        QString tid = obj["data"]["ticket_id"].toString();
        if (WorkOrderManager::instance()->joinTicket(tid, this)) {
            QJsonObject resp{
                {"type", "joined_ticket"},
                {"data", {{"ticket_id", tid}}}
            };
            sendMessage(QJsonDocument(resp).toJson(QJsonDocument::Compact));
            emit readyForMedia(tid);
        }
    }
    else if (type == "text_msg") {
        MessageRouter::instance()->routeMessage(this, data);
    }
    else if (type == "video_frame" || type == "audio_chunk") {
        MediaRelay::instance()->relayMedia(this, data);
    }
    else if (type == "request_device_data") {
        DeviceProxy::instance()->requestDataFromDevice(this, obj);
    }
    else if (type == "control_command") {
        DeviceProxy::instance()->sendControlCommand(obj);
    }
    // 其他类型...
}
