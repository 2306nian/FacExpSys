#include "workorder.h"
#include "clientsession.h"

WorkOrder::WorkOrder(const QStringList &devIds)
    : deviceIds(devIds)
    , createdAt(QDateTime::currentDateTime())
    , isActive(true)
    , status(WORK_ORDER_PENDING)
{
    ticketId = "T" + createdAt.toString("yyyyMMddhhmmss");
}

void WorkOrder::addClient(ClientSession *client)
{
    if (!clients.contains(client))
        clients.append(client);
}

void WorkOrder::removeClient(ClientSession *client)
{
    clients.removeOne(client);
    if (clients.isEmpty())
        isActive = false;
}

bool WorkOrder::isEmpty() const
{
    return clients.isEmpty();
}
