#ifndef HANDLERS_H
#define HANDLERS_H

#include <QObject>

#include "session.h"
class MessageHandler: public QObject
{
    Q_OBJECT
    public:
        static MessageHandler* instance();
        void handleTextMessage(Session* sender, QJsonObject &data);
    private:
        explicit MessageHandler(QObject *parent = nullptr);
        static MessageHandler* m_instance;
    signals:
        void sendMessageToChat(QString s1);
};

class FileHandler: public QObject
{
    Q_OBJECT
    public:
        static FileHandler* instance();
        //发送
        void handleFileUploadStart(Session *sender,  QJsonObject &data);
        void handleFileUploadChunk(Session *sender, const QJsonObject &data);
        void downloadFileRequest(Session *sender, const QJsonObject &data);

        //接收
        void handleUploadStarted(const QJsonObject &data);
        void handleFileUploaded(const QJsonObject &data);
        void handleFileMeta(const QJsonObject &data);
        void handleFileChunk(const QJsonObject &data);
        void handelDownload(const QJsonObject &json);

    private:
        explicit FileHandler(QObject *parent=nullptr);
        static FileHandler* m_instance;

    signals:
        void sendFileidToChat(QString s1,QString s2,qint64 s3);
        void startFileUploadInChat(const QJsonObject &data);
        void getfileId(QString s1,QJsonObject &jobj);
        void ChatDownloadStart(const QJsonObject&json);
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
        void handleTicketCreate(Session *, const QJsonArray &data);
        void handleTicketJoined(const QJsonObject &data);

    private:
        explicit TicketHandler(QObject *parent = nullptr);
        static TicketHandler* m_instance;

    signals:
        void sendTicketToSession(QString s1);
};

class RTMPHandler: public QObject
{
    Q_OBJECT
    public:
        static RTMPHandler* instance();
        //发送
        void handleRTMPStartSend(Session* ,QString);
        void handleRTMPStopSend(Session*, QString);

        //接收
        void handleRTMPStartedRecv(const QJsonObject &data);
        void handleRTMPAvailableRecv(const QJsonObject &data);
        void handleRTMPStopRecv(const QJsonObject &data);
        QList<QString> getURLs();
    private:
        explicit RTMPHandler(QObject* parent=nullptr);
        static RTMPHandler* m_instance;
        QList<QString> urls;
};

#endif // HANDLERS_H
