// devicedao.cpp
#include "devicedao.h"
#include "database.h"
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonDocument>

DeviceDAO *DeviceDAO::m_instance = nullptr;

DeviceDAO *DeviceDAO::instance()
{
    if (!m_instance) {
        m_instance = new DeviceDAO;
    }
    return m_instance;
}

DeviceDAO::DeviceDAO(QObject *parent)
    : QObject(parent)
{
}

QList<DeviceBasicInfo> DeviceDAO::getAllDevices()
{
    QList<DeviceBasicInfo> list;
    QSqlQuery query(Database::instance()->db());
    query.prepare(R"(
        SELECT device_id, name, type, location, manufacturer, model,
               online_status, last_heartbeat, created_at
        FROM devices
        ORDER BY name
    )");

    if (query.exec()) {
        while (query.next()) {
            DeviceBasicInfo info;
            info.deviceId = query.value("device_id").toString();
            info.name = query.value("name").toString();
            info.type = query.value("type").toString();
            info.location = query.value("location").toString();
            info.manufacturer = query.value("manufacturer").toString();
            info.model = query.value("model").toString();
            info.onlineStatus = query.value("online_status").toString();
            info.lastHeartbeat = query.value("last_heartbeat").toDateTime();
            info.createdAt = query.value("created_at").toDateTime();
            list.append(info);
        }
    } else {
        qWarning() << "Failed to query devices:" << query.lastError().text();
    }
    return list;
}

QJsonObject DeviceDAO::getDeviceRealtime(const QString &deviceId)
{
    QSqlQuery query(Database::instance()->db());
    query.prepare(R"(
        SELECT pressure, temperature, status, last_update, control_count
        FROM device_realtime
        WHERE device_id = ?
    )");
    query.addBindValue(deviceId);

    if (query.exec() && query.next()) {
        return QJsonObject{
            {"device_id", deviceId},
            {"pressure", query.value("pressure").toDouble()},
            {"temperature", query.value("temperature").toDouble()},
            {"status", query.value("status").toString()},
            {"last_update", query.value("last_update").toDateTime().toString(Qt::ISODate)},
            {"control_count", query.value("control_conut").toInt()},
        };
    }
    return QJsonObject(); // 空表示未找到
}

QJsonArray DeviceDAO::getDeviceHistory(const QString &deviceId,
                                       const QDateTime &start,
                                       const QDateTime &end)
{
    QJsonArray array;
    QSqlQuery query(Database::instance()->db());
    query.prepare(R"(
        SELECT pressure, temperature, status, timestamp
        FROM device_history
        WHERE device_id = ? AND timestamp BETWEEN ? AND ?
        ORDER BY timestamp
    )");
    query.addBindValue(deviceId);
    query.addBindValue(start);
    query.addBindValue(end);

    if (query.exec()) {
        while (query.next()) {
            array.append(QJsonObject{
                {"pressure", query.value("pressure").toDouble()},
                {"temperature", query.value("temperature").toDouble()},
                {"status", query.value("status").toString()},
                {"timestamp", query.value("timestamp").toString()}
            });
        }
    }
    return array;
}

QJsonArray DeviceDAO::getDeviceLogs(const QString &deviceId, int limit)
{
    QJsonArray array;
    QSqlQuery query(Database::instance()->db());
    query.prepare(R"(
        SELECT level, message, timestamp
        FROM device_logs
        WHERE device_id = ?
        ORDER BY timestamp DESC
        LIMIT ?
    )");
    query.addBindValue(deviceId);
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            array.append(QJsonObject{
                {"level", query.value("level").toString()},
                {"message", query.value("message").toString()},
                {"timestamp", query.value("timestamp").toString()}
            });
        }
    }
    return array;
}

void DeviceDAO::sendDeviceListTo(ClientSession *client)
{
    QList<DeviceBasicInfo> devices = getAllDevices();
    QJsonArray arr;
    for (const auto &dev : devices) {
        QJsonObject realtime = getDeviceRealtime(dev.deviceId);
        arr.append(QJsonObject{
            {"device_id", dev.deviceId},
            {"name", dev.name},
            {"type", dev.type},
            {"location", dev.location},
            {"online_status", dev.onlineStatus},
            {"pressure", realtime["pressure"]},
            {"temperature", realtime["temperature"]},
            {"status", realtime["status"]},
            {"control_count",realtime["control_count"]}
        });
    }

    QJsonObject response{
        {"type", "device_list"},
        {"data", QJsonObject{{"devices", arr}}}
    };
    client->sendMessage(QJsonDocument(response).toJson());
}
