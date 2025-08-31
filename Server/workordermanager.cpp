#include "workordermanager.h"
#include "clientsession.h"
#include "common.h"
#include "workorder.h"
#include "workorderdao.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

WorkOrderManager *WorkOrderManager::m_instance = nullptr;

WorkOrderManager *WorkOrderManager::instance()
{
    if (!m_instance) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!m_instance)
            m_instance = new WorkOrderManager;
    }
    return m_instance;
}

WorkOrderManager::WorkOrderManager(QObject *parent)
    : QObject(parent)
{
}

QString WorkOrderManager::createTicket(ClientSession *creator,
                                       const QStringList &deviceIds,
                                       const QString &clientUsername)
{
    QMutexLocker locker(&m_mutex);

    // 从 session 获取真实 IP 和 Port
    QString clientIp = creator->clientIp();
    int clientPort = creator->clientPort();

    WorkOrder *order = new WorkOrder(deviceIds);
    m_tickets[order->ticketId] = order;

    // 调用 DAO，传真实连接信息
    bool ok = WorkOrderDAO::instance()->insertWorkOrder(
        order->ticketId,
        clientUsername,
        clientIp,
        clientPort,
        order->createdAt,
        deviceIds
        );

    if (!ok) {
        delete order;
        return QString();
    }

    QJsonObject ticketInfo{
        {"ticket_id", order->ticketId},
        {"username", clientUsername},
        {"device_ids", QJsonArray::fromStringList(deviceIds)},
        {"createdAt", order->createdAt.toString(Qt::ISODate)}
    };
    emit ticketPending(order->ticketId, ticketInfo);

    return order->ticketId;
}

bool WorkOrderManager::joinTicket(const QString &ticketId, ClientSession *client)
{
    QMutexLocker locker(&m_mutex);

    if (WorkOrder *order = m_tickets.value(ticketId)) {
        order->addClient(client);
        client->setCurrentTicket(order);
        return true;
    }
    return false;
}

void WorkOrderManager::leaveTicket(ClientSession *client)
{
    QMutexLocker locker(&m_mutex);

    WorkOrder *order = client->currentTicket();
    if (order) {
        order->removeClient(client);
        client->setCurrentTicket(nullptr);
        if (order->isEmpty()) {
            m_tickets.remove(order->ticketId);
            delete order;
            emit ticketClosed(order->ticketId);
        }
    }
}

void WorkOrderManager::destroyTicket(const QString &ticketId)
{
    QMutexLocker locker(&m_mutex);

    if (WorkOrder *order = m_tickets.take(ticketId)) {
        delete order;
        emit ticketClosed(ticketId);
    }
}

WorkOrder *WorkOrderManager::getWorkOrder(const QString &ticketId)
{
    QMutexLocker locker(&m_mutex);
    return m_tickets.value(ticketId);
}

void WorkOrderManager::acceptTicket(const QString &ticketId,
                                    const QString &expertUsername,
                                    const QString &expertIp,
                                    int expertPort)
{
    QMutexLocker locker(&m_mutex);

    WorkOrder *order = m_tickets.value(ticketId);
    if (!order) {
        qWarning() << "acceptTicket failed: ticket not found" << ticketId;
        return;
    }

    // 状态判断：只能从 pending → in_progress
    if (order->status != WORK_ORDER_PENDING) {
        qWarning() << "Ticket already handled:" << ticketId << "status:" << order->status;
        return;
    }

    // 更新内存状态
    order->status = WORK_ORDER_IN_PROGRESS;

    // 记录到数据库
    WorkOrderDAO::instance()->acceptWorkOrder(
        ticketId,
        expertUsername,
        expertIp,
        expertPort,
        QDateTime::currentDateTime()
        );

    qDebug() << "Ticket accepted:" << ticketId << "by" << expertUsername;

    // 通知工单创建者工单已经创建
    QJsonObject notify{
        {"type", "ticket_accepted"},
        {"data", QJsonObject{
                     {"ticket_id", ticketId},
                     {"username", expertUsername},
                     {"acceptedAt", QDateTime::currentDateTime().toString(Qt::ISODate)}
                 }}
    };
    QByteArray packet = packMessage(QJsonDocument(notify).toJson(QJsonDocument::Compact));

    // 发送给该工单内的所有客户端（主要是工厂端）
    for (ClientSession *client : order->clients) {
        if (client && client->socket()->state() == QAbstractSocket::ConnectedState) {
            client->sendMessage(packet);
        }
    }
}

void WorkOrderManager::completeTicket(const QString &ticketId,
                                      const QString &description,
                                      const QString &solution)
{
    QMutexLocker locker(&m_mutex);

    WorkOrder *order = m_tickets.value(ticketId);
    if (!order) {
        qWarning() << "completeTicket failed: ticket not found" << ticketId;
        return;
    }

    // 状态判断：只能从 in_progress → completed
    if (order->status != WORK_ORDER_IN_PROGRESS) {
        qWarning() << "Cannot complete ticket: invalid state" << order->status;
        return;
    }

    order->status = WORK_ORDER_COMPLETED;

    // 记录到数据库
    WorkOrderDAO::instance()->completeWorkOrder(
        ticketId,
        description,
        solution,
        QDateTime::currentDateTime()
        );

    // 广播给工单内所有客户端工单已结束
    QJsonObject notify{
        {"type", "ticket_completed"},
        {"data", QJsonObject{
                     {"ticket_id", ticketId},
                     {"description", description},
                     {"solution", solution},
                     {"completedAt", QDateTime::currentDateTime().toString(Qt::ISODate)}
                 }}
    };
    QByteArray packet = packMessage(QJsonDocument(notify).toJson(QJsonDocument::Compact));

    for (ClientSession *client : order->clients) {
        if (client && client->socket()->state() == QAbstractSocket::ConnectedState) {
            client->sendMessage(packet);
        }
    }

    // 自动销毁工单
    m_tickets.remove(ticketId);
    delete order;

    qDebug() << "Ticket completed:" << ticketId;
}
