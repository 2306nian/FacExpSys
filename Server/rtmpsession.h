#ifndef RTMPSESSION_H
#define RTMPSESSION_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>

extern "C" {
#include <librtmp/rtmp.h>
#include <librtmp/log.h>
}

class StreamManager;

class RTMPSession : public QObject
{
    Q_OBJECT

public:
    explicit RTMPSession(QTcpSocket *socket, StreamManager *streamManager, QObject *parent = nullptr);
    ~RTMPSession();

signals:
    void disconnected(RTMPSession *session);
    void streamPublished(const QString &streamName);
    void streamUnpublished(const QString &streamName);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onTimeout();

private:
    void handleRTMPHandshake();
    void handleRTMPMessage();
    void sendRTMPResponse();

    QTcpSocket *m_socket;
    StreamManager *m_streamManager;
    QByteArray m_buffer;
    QTimer *m_timeoutTimer;

    // RTMP相关
    bool m_handshakeDone;
    QString m_appName;
    QString m_streamName;
    bool m_isPublishing;
};

#endif // RTMPSESSION_H
