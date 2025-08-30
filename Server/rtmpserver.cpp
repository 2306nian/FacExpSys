#include "rtmpserver.h"
#include "rtmpsession.h"
#include "streammanager.h"
#include <QTcpSocket>
#include <QDebug>

RTMPServer::RTMPServer(quint16 port, QObject *parent)
    : QTcpServer(parent)
    , m_port(port)
    , m_streamManager(new StreamManager(this))
{
}

RTMPServer::~RTMPServer()
{
    stop();
}

bool RTMPServer::start()
{
    if (!listen(QHostAddress::Any, m_port)) {
        qCritical() << "Failed to start RTMP server:" << errorString();
        return false;
    }

    qDebug() << "RTMP Server started on port" << m_port;
    return true;
}

void RTMPServer::stop()
{
    close();
    m_sessions.clear();
    qDebug() << "RTMP Server stopped";
}

StreamManager* RTMPServer::streamManager() const
{
    return m_streamManager;
}

void RTMPServer::incomingConnection(qintptr handle)
{
    QMutexLocker locker(&m_mutex);

    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(handle);

    RTMPSession *session = new RTMPSession(socket, m_streamManager, this);

    connect(session, &RTMPSession::disconnected,
            this, &RTMPServer::onSessionDisconnected);

    connect(session, &RTMPSession::streamPublished,
            this, &RTMPServer::streamPublished);

    connect(session, &RTMPSession::streamUnpublished,
            this, &RTMPServer::streamUnpublished);

    m_sessions[handle] = session;

    qDebug() << "New RTMP connection from:" << socket->peerAddress().toString();
}

void RTMPServer::onSessionDisconnected(RTMPSession *session)
{
    QMutexLocker locker(&m_mutex);

    qintptr handle = reinterpret_cast<qintptr>(session);
    m_sessions.remove(handle);

    qDebug() << "RTMP session disconnected";
    session->deleteLater();
}
