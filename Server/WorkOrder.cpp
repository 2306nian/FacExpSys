#include "WorkOrder.h"
#include "ClientSession.h"

WorkOrder::WorkOrder(const QString& devId)
    : deviceId(devId), createdAt(QDateTime::currentDateTime()), isActive(true)
{
    ticketId = "T" + createdAt.toString("yyyyMMddhhmmss");
}

void WorkOrder::addClient(ClientSession* client)
{
    if (!clients.contains(client))
        clients.append(client);
}

void WorkOrder::removeClient(ClientSession* client)
{
    clients.removeOne(client);
    if (clients.isEmpty())
        isActive = false;
}

bool WorkOrder::isEmpty() const
{
    return clients.isEmpty();
}
