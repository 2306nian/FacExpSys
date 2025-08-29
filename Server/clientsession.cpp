#include "clientsession.h"
#include "workordermanager.h"
#include "common.h"
#include "mediarelay.h"
#include "messagerouter.h"
#include "filerouter.h"
#include "deviceproxy.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ClientSession::ClientSession(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_currentTicket(nullptr)
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

    connect(this, &ClientSession::fileUploadRequest,
            FileRouter::instance(), &FileRouter::handleFileUploadRequest);

    connect(this, &ClientSession::fileDownloadRequest,
            FileRouter::instance(), &FileRouter::handleFileDownloadRequest);
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
        QString deviceId = obj["data"].toObject()["device_id"].toString();
        QString ticketId = WorkOrderManager::instance()->createTicket(deviceId);

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
        emit textMessageReceived(this, data); // 文字传输功能已实现
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
    else if (type == "file_upload") {
        emit fileUploadRequest(this, obj["data"].toObject()); // 客户端发送文件
    }
    else if (type == "file_download") {
        emit fileDownloadRequest(this, obj["data"].toObject()); // 发出接收文件请求信号
        }
    }

