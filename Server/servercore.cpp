#include "servercore.h"
#include "clientsession.h"
#include "deviceproxy.h"
#include "common.h"
#include "qjsondocument.h"
#include "workordermanager.h"
#include <QDebug>
#include <QJsonArray>
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
void ServerCore::broadcastDeviceData(const QJsonArray &devicesData)
{
    QJsonObject msg{
        {"type", "device_realtime_update"},
         {"data", QJsonObject{{"devices", devicesData}}}
    };

    QByteArray payload = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    // 广播给所有在线客户端
    for (ClientSession *client : qAsConst(m_clients)) {
        if (client) {
            client->sendMessage(payload);
        }
    }
}

void ServerCore::broadcastTicketPending(const QString &ticketId, const QJsonObject &info)
{
    QJsonObject msg{
        {"type", "ticket_pending"},
        {"data", info}
    };
<<<<<<< HEAD
    qDebug()<<msg;
    qDebug()<<"test";
=======
>>>>>>> f8ee5bdb121191fe9ae19de22fe72f5383f73ae9
    QByteArray packet = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    for (ClientSession *client : qAsConst(m_clients)) {
        if (client) {
            client->sendMessage(packet);
            qDebug()<<"服务端已经发送";
        }
    }
    qDebug()<<packet;
}
