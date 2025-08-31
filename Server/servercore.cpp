#include "servercore.h"
#include "clientsession.h"
#include "deviceproxy.h"
#include "common.h"
#include "qjsondocument.h"
#include "workordermanager.h"
#include <QDebug>
#include <QJsonDocument>

ServerCore::ServerCore(quint16 port, QObject *parent)
    : QTcpServer(parent)
{
    if (!listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server:" << errorString();
        return;
    }

    // 连接 DeviceProxy 的信号到本类的槽函数
    connect(DeviceProxy::instance(), &DeviceProxy::deviceDataUpdated,
            this, &ServerCore::broadcastDeviceData);

    // 连接工单创建成功的推送信号
    connect(WorkOrderManager::instance(), &WorkOrderManager::ticketPending,
            this, &ServerCore::broadcastTicketPending);

    qDebug() << "Server started on port" << port;
}

void ServerCore::incomingConnection(qintptr handle)
{
    qDebug() << "Incoming Connection...";
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(handle);

    ClientSession *session = new ClientSession(socket, this);
    m_clients.append(session);  // 添加到列表

    connect(session, &ClientSession::disconnected, this, [this, session]() {
        m_clients.removeOne(session);
        session->deleteLater();
        qDebug() << "Client disconnected. Remaining clients:" << m_clients.size();
    });

    qDebug() << "New client connected:" << socket->peerAddress().toString();
}

// 广播设备实时数据给所有客户端
void ServerCore::broadcastDeviceData(const QString &deviceId, const QJsonObject &data)
{
    // 构造 JSON 消息
    QJsonObject msg{
        {"type", "device_realtime_update"},
        {"data", data}
    };

    // 序列化并添加长度头
    QByteArray payload = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    QByteArray packet = packMessage(payload);  // 使用 common.h 中的 packMessage

    // 广播给所有在线客户端
    for (ClientSession *client : qAsConst(m_clients)) {
        if (client && client->socket()->state() == QAbstractSocket::ConnectedState) {
            client->socket()->write(packet);
            client->socket()->flush();  // 立即发送
        }
    }

    qDebug() << "Broadcasted device data:" << deviceId;
}

void ServerCore::broadcastTicketPending(const QString &ticketId, const QJsonObject &info)
{
    QJsonObject msg{
        {"type", "ticket_pending"},
        {"data", info}
    };
    QByteArray packet = packMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));

    for (ClientSession *client : qAsConst(m_clients)) {
        if (client->socket()->state() == QAbstractSocket::ConnectedState) {
            client->socket()->write(packet);
        }
    }
}
