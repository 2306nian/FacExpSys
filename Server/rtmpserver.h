#ifndef RTMPSERVER_H
#define RTMPSERVER_H

#include <QTcpServer>
#include <QMap>
#include <QMutex>

class RTMPSession;
class StreamManager;

class RTMPServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit RTMPServer(quint16 port = 1935, QObject *parent = nullptr);
    ~RTMPServer();

    bool start();
    void stop();

    StreamManager* streamManager() const;

signals:
    void streamPublished(const QString &streamName);
    void streamUnpublished(const QString &streamName);

protected:
    void incomingConnection(qintptr handle) override;

private slots:
    void onSessionDisconnected(RTMPSession *session);

private:
    quint16 m_port;
    StreamManager *m_streamManager;
    QMap<qintptr, RTMPSession*> m_sessions;
    QMutex m_mutex;
};

#endif // RTMPSERVER_H
