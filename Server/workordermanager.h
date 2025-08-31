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

    QString createTicket(ClientSession *creator,
                         const QStringList &deviceIds,
                         const QString &clientUsername);

    bool joinTicket(const QString &ticketId, ClientSession *client);
    void leaveTicket(ClientSession *client);
    void destroyTicket(const QString &ticketId);
    WorkOrder *getWorkOrder(const QString &ticketId);

    // 接受工单
    void acceptTicket(const QString &ticketId,
                      const QString &expertUsername,
                      const QString &expertIp,
                      int expertPort);

    // 完成工单
    void completeTicket(const QString &ticketId,
                        const QString &description,
                        const QString &solution);
signals:
    void ticketCreated(const QString &ticketId, const QString &deviceId);
    void ticketClosed(const QString &ticketId);
    void ticketPending(const QString &ticketId, const QJsonObject &ticketInfo);

private:
    explicit WorkOrderManager(QObject *parent = nullptr);
    static WorkOrderManager *m_instance;
    QMap<QString, WorkOrder *> m_tickets;
    QMutex m_mutex;
};

#endif // WORKORDERMANAGER_H
