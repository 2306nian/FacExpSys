#include "deviceproxy.h"
#include "clientsession.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include "database.h"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>

DeviceProxy *DeviceProxy::m_instance = nullptr;

DeviceProxy *DeviceProxy::instance()
{
    if (!m_instance) {
        m_instance = new DeviceProxy;
    }
    return m_instance;
}

DeviceProxy::DeviceProxy(QObject *parent)
    : QObject(parent)
{
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DeviceProxy::onUpdateTimer);
    m_updateTimer->start(3000); // 每3秒更新一次

    // 注册模拟设备（只执行一次）
    QSqlQuery query(Database::instance()->db());

    // 清空旧数据（测试用）
    query.exec("DELETE FROM devices WHERE device_id LIKE 'SIM_%'");
    query.exec("DELETE FROM device_realtime WHERE device_id LIKE 'SIM_%'");
    query.exec("DELETE FROM device_history WHERE device_id LIKE 'SIM_%'");

    // 注册3个模拟设备
    QList<QJsonObject> simDevices = {
        {{"id", "SIM_PLC_1001"}, {"name", "主控PLC"}, {"type", "PLC"}, {"loc", "车间A-1楼"}},
        {{"id", "SIM_SENSOR_2002"}, {"name", "温度传感器"}, {"type", "Sensor"}, {"loc", "车间A-2楼"}},
        {{"id", "SIM_MOTOR_3003"}, {"name", "主轴电机"}, {"type", "Motor"}, {"loc", "车间B-流水线1"}}
    };

    QDateTime now = QDateTime::currentDateTime();
    for (const auto &dev : simDevices) {
        QString id = dev["id"].toString();
        query.prepare(R"(
        INSERT OR IGNORE INTO devices (
            device_id, name, type, location, manufacturer, model, created_at, online_status
        ) VALUES (?, ?, ?, ?, ?, ?, ?, 'online')
    )");
        query.addBindValue(id);
        query.addBindValue(dev["name"].toString());
        query.addBindValue(dev["type"].toString());
        query.addBindValue(dev["loc"].toString());
        query.addBindValue("Simulated");
        query.addBindValue("SIM-2025");
        query.addBindValue(now);
        query.exec();
    }
}

/*void DeviceProxy::requestData(ClientSession *requester, const QJsonObject &request)
{
    QString deviceId = request["device_id"].toString();

    // 模拟设备数据
    QJsonObject data{
        {"pressure", 85.5 + (qrand() % 10 - 5)},
        {"temperature", 62.3 + (qrand() % 10 - 5)},
        {"status", "normal"},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"logs", QJsonArray{"System started", "No errors detected"}},
        {"faults", QJsonArray{}}
    };

    QJsonObject response{
        {"type", "device_data"},
        {"data", data}
    };

    requester->sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
}*/

void DeviceProxy::receiveControlCommand(const QJsonObject &command)
{
    QString deviceId = command["device_id"].toString();
    QString action = command["action"].toString();

    QJsonArray devicesData;

    qDebug() << "Control command sent to device" << deviceId << ":" << action;

    // 更新数据库：增加控制次数
    QSqlQuery query(Database::instance()->db());
    query.prepare("UPDATE devices SET control_count = control_count + 1 WHERE device_id = ?");
    query.addBindValue(deviceId);
    if (!query.exec()) {
        qWarning() << "Failed to update control count:" << query.lastError().text();
    }

    // 触发一次设备数据更新，推动客户端刷新
    // 模拟一次实时数据更新并广播
    devicesData.append(QJsonObject{
        {"device_id", deviceId},
        {"action", action},
        {"control_count", getControlCount(deviceId)}, // 自定义函数获取当前次数
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    });
    emit deviceDataUpdated(devicesData);
}


void DeviceProxy::onUpdateTimer()
{
    static int counter = 0;
    QSqlDatabase db = Database::instance()->db();
    QSqlQuery query(db);

    QStringList deviceIds = {"SIM_PLC_1001", "SIM_SENSOR_2002", "SIM_MOTOR_3003"};
    QDateTime now = QDateTime::currentDateTime();

    QJsonArray alldevicesData;

    for (const QString &deviceId : deviceIds) {
        double pressure = 80 + QRandomGenerator::global()->bounded(20);  // 80~100
        double temperature = 50 + QRandomGenerator::global()->bounded(20); // 50~70
        QString status = (counter % 5 == 0) ? "warning" : "normal";

        // 写入实时表（UPSERT）
        query.prepare(R"(
        INSERT OR REPLACE INTO device_realtime (
            device_id, pressure, temperature, status, last_update
        ) VALUES (?, ?, ?, ?, ?)
    )");
        query.addBindValue(deviceId);
        query.addBindValue(pressure);
        query.addBindValue(temperature);
        query.addBindValue(status);
        query.addBindValue(now);
        query.exec();

        // 写入历史表
        query.prepare(R"(
        INSERT INTO device_history (device_id, pressure, temperature, status, timestamp)
        VALUES (?, ?, ?, ?, ?)
    )");
        query.addBindValue(deviceId);
        query.addBindValue(pressure);
        query.addBindValue(temperature);
        query.addBindValue(status);
        query.addBindValue(now);
        query.exec();

        // 模拟日志（偶尔写一条）
        if (QRandomGenerator::global()->bounded(100) < 5) {
            query.prepare(R"(
            INSERT INTO device_logs (device_id, level, message, timestamp)
            VALUES (?, 'WARN', 'High temperature detected', ?)
        )");
            query.addBindValue(deviceId);
            query.addBindValue(now);
            query.exec();
        }

        // emit 给客户端
        alldevicesData.append( QJsonObject{
            {"device_id", deviceId},
            {"pressure", pressure},
            {"temperature", temperature},
            {"control_count", getControlCount(deviceId)},
            {"status", status},
            {"timestamp", now.toString(Qt::ISODate)}
        });
        emit deviceDataUpdated(alldevicesData);
    }

    counter++;
}

int DeviceProxy::qrand()
{
    // 使用 Qt5.10+ 推荐的随机数生成器
    return QRandomGenerator::global()->bounded(1, 101);  // [1, 100]
}

int DeviceProxy::getControlCount(const QString &deviceId)
{
    QSqlQuery query(Database::instance()->db());
    query.prepare("SELECT control_count FROM devices WHERE device_id = ?");
    query.addBindValue(deviceId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
