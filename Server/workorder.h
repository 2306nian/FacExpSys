#ifndef WORKORDER_H
#define WORKORDER_H

#include <QString>
#include <QList>
#include <QDateTime>

class ClientSession;

class WorkOrder
{
public:
    QString ticketId;
    QList<ClientSession*> clients;
    QStringList deviceIds;
    QDateTime createdAt;
    bool isActive;

    QString status; // 工单状态

    explicit WorkOrder(const QStringList &devIds);
    void addClient(ClientSession *client);
    void removeClient(ClientSession *client);
    bool isEmpty() const;
};

#endif // WORKORDER_H
