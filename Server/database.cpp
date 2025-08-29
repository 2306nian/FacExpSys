// database.cpp
#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

Database *Database::m_instance = nullptr;

Database *Database::instance()
{
    if (!m_instance) {
        m_instance = new Database;
    }
    return m_instance;
}

Database::Database(QObject *parent)
    : QObject(parent)
{
}

bool Database::initialize(const QString &dbPath)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "server_conn");
    m_db.setDatabaseName(dbPath+"server.db");

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // 创建工单表
    QSqlQuery query(m_db);
    bool work_order_ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS work_orders (
            ticket_id TEXT PRIMARY KEY,
            client_username TEXT NOT NULL,
            client_ip TEXT NOT NULL,
            client_port INTEGER NOT NULL,
            created_at DATETIME NOT NULL,
            status TEXT NOT NULL CHECK(status IN ('pending', 'in_progress', 'completed')),
            expert_username TEXT,
            expert_ip TEXT,
            expert_port INTEGER,
            accepted_at DATETIME,
            feedback_description TEXT,
            feedback_solution TEXT,
            completed_at DATETIME
        )
    )");

    // 设备工单关联表
    QSqlQuery devQuery(m_db);
    bool work_order_devices_ok = devQuery.exec(R"(
        CREATE TABLE IF NOT EXISTS work_order_devices (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ticket_id TEXT NOT NULL,
            device_id TEXT NOT NULL,
            FOREIGN KEY (ticket_id) REFERENCES work_orders (ticket_id) ON DELETE CASCADE,
            UNIQUE(ticket_id, device_id)
        )
    )");

    // 用户信息表
    QSqlQuery userQuery(m_db);
    bool users_ok = userQuery.exec(R"(
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,           -- 用户名唯一
        password_hash TEXT NOT NULL,             -- 密码哈希（不要存明文！）
        user_type TEXT NOT NULL CHECK(user_type IN ('client', 'expert')),  -- 类型
        created_at DATETIME NOT NULL,            -- 注册时间
        last_login DATETIME                      -- 最后登录时间
        )
    )");

    if (!(work_order_ok && work_order_devices_ok && users_ok)) {
        qCritical() << "Failed to create table:" << query.lastError().text();
    } else {
        qDebug() << "work_orders table ready.";
    }

    return work_order_devices_ok && work_order_ok && users_ok;
}
