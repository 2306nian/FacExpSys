#ifndef WORKORDER_H
#define WORKORDER_H


#include <QString>
#include <QList>
#include <QDateTime>

class ClientSession;

struct WorkOrder {
    QString ticketId;
    QList<ClientSession*> clients;
    QString deviceId;
    QDateTime createdAt;
    bool isActive;

    explicit WorkOrder(const QString& devId);
    void addClient(ClientSession* client);
    void removeClient(ClientSession* client);
    bool isEmpty() const;
};


#endif // WORKORDER_H
