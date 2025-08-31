#include "handlers.h"
#include <QJsonDocument>
#include <QObject>
#include <QJsonObject>
#include <QDebug>
#include<QJsonArray>

MessageHandler* MessageHandler::m_instance = nullptr;
FileHandler* FileHandler::m_instance = nullptr;
TicketHandler* TicketHandler::m_instance = nullptr;
RTMPHandler* RTMPHandler::m_instance = nullptr;

//==============MessageHandler================
MessageHandler* MessageHandler::instance(){
    if (!m_instance) {
        m_instance = new MessageHandler;
    }
    return m_instance;
}

MessageHandler::MessageHandler(QObject *parent)
    : QObject(parent){}

void MessageHandler::handleTextMessage(Session* sender, QJsonObject &data){
    QString message=data["message"].toString();
    if(!message.isEmpty()){
        qDebug()<<"消息已经就绪，准备放入聊天框"<<message;
    }
    emit sendMessageToChat(message);
}
//==============FileHandler================
FileHandler* FileHandler::instance(){
    if (!m_instance) {
        m_instance = new FileHandler;
    }
    return m_instance;
}
FileHandler::FileHandler(QObject *parent)
    : QObject(parent){}

//发送
void FileHandler::handleFileUploadStart(Session *sender, const QJsonObject &data){

}

void FileHandler::handleFileUploadChunk(Session *sender, const QJsonObject &data){

}

void FileHandler::downloadFileRequest(Session *sender, const QJsonObject &data){

}

//接收
void FileHandler::handleUploadStarted(const QJsonObject &data){

}

void FileHandler::handleFileUploaded(const QJsonObject &data){

}

void FileHandler::handleFileMeta(const QJsonObject &data){

}

void FileHandler::handleFileChunk(const QJsonObject &data){

}

//==============TicketHandler================
TicketHandler* TicketHandler::instance(){
    if (!m_instance) {
        m_instance = new TicketHandler;
    }
    return m_instance;
}
TicketHandler::TicketHandler(QObject *parent)
    : QObject(parent){}

//发送
void TicketHandler::handleCreateTicket(Session *sender, const QJsonObject &data){

}

void TicketHandler::handleJoinTicket(Session *sender, const QJsonObject &data){

}

void TicketHandler::handleAcceptTicket(Session *sender, const QJsonObject &data){

}

void TicketHandler::handleCompleteTicket(Session *sender, const QJsonObject &data){

}

//接收
void TicketHandler::handleTicketCreate(const QJsonArray &data){
    QJsonObject firstOrder = data.first().toObject();
    QString s2=firstOrder["ticket_id"].toString();
    emit sendTicketToSession(s2);
}

void TicketHandler::handleTicketJoined(const QJsonObject &data){

}

//================RTMPHandler===============
RTMPHandler* RTMPHandler::instance(){
    if (!m_instance){
        m_instance = new RTMPHandler;
    }
    return m_instance;
}
RTMPHandler::RTMPHandler(QObject *parent)
    : QObject(parent){}

void RTMPHandler::handleRTMPAvailableRecv(const QJsonObject &data){
    QString streamurl = data["stream_url"].toString();
    urls.append(streamurl);
    qDebug() << "stream url: " << streamurl;
    return;
}
void RTMPHandler::handleRTMPStartedRecv(const QJsonObject &data){
    QString ticket_id = data["ticket_id"].toString();
    QString stream_url = data["stream_url"].toString();
    qDebug() << "ticket id:" << ticket_id << "\n" << "stream url:" << stream_url;
}

void RTMPHandler::handleRTMPStopRecv(const QJsonObject &data){
    QString stream_url = data["stream_url"].toString();
    urls.removeOne(stream_url);
}

void RTMPHandler::handleRTMPStartSend(Session *sender, QString stream_url){
    QJsonObject json;
    json["type"] = "rtmp_stream_start";
    QJsonObject data;
    data["stream_url"] = stream_url;
    json["data"] = data;
    sender->sendMessage(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void RTMPHandler::handleRTMPStopSend(Session *sender, QString stream_url){
    QJsonObject json;
    json["type"] = "rtmp_stream_stop";
    QJsonObject data;
    data["stream_url"] = stream_url;
    json["data"] = data;
    sender->sendMessage(QJsonDocument(json).toJson(QJsonDocument::Compact));
}
