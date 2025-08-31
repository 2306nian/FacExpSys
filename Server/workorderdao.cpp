// workorderdao.cpp
#include "workorderdao.h"
#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

WorkOrderDAO *WorkOrderDAO::m_instance = nullptr;

WorkOrderDAO *WorkOrderDAO::instance()
{
    if (!m_instance) {
        m_instance = new WorkOrderDAO;
    }
    return m_instance;
}

WorkOrderDAO::WorkOrderDAO(QObject *parent)
    : QObject(parent)
{
}

bool WorkOrderDAO::insertWorkOrder(const QString &ticketId,
                                   const QString &clientUsername,
                                   const QString &clientIp,
                                   int clientPort,
                                   const QDateTime &createdAt,
                                   const QStringList &deviceIds)
{
    QSqlDatabase db = Database::instance()->db();
    QSqlQuery query(db);

    // 开启事务（保证：工单和设备绑定要么全成功，要么全失败）
    db.transaction();

    // 1. 插入工单主表
    query.prepare(R"(
        INSERT INTO work_orders (
            ticket_id, client_username, client_ip, client_port, created_at, status
        ) VALUES (?, ?, ?, ?, ?, 'pending')
    )");
    query.addBindValue(ticketId);
    query.addBindValue(clientUsername);
    query.addBindValue(clientIp);
    query.addBindValue(clientPort);
    query.addBindValue(createdAt);

    if (!query.exec()) {
        qWarning() << "Failed to insert work order:" << query.lastError().text();
        db.rollback();
        return false;
    }

    // 2. 插入关联表（每个设备一条记录）
    QSqlQuery devQuery(db);
    devQuery.prepare("INSERT OR IGNORE INTO work_order_devices (ticket_id, device_id) VALUES (?, ?)");
    devQuery.addBindValue(ticketId);  // ticket_id 固定
    for (const QString &devId : deviceIds) {
        devQuery.bindValue(1, devId);  // 绑定 device_id
        if (!devQuery.exec()) {
            qWarning() << "Failed to insert device:" << devId << devQuery.lastError().text();
            db.rollback();
            return false;
        }
    }

    // 提交事务
    db.commit();
    qDebug() << "Work order created with devices:" << deviceIds;
    return true;
}

bool WorkOrderDAO::acceptWorkOrder(const QString &ticketId,
                                   const QString &expertUsername,
                                   const QString &expertIp,
                                   int expertPort,
                                   const QDateTime &acceptedAt)
{
    QSqlQuery query(Database::instance()->db());
    query.prepare(R"(
        UPDATE work_orders SET
            status = 'in_progress',
            expert_username = ?,
            expert_ip = ?,
            expert_port = ?,
            accepted_at = ?
        WHERE ticket_id = ? AND status = 'pending'
    )");
    query.addBindValue(expertUsername);
    query.addBindValue(expertIp);
    query.addBindValue(expertPort);
    query.addBindValue(acceptedAt);
    query.addBindValue(ticketId);

    if (!query.exec()) {
        qWarning() << "Failed to accept work order:" << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        qWarning() << "No work order found to accept:" << ticketId;
        return false;
    }
    qDebug() << "Work order accepted:" << ticketId;
    return true;
}

bool WorkOrderDAO::completeWorkOrder(const QString &ticketId,
                                     const QString &description,
                                     const QString &solution,
                                     const QDateTime &completedAt)
{
    QSqlQuery query(Database::instance()->db());
    query.prepare(R"(
        UPDATE work_orders SET
            status = 'completed',
            feedback_description = ?,
            feedback_solution = ?,
            completed_at = ?
        WHERE ticket_id = ? AND status = 'in_progress'
    )");
    query.addBindValue(description);
    query.addBindValue(solution);
    query.addBindValue(completedAt);
    query.addBindValue(ticketId);

    if (!query.exec()) {
        qWarning() << "Failed to complete work order:" << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        qWarning() << "No work order found to complete:" << ticketId;
        return false;
    }
    qDebug() << "Work order completed:" << ticketId;
    return true;
}

