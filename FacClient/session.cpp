#include "session.h"
#include "handlers.h"
#include "common.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include<chatroom.h>
#include"clientcore.h"

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
    connect(TicketHandler::instance(),&TicketHandler::sendTicketToSession,this,&Session::setTickedFromHandel);
    connect(this,&Session::fileDownloadStart,FileHandler::instance(),&FileHandler::handelDownload);

    connect(this, &Session::rtmpStreamAvailableRecv, RTMPHandler::instance(), &RTMPHandler::handleRTMPAvailableRecv);
    connect(this, &Session::rtmpStreamStopRecv, RTMPHandler::instance(), &RTMPHandler::handleRTMPStopRecv);
    connect(this, &Session::rtmpStreamStartedRecv, RTMPHandler::instance(), &RTMPHandler::handleRTMPStartedRecv);
    connect(this, &Session::rtmpStreamStartSend, RTMPHandler::instance(), &RTMPHandler::handleRTMPStartSend);
    connect(this, &Session::rtmpStreamStopSend, RTMPHandler::instance(), &RTMPHandler::handleRTMPStopSend);

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

void Session::setTickedId(QString s1){
    ticketId=s1;
}
QString Session::getTicketId(){
    return ticketId;
}


void Session::setTickedFromHandel(QString s1){
    setTickedId(s1);
}

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
    else if(doc["type"]=="work_orders"){
        qDebug() << "收到工单创建完成请求";
        // 正确获取data数组
        QJsonArray dataArray = doc["data"].toArray();
        // TODO:此处有问题
        emit createChatRoom();
        emit ticketCreateRecv(this, dataArray);
    }
    else if(doc["type"]=="text_msg"){
        qDebug()<<"收到传来的消息";
        QJsonObject dataObj = doc["data"].toObject();
        emit textMessageSend(g_session,dataObj);
        emit textUpdate();
    }
    else if(doc["type"]=="upload_started"){
        qDebug()<<"接收到上传信号";
        QJsonObject dataObj = doc.object();
        emit fileUploadStart(g_session,dataObj);
    }
    else if(doc["type"]=="file_uploaded"){
        qDebug()<<"准备开始接收文件信息";
        QJsonObject dataObj = doc["data"].toObject();
        emit fileUploadedRecv(dataObj);
    }
    else if(doc["type"]=="file_meta"||doc["type"]=="file_chunk"){
        // qDebug()<<"开始下载文件";
        QJsonObject json_obj=doc.object();
        emit fileDownloadStart(json_obj);
    }
    else if(doc["type"]=="device_list"||doc["type"]=="device_realtime_update"){
        // 正确获取devices数组，它位于data.devices中
        QJsonArray deviceDataArray = doc["data"].toObject()["devices"].toArray();
        emit deviceDataArrayReceived(deviceDataArray); // 发送信号，通知UI更新多个设备信息
    }
    //TODO:RTMP处理
    else if(doc["type"]=="rtmp_stream_started"){
        QJsonObject dataObj = doc["data"].toObject();
        emit rtmpStreamStartedRecv(dataObj);
    }
    else if(doc["type"]=="rtmp_stream_rtmp_stream_available"){
        qDebug() << "A Stream Available";
        QJsonObject dataObj = doc["data"].toObject();
        emit rtmpStreamAvailableRecv(dataObj);
    }
    else if(doc["type"]=="rtmp_stream_ended"){
        qDebug() << "A Stream Stop";
        QJsonObject dataObj = doc["data"].toObject();
        emit rtmpStreamStopRecv(dataObj);
    }
}
