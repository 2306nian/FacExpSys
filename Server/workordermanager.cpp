#include "workordermanager.h"
#include "clientsession.h"
#include "workorder.h"
#include <QDebug>

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

QString WorkOrderManager::createTicket(const QString &deviceId)
{
    QMutexLocker locker(&m_mutex);

    WorkOrder *order = new WorkOrder(deviceId);
    m_tickets[order->ticketId] = order;

    emit ticketCreated(order->ticketId, deviceId);
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