WorkOrderRecord WorkOrderDAO::getWorkOrder(const QString &ticketId)
{
    WorkOrderRecord record;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE ticket_id = ?");
    query.addBindValue(ticketId);

    if (query.exec() && query.next()) {
        // ... 原有字段赋值 ...
        record.ticketId = query.value("ticket_id").toString();
        record.clientUsername = query.value("client_username").toString();
        record.clientIp = query.value("client_ip").toString();
        record.clientPort = query.value("client_port").toInt();
        record.createdAt = query.value("created_at").toDateTime();
        record.status = query.value("status").toString();
        record.expertUsername = query.value("expert_username").toString();
        record.expertIp = query.value("expert_ip").toString();
        record.expertPort = query.value("expert_port").toInt();
        record.acceptedAt = query.value("accepted_at").toDateTime();
        record.feedbackDescription = query.value("feedback_description").toString();
        record.feedbackSolution = query.value("feedback_solution").toString();
        record.completedAt = query.value("completed_at").toDateTime();

        // 查询这个工单关联的所有设备
        QSqlQuery devQuery(Database::instance()->db());
        devQuery.prepare("SELECT device_id FROM work_order_devices WHERE ticket_id = ?");
        devQuery.addBindValue(ticketId);
        if (devQuery.exec()) {
            while (devQuery.next()) {
                record.deviceIds.append(devQuery.value(0).toString());
            }
        }
    }

    return record;
}

QList<WorkOrderRecord> WorkOrderDAO::getAllWorkOrders()
{
    QList<WorkOrderRecord> list;
    QSqlQuery query("SELECT * FROM work_orders ORDER BY created_at DESC", Database::instance()->db());

    while (query.next()) {
        WorkOrderRecord r;
        r.ticketId = query.value("ticket_id").toString();
        r.clientUsername = query.value("client_username").toString();
        r.clientIp = query.value("client_ip").toString();
        r.clientPort = query.value("client_port").toInt();
        r.createdAt = query.value("created_at").toDateTime();
        r.status = query.value("status").toString();
        r.expertUsername = query.value("expert_username").toString();
        r.expertIp = query.value("expert_ip").toString();
        r.expertPort = query.value("expert_port").toInt();
        r.acceptedAt = query.value("accepted_at").toDateTime();
        r.feedbackDescription = query.value("feedback_description").toString();
        r.feedbackSolution = query.value("feedback_solution").toString();
        r.completedAt = query.value("completed_at").toDateTime();
        list.append(r);
    }

    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getInProgressWorkOrders()
{
    QList<WorkOrderRecord> list;
    QSqlQuery query("SELECT * FROM work_orders WHERE status = 'in_progress'", Database::instance()->db());
    while (query.next()) {
        WorkOrderRecord r;
        // ... 同上赋值
        r.ticketId = query.value("ticket_id").toString();
        r.clientUsername = query.value("client_username").toString();
        r.clientIp = query.value("client_ip").toString();
        r.clientPort = query.value("client_port").toInt();
        r.createdAt = query.value("created_at").toDateTime();
        r.status = query.value("status").toString();
        r.expertUsername = query.value("expert_username").toString();
        r.expertIp = query.value("expert_ip").toString();
        r.expertPort = query.value("expert_port").toInt();
        r.acceptedAt = query.value("accepted_at").toDateTime();
        list.append(r);
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getPendingWorkOrders()
{
    QList<WorkOrderRecord> list;
    QSqlQuery query("SELECT * FROM work_orders WHERE status = 'pending'", Database::instance()->db());
    while (query.next()) {
        WorkOrderRecord r;
        r.ticketId = query.value("ticket_id").toString();
        r.clientUsername = query.value("client_username").toString();
        r.clientIp = query.value("client_ip").toString();
        r.clientPort = query.value("client_port").toInt();
        r.createdAt = query.value("created_at").toDateTime();
        list.append(r);
    }
    return list;
}

