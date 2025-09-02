// workorderdao.h
#ifndef WORKORDERDAO_H
#define WORKORDERDAO_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QSqlQuery>

struct WorkOrderRecord {
    QString ticketId;
    QString clientUsername;
    QString clientIp;
    int clientPort;
    QDateTime createdAt;
    QString status;
    QString expertUsername;
    QString expertIp;
    int expertPort;
    QDateTime acceptedAt;
    QString feedbackDescription;
    QString feedbackSolution;
    QDateTime completedAt;
    QStringList deviceIds;
};

class WorkOrderDAO : public QObject
{
    Q_OBJECT

public:
    static WorkOrderDAO *instance();

    bool insertWorkOrder(const QString &ticketId,
                         const QString &clientUsername,
                         const QString &clientIp,
                         int clientPort,
                         const QDateTime &createdAt,
                         const QStringList &deviceIds,
                         const QString &feedback_description);

    bool acceptWorkOrder(const QString &ticketId,
                         const QString &expertUsername,
                         const QString &expertIp,
                         int expertPort,
                         const QDateTime &acceptedAt);

    bool completeWorkOrder(const QString &ticketId,
                           const QString &description,
                           const QString &solution,
                           const QDateTime &completedAt);

    WorkOrderRecord getWorkOrder(const QString &ticketId);

    QList<WorkOrderRecord> getAllWorkOrders();
    QList<WorkOrderRecord> getPendingWorkOrders();

    // 新增：按用户查询
    QList<WorkOrderRecord> getClientWorkOrders(const QString &clientUsername);
    QList<WorkOrderRecord> getClientPendingWorkOrders(const QString &clientUsername);
    QList<WorkOrderRecord> getClientInProgressWorkOrders(const QString &clientUsername);
    QList<WorkOrderRecord> getClientCompletedWorkOrders(const QString &clientUsername);
    QList<WorkOrderRecord> getExpertInProgressWorkOrders(const QString &expertUsername);
    QList<WorkOrderRecord> getExpertCompletedWorkOrders(const QString &expertUsername);

private:
    explicit WorkOrderDAO(QObject *parent = nullptr);
    static WorkOrderDAO *m_instance;

    // 内部：从 query 构造 record
    WorkOrderRecord getWorkOrderFromQuery(const QSqlQuery &query);
};

#endif // WORKORDERDAO_H
