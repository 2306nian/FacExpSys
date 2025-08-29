#include "clientsession.h"
#include "qhostaddress.h"
#include "qjsonarray.h"
#include "workordermanager.h"
#include "common.h"
#include "mediarelay.h"
#include "messagerouter.h"
#include "deviceproxy.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ClientSession::ClientSession(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_currentTicket(nullptr)
    , m_clientIp(socket->peerAddress().toString())
    , m_clientPort(socket->peerPort())
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
    connect(this, &ClientSession::textMessageReceived,
            MessageRouter::instance(), &MessageRouter::routeTextMessage);

    connect(this, &ClientSession::mediaDataReceived,
            MediaRelay::instance(), &MediaRelay::relayMedia);

    connect(this, &ClientSession::deviceDataRequest,
            DeviceProxy::instance(), [](ClientSession* sender, const QJsonObject& req) {
                DeviceProxy::instance()->requestData(sender, req);
            });

}

ClientSession::~ClientSession()
{
    if (m_socket) {
        m_socket->deleteLater();
    }
}

QTcpSocket *ClientSession::socket() const
{
    return m_socket;
}

WorkOrder *ClientSession::currentTicket() const
{
    return m_currentTicket;
}

void ClientSession::setCurrentTicket(WorkOrder *ticket)
{
    m_currentTicket = ticket;
}

void ClientSession::sendMessage(const QByteArray &data)
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(packMessage(data));
    }
}

void ClientSession::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    QByteArray message;
    while (unpackMessage(m_buffer, message)) {
        handleMessage(message);
    }
}

void ClientSession::onDisconnected()
{
    emit disconnected(this);
}

void ClientSession::handleMessage(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Invalid JSON message:" << data;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "create_ticket") {
        QJsonObject dataObj = obj["data"].toObject();
        QString username = dataObj["username"].toString();
        QJsonArray deviceArray = dataObj["device_ids"].toArray();

        QStringList deviceIds;
        for (const QJsonValue &val : deviceArray) {
            deviceIds << val.toString();
        }

        // 使用当前 session 的真实连接信息
        QString ticketId = WorkOrderManager::instance()->createTicket(
            this,           // ← 传入当前 session
            deviceIds,
            username
            );

        QJsonObject response{
            {"type", "ticket_created"},
            {"data", QJsonObject{{"ticket_id", ticketId}}}
        };
        sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
    else if (type == "join_ticket") {
        QString ticketId = obj["data"].toObject()["ticket_id"].toString();
        if (WorkOrderManager::instance()->joinTicket(ticketId, this)) {
            QJsonObject response{
                {"type", "joined_ticket"},
                {"data", QJsonObject{{"ticket_id", ticketId}}}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            emit readyForMedia(ticketId);
        }
    }
    else if (type == "text_msg") {
        emit textMessageReceived(this, data);
    }
    else if (type == "video_frame" || type == "audio_chunk") {
        emit mediaDataReceived(this, data);
    }
    else if (type == "request_device_data") {
        emit deviceDataRequest(this, obj["data"].toObject());
    }
    else if (type == "control_command") {
        emit controlCommandReceived(obj["data"].toObject());
    }
    else if (type == "accept_ticket") {
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();
        QString expertUsername = dataObj["expert_username"].toString();
        QString expertIp = m_socket->peerAddress().toString();
        int expertPort = m_socket->peerPort();

        WorkOrderManager::instance()->acceptTicket(ticketId, expertUsername, expertIp, expertPort);
    }
    else if (type == "complete_ticket") {
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();
        QString description = dataObj["description"].toString();
        QString solution = dataObj["solution"].toString();

        WorkOrderManager::instance()->completeTicket(ticketId, description, solution);
    }
}

QString ClientSession::clientIp() const
{
    return m_clientIp;
}

int ClientSession::clientPort() const
{
    return m_clientPort;
}
