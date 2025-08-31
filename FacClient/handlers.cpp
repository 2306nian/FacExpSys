#include "handlers.h"
#include <QObject>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>

MessageHandler* MessageHandler::m_instance = nullptr;
FileHandler* FileHandler::m_instance = nullptr;
TicketHandler* TicketHandler::m_instance = nullptr;

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
void FileHandler::handleUploadStarted(Session *sender, const QJsonObject &data){

}

void FileHandler::handleFileUploaded(Session *sender, const QJsonObject &data){

}

void FileHandler::handleFileMeta(Session *client, const QJsonObject &data){

}

void FileHandler::handleFileChunk(Session *client, const QJsonObject &data){

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
void TicketHandler::handleTicketCreate(Session *client, const QJsonArray &data){
    QJsonObject firstOrder = data.first().toObject();
    QString s2=firstOrder["ticket_id"].toString();
    emit sendTicketToSession(s2);
}

void TicketHandler::handleTicketJoined(Session *client, const QJsonObject &data){

}
