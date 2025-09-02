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
                                       const QString &clientUsername,
                                       const QString &description)
{
    QMutexLocker locker(&m_mutex);

    QString clientIp = creator->clientIp();
    int clientPort = creator->clientPort();

    WorkOrder *order = new WorkOrder(deviceIds);
    qDebug()<<"工单号："+order->ticketId;
    m_tickets[order->ticketId] = order;
    //
    order->addClient(creator);
    creator->setCurrentTicket(order);

    bool ok = WorkOrderDAO::instance()->insertWorkOrder(
        order->ticketId,
        clientUsername,
        clientIp,
        clientPort,
        order->createdAt,
        deviceIds,
        description
        );

    if (!ok) {
        delete order;
        return QString();
    }

    // 获取该用户的所有工单
    QList<WorkOrderRecord> allOrders = WorkOrderDAO::instance()->getClientWorkOrders(clientUsername);
    qDebug() << allOrders.length();

    // 构造完整响应
    QJsonArray arr;
    for (const auto &r : allOrders) {
        arr.append(QJsonObject{
            {"ticket_id", r.ticketId},
            {"status", r.status},
            {"created_at", r.createdAt.toString(Qt::ISODate)},
            {"device_ids", QJsonArray::fromStringList(r.deviceIds)},
            {"description",description}
        });
    }

    QJsonObject response{
        {"type", "work_orders"},
        {"scope", "all"},
        {"data", arr}
    };
    QByteArray packet = QJsonDocument(response).toJson(QJsonDocument::Compact);
    creator->sendMessage(packet);

    // 依然 emit 信号（用于广播给专家端）
    emit ticketPending(order->ticketId, QJsonObject{
                                            {"ticket_id", order->ticketId},
                                            {"username", clientUsername},
                                            {"device_ids", QJsonArray::fromStringList(deviceIds)},
                                            {"created_at", order->createdAt.toString(Qt::ISODate)},
                                            {"description",description}
                                        });
    qDebug()<<"工单创建成功";

    return order->ticketId;
}

void sendNewTicketCreated(ClientSession *client, const QString ticketId){

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

    // 状态判断
    if (order->status != WORK_ORDER_PENDING) {
        qWarning() << "Ticket already handled:" << ticketId << "status:" << order->status;
        return;
    }

    // 更新状态
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

    // 通知双端该工单被承接
    QJsonObject notify{
        {"type", "ticket_accepted"},
        {"data", QJsonObject{
                     {"ticket_id", ticketId},
                     {"username", expertUsername},
                     {"acceptedAt", QDateTime::currentDateTime().toString(Qt::ISODate)},
                     {"status","in_progress"}
                 }}
    };

    QByteArray packet = QJsonDocument(notify).toJson(QJsonDocument::Compact);

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

    // 状态判断
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

    QByteArray packet =QJsonDocument(notify).toJson(QJsonDocument::Compact);

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

void WorkOrderManager::sendInitialWorkOrdersTo(ClientSession *client, const QString &username, const QString &userType)
{
    if (userType == "client") {
        QList<WorkOrderRecord> allOrders = WorkOrderDAO::instance()->getClientWorkOrders(username);
        QJsonArray arr;
        for (const auto &r : allOrders) {
            arr.append(QJsonObject{
                {"ticket_id", r.ticketId},
                {"status", r.status},
                {"created_at", r.createdAt.toString(Qt::ISODate)},
                {"device_ids", QJsonArray::fromStringList(r.deviceIds)}
            });
        }

        QJsonObject response{
            {"type", "work_orders_initial_client"},
            {"scope", "all"},
            {"data", arr}
        };
        client->sendMessage(QJsonDocument(response).toJson());
    }
    else if (userType == "expert") {
        QList<WorkOrderRecord> pendingOrders = WorkOrderDAO::instance()->getPendingWorkOrders();
        QJsonArray arr;
        for (const auto &r : pendingOrders) {
            arr.append(QJsonObject{
                {"ticket_id", r.ticketId},
                {"username", r.clientUsername},
                {"created_at", r.createdAt.toString(Qt::ISODate)},
                {"device_ids", QJsonArray::fromStringList(r.deviceIds)}
            });
        }

        QJsonObject response{
            {"type", "work_orders_initial_expert"},
            {"scope", "pending"},
            {"data", arr}
        };
        client->sendMessage(QJsonDocument(response).toJson());
    }
}

QList<WorkOrderRecord> WorkOrderManager::queryWorkOrders(const QString &username, const QString &userType, const QString &scope)
{
    QMutexLocker locker(&m_mutex);
    QList<WorkOrderRecord> records;

    if (userType == "client") {
        if (scope == "all") {
            records = WorkOrderDAO::instance()->getClientWorkOrders(username);
        } else if (scope == "pending") {
            records = WorkOrderDAO::instance()->getClientPendingWorkOrders(username);
        } else if (scope == "in_progress") {
            records = WorkOrderDAO::instance()->getClientInProgressWorkOrders(username);
        } else if (scope == "completed") {
            records = WorkOrderDAO::instance()->getClientCompletedWorkOrders(username);
        }
    } else if (userType == "expert") {
        if (scope == "pending") {
            records = WorkOrderDAO::instance()->getPendingWorkOrders();
        } else if (scope == "in_progress") {
            records = WorkOrderDAO::instance()->getExpertInProgressWorkOrders(username);
        } else if (scope == "completed") {
            records = WorkOrderDAO::instance()->getExpertCompletedWorkOrders(username);
        }
    }

    return records;
}
