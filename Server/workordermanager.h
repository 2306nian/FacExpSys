#ifndef WORKORDERMANAGER_H
#define WORKORDERMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QJsonDocument>

#include "workorderdao.h"

class WorkOrder;
class ClientSession;

class WorkOrderManager : public QObject
{
    Q_OBJECT

public:
    static WorkOrderManager *instance();

    QString createTicket(ClientSession *creator,
                         const QStringList &deviceIds,
                         const QString &clientUsername,
                         const QString &description);
    void sendNewTicketCreated(ClientSession *client, const QString ticketId);
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

    // 初始化客户端工单表
    void sendInitialWorkOrdersTo(ClientSession *client, const QString &username, const QString &userType);

    //查询工单统一接口
    QList<WorkOrderRecord> queryWorkOrders(const QString &username, const QString &userType, const QString &scope);
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
