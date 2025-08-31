// workorderdao.cpp
#include "workorderdao.h"
#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QJsonArray>

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

WorkOrderRecord WorkOrderDAO::getWorkOrderFromQuery(const QSqlQuery &query)
{
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

    // 查询设备
    QSqlQuery devQuery(Database::instance()->db());
    devQuery.prepare("SELECT device_id FROM work_order_devices WHERE ticket_id = ?");
    devQuery.addBindValue(r.ticketId);
    if (devQuery.exec()) {
        while (devQuery.next()) {
            r.deviceIds.append(devQuery.value(0).toString());
        }
    }
    return r;
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
    db.transaction();

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

    QSqlQuery devQuery(db);
    devQuery.prepare("INSERT OR IGNORE INTO work_order_devices (ticket_id, device_id) VALUES (?, ?)");
    devQuery.addBindValue(ticketId);
    for (const QString &devId : deviceIds) {
        devQuery.bindValue(1, devId);
        if (!devQuery.exec()) {
            qWarning() << "Failed to insert device:" << devId;
            db.rollback();
            return false;
        }
    }

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
        record = getWorkOrderFromQuery(query);
    }
    return record;
}

QList<WorkOrderRecord> WorkOrderDAO::getAllWorkOrders()
{
    QList<WorkOrderRecord> list;
    QSqlQuery query("SELECT * FROM work_orders ORDER BY created_at DESC", Database::instance()->db());
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getPendingWorkOrders()
{
    QList<WorkOrderRecord> list;
    QSqlQuery query("SELECT * FROM work_orders WHERE status = 'pending'", Database::instance()->db());
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

// 新增：工厂端查询
QList<WorkOrderRecord> WorkOrderDAO::getClientWorkOrders(const QString &clientUsername)
{
    QList<WorkOrderRecord> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE client_username = ? ORDER BY created_at DESC");
    query.addBindValue(clientUsername);
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getClientPendingWorkOrders(const QString &clientUsername)
{
    QList<WorkOrderRecord> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE client_username = ? AND status = 'pending' ORDER BY accepted_at DESC");
    query.addBindValue(clientUsername);
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getClientInProgressWorkOrders(const QString &clientUsername)
{
    QList<WorkOrderRecord> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE client_username = ? AND status = 'in_progress' ORDER BY accepted_at DESC");
    query.addBindValue(clientUsername);
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getClientCompletedWorkOrders(const QString &clientUsername)
{
    QList<WorkOrderRecord> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE client_username = ? AND status = 'completed' ORDER BY completed_at DESC");
    query.addBindValue(clientUsername);
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getExpertInProgressWorkOrders(const QString &expertUsername)
{
    QList<WorkOrderRecord> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE expert_username = ? AND status = 'in_progress' ORDER BY accepted_at DESC");
    query.addBindValue(expertUsername);
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}

QList<WorkOrderRecord> WorkOrderDAO::getExpertCompletedWorkOrders(const QString &expertUsername)
{
    QList<WorkOrderRecord> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT * FROM work_orders WHERE expert_username = ? AND status = 'completed' ORDER BY completed_at DESC");
    query.addBindValue(expertUsername);
    while (query.next()) {
        list.append(getWorkOrderFromQuery(query));
    }
    return list;
}
