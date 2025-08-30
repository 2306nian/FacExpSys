#ifndef RTMPMANAGER_H
#define RTMPMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QMutex>

class ClientSession;

class RTMPManager : public QObject
{
    Q_OBJECT

public:
    static RTMPManager *instance();

    void onStreamStarted(ClientSession *sender, const QString &ticketId, const QString &streamUrl);
    void onStreamStopped(ClientSession *sender, const QString &ticketId);
    void relayStreamData(ClientSession *sender, const QByteArray &data);

    bool isTicketStreaming(const QString &ticketId) const;
    QString getStreamUrl(const QString &ticketId) const;

signals:
    void streamStarted(const QString &ticketId, const QString &streamUrl);
    void streamStopped(const QString &ticketId);
    void streamDataAvailable(const QString &ticketId, const QByteArray &data);

private:
    explicit RTMPManager(QObject *parent = nullptr);
    static RTMPManager *m_instance;

    struct StreamInfo {
        ClientSession *publisher;
        QString streamUrl;
    };

    QMap<QString, StreamInfo> m_streams; // ticketId -> StreamInfo
    mutable QMutex m_mutex;
};

#endif // RTMPMANAGER_H
