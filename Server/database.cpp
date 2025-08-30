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


    // 1. 设备基本信息
    bool devices_ok = query.exec(R"(
    CREATE TABLE IF NOT EXISTS devices (
        device_id TEXT PRIMARY KEY,
        name TEXT NOT NULL,
        type TEXT NOT NULL,
        location TEXT,
        manufacturer TEXT,
        model TEXT,
        ip_address TEXT,
        port INTEGER,
        protocol TEXT,
        online_status TEXT DEFAULT 'offline',
        last_heartbeat DATETIME,
        created_at DATETIME NOT NULL
    )
)");

    // 2. 实时数据（最新值）
    bool device_realtime_ok = query.exec(R"(
    CREATE TABLE IF NOT EXISTS device_realtime (
        device_id TEXT PRIMARY KEY,
        pressure REAL,
        temperature REAL,
        status TEXT,
        humidity REAL,
        vibration REAL,
        power_status TEXT,
        control_count INTEGER DEFAULT 0,
        last_update DATETIME NOT NULL,
        FOREIGN KEY (device_id) REFERENCES devices (device_id) ON DELETE CASCADE
    )
)");

    // 3. 历史数据（用于曲线）
    bool device_history_ok = query.exec(R"(
    CREATE TABLE IF NOT EXISTS device_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id TEXT NOT NULL,
        pressure REAL,
        temperature REAL,
        status TEXT,
        timestamp DATETIME NOT NULL,
        fault_code TEXT,
        FOREIGN KEY (device_id) REFERENCES devices (device_id) ON DELETE CASCADE
    )
)");

    // 4. 设备日志
    bool device_logs_ok = query.exec(R"(
    CREATE TABLE IF NOT EXISTS device_logs (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id TEXT NOT NULL,
        level TEXT NOT NULL,
        message TEXT NOT NULL,
        timestamp DATETIME NOT NULL,
        FOREIGN KEY (device_id) REFERENCES devices (device_id) ON DELETE CASCADE
    )
)");

    if (!(work_order_ok && work_order_devices_ok && users_ok && devices_ok
            && device_history_ok && device_logs_ok && device_realtime_ok)) {
        qCritical() << "Failed to create table:" << query.lastError().text();
    } else {
        qDebug() << "All the tables ready.";
    }

    return work_order_devices_ok && work_order_ok && users_ok && devices_ok
           && device_history_ok && device_logs_ok && device_realtime_ok;
}
