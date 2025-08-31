#include "session.h"
#include "handlers.h"
#include "common.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

Session::Session(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &Session::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &Session::onDisconnected);

    connect(this, &Session::textMessageSend, MessageHandler::instance(), &MessageHandler::handleTextMessage);
    connect(this, &Session::acceptTicketSend, TicketHandler::instance(), &TicketHandler::handleAcceptTicket);
    connect(this, &Session::completeTicketSend, TicketHandler::instance(), &TicketHandler::handleCompleteTicket);
    connect(this, &Session::joinTicketSend, TicketHandler::instance(), &TicketHandler::handleJoinTicket);
    connect(this, &Session::joinedTicketRecv, TicketHandler::instance(), &TicketHandler::handleTicketJoined);
    connect(this, &Session::ticketCreateRecv, TicketHandler::instance(), &TicketHandler::handleTicketCreate);

    connect(this, &Session::fileChunkRecv, FileHandler::instance(), &FileHandler::handleFileChunk);
    connect(this, &Session::fileMetaRecv, FileHandler::instance(), &FileHandler::handleFileMeta);
    connect(this, &Session::fileUploadChunk, FileHandler::instance(), &FileHandler::handleFileUploadChunk);
    connect(this, &Session::fileUploadStart, FileHandler::instance(), &FileHandler::handleFileUploadStart);
    connect(this, &Session::fileUploadedRecv, FileHandler::instance(), &FileHandler::handleFileUploaded);
    connect(this, &Session::fileDownloadRequest, FileHandler::instance(), &FileHandler::downloadFileRequest);
}
Session::~Session() // 应该在析构函数中添加一个清理函数 防止意外连接中断时m_uploads不会被清除 不过不必要
{
    if (m_socket) {
        m_socket->deleteLater();
    }

    // // 如果正在推流，通知停止
    // if (m_isStreaming && m_currentTicket) {
    //     emit rtmpStreamStopped(this, m_currentTicket->ticketId);
    // }
}

<<<<<<< HEAD
QString Session::getTicketId(){
    return ticketId;
}
=======
>>>>>>> fd637657edae9c87494b0eaa1fde8ce1100cd062

void Session::sendMessage(const QByteArray &data)
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(packMessage(data));
    }
}

void Session::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    QByteArray message;
    while (unpackMessage(m_buffer, message)) {
        handleMessage(message);
    }
}

void Session::onDisconnected()
{
    emit disconnected(this);
}

void Session::handleMessage(const QByteArray &data)
{
    //此处可能后续需要修改
    QJsonDocument doc= QJsonDocument::fromJson(data);
    if (doc["type"] == "register_result"){
        QJsonObject dataObj = doc["data"].toObject();
        emit registerResult(dataObj["success"].toBool());
    }
    else if(doc["type"] == "login_result"){
        QJsonObject dataObj = doc["data"].toObject();
        emit loginResult(dataObj["success"].toBool());
    }
    //TODO:RTMP处理

}
