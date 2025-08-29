// workorderdao.h
#ifndef WORKORDERDAO_H
#define WORKORDERDAO_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>

// 简化版工单数据结构（用于数据库读写）
struct WorkOrderRecord {
    QString ticketId;               // 工单号
    QString clientUsername;         // 工厂客户端用户名称
    QString clientIp;               // 工单发起人的IP地址
    int clientPort;                 // 工单发起人的端口号
    QDateTime createdAt;            // 工单创建时间

    QString status;                 // 工单当前状态：pending（待接收）、in_progress（进行中）、completed（已完成）

    QString expertUsername;         // 承接该工单的专家用户名
    QString expertIp;               // 专家端的IP地址
    int expertPort;                 // 专家端的端口号
    QDateTime acceptedAt;           // 专家承接工单的时间

    QString feedbackDescription;    // 问题描述：工单完成后由专家填写，说明遇到的问题
    QString feedbackSolution;       // 解决办法：工单完成后由专家填写，说明如何解决的
    QDateTime completedAt;          // 工单完成的时间

    QStringList deviceIds;               // 出现故障的所有设备ID
};

class WorkOrderDAO : public QObject
{
    Q_OBJECT

public:
    static WorkOrderDAO *instance();

    // 1. 插入新工单（状态 pending）
    bool insertWorkOrder(const QString &ticketId,
                         const QString &clientUsername,
                         const QString &clientIp,
                         int clientPort,
                         const QDateTime &createdAt,
                         const QStringList &deviceIds);

    // 2. 专家承接工单（更新为 in_progress）
    bool acceptWorkOrder(const QString &ticketId,
                         const QString &expertUsername,
                         const QString &expertIp,
                         int expertPort,
                         const QDateTime &acceptedAt);

    // 3. 完成工单（更新为 completed，填写反馈）
    bool completeWorkOrder(const QString &ticketId,
                           const QString &description,
                           const QString &solution,
                           const QDateTime &completedAt);

    // 4. 查询某个工单的完整信息
    WorkOrderRecord getWorkOrder(const QString &ticketId);

    // 5. 获取所有工单（可用于管理界面）
    QList<WorkOrderRecord> getAllWorkOrders();

    // 6. 获取所有进行中的工单
    QList<WorkOrderRecord> getInProgressWorkOrders();

    // 7. 获取所有待处理的工单
    QList<WorkOrderRecord> getPendingWorkOrders();

private:
    explicit WorkOrderDAO(QObject *parent = nullptr);
    static WorkOrderDAO *m_instance;
};

#endif // WORKORDERDAO_H
