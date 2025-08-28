#ifndef WORKORDERMANAGER_H
#define WORKORDERMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>

class WorkOrder;
class ClientSession;

class WorkOrderManager : public QObject
{
    Q_OBJECT

public:
    static WorkOrderManager *instance();

    QString createTicket(const QString &deviceId);
    bool joinTicket(const QString &ticketId, ClientSession *client);
    void leaveTicket(ClientSession *client);
    void destroyTicket(const QString &ticketId);
    WorkOrder *getWorkOrder(const QString &ticketId);

signals:
    void ticketCreated(const QString &ticketId, const QString &deviceId);
    void ticketClosed(const QString &ticketId);

private:
    explicit WorkOrderManager(QObject *parent = nullptr);
    static WorkOrderManager *m_instance;
    QMap<QString, WorkOrder *> m_tickets;
    QMutex m_mutex;
};

#endif // WORKORDERMANAGER_H
