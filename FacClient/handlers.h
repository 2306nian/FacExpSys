#ifndef HANDLERS_H
#define HANDLERS_H

#include <QObject>

#include "session.h"
class MessageHandler: public QObject
{
    Q_OBJECT
    public:
        static MessageHandler* instance();
        void handleTextMessage(Session* sender, QByteArray &data);
    private:
        explicit MessageHandler(QObject *parent = nullptr);
        static MessageHandler* m_instance;
};

class FileHandler: public QObject
{
    Q_OBJECT
    public:
        static FileHandler* instance();
        //发送
        void handleFileUploadStart(Session *sender, const QJsonObject &data);
        void handleFileUploadChunk(Session *sender, const QJsonObject &data);
        void downloadFileRequest(Session *sender, const QJsonObject &data);

        //接收
        void handleUploadStarted(Session *sender, const QJsonObject &data);
        void handleFileUploaded(Session *sender, const QJsonObject &data);
        void handleFileMeta(Session *client, const QJsonObject &data);
        void handleFileChunk(Session *client, const QJsonObject &data);

    private:
        explicit FileHandler(QObject *parent=nullptr);
        static FileHandler* m_instance;
};

class TicketHandler: public QObject
{
    Q_OBJECT
    public:
        static TicketHandler* instance();
        //发送
        void handleCreateTicket(Session *sender, const QJsonObject &data);
        void handleJoinTicket(Session *sender, const QJsonObject &data);
        void handleAcceptTicket(Session *sender, const QJsonObject &data);
        void handleCompleteTicket(Session *sender, const QJsonObject &data);

        //接收
        void handleTicketCreate(Session *client, const QJsonObject &data);
        void handleTicketJoined(Session *client, const QJsonObject &data);

    private:
        explicit TicketHandler(QObject *parent = nullptr);
        static TicketHandler* m_instance;
};

#endif // HANDLERS_H
