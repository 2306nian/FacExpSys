#include "rtmpsession.h"
#include "streammanager.h"
#include <QDebug>

RTMPSession::RTMPSession(QTcpSocket *socket, StreamManager *streamManager, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_streamManager(streamManager)
    , m_timeoutTimer(new QTimer(this))
    , m_handshakeDone(false)
    , m_isPublishing(false)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &RTMPSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &RTMPSession::onDisconnected);

    connect(m_timeoutTimer, &QTimer::timeout, this, &RTMPSession::onTimeout);
    m_timeoutTimer->start(30000); // 30秒超时
}

RTMPSession::~RTMPSession()
{
    if (m_socket) {
        m_socket->deleteLater();
    }
}

void RTMPSession::onReadyRead()
{
    m_timeoutTimer->start(30000); // 重置超时

    m_buffer.append(m_socket->readAll());

    if (!m_handshakeDone) {
        handleRTMPHandshake();
    } else {
        handleRTMPMessage();
    }
}

void RTMPSession::onDisconnected()
{
    if (m_isPublishing && !m_streamName.isEmpty()) {
        m_streamManager->unpublishStream(m_streamName, this);
        emit streamUnpublished(m_streamName);
    }

    emit disconnected(this);
}

void RTMPSession::onTimeout()
{
    qDebug() << "RTMP session timeout";
    m_socket->disconnectFromHost();
}

void RTMPSession::handleRTMPHandshake()
{
    // 简化的RTMP握手处理
    if (m_buffer.size() >= 1537) { // C0 + C1
        // 发送S0, S1, S2
        QByteArray response;
        response.append('\x03'); // S0 - RTMP版本3

        // S1 - 4字节时间戳 + 4字节0 + 1528字节随机数据
        response.append(m_buffer.mid(1, 1536)); // 使用客户端的C1作为S1

        // S2 - 回显客户端的C1
        response.append(m_buffer.mid(1, 1536));

        m_socket->write(response);
        m_buffer.remove(0, 1537);
        m_handshakeDone = true;

        qDebug() << "RTMP handshake completed";
    }
}

void RTMPSession::handleRTMPMessage()
{
    // 这里需要实现完整的RTMP协议解析
    // 包括连接、创建流、发布等命令处理

    // 简化示例：检测publish命令
    if (m_buffer.contains("publish")) {
        // 解析流名
        // 这里需要完整的AMF解析

        m_isPublishing = true;
        // m_streamName = parsedStreamName;

        if (!m_streamName.isEmpty()) {
            m_streamManager->publishStream(m_streamName, this);
            emit streamPublished(m_streamName);
        }
    }

    // 处理音视频数据
    // 转发给StreamManager进行分发
}
