// devicedao.h
#ifndef DEVICE_DAO_H
#define DEVICE_DAO_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QDateTime>

// 设备基本信息
struct DeviceBasicInfo {
    QString deviceId;
    QString name;
    QString type;
    QString location;
    QString manufacturer;
    QString model;
    QString onlineStatus;
    QDateTime lastHeartbeat;
    QDateTime createdAt;
};

class DeviceDAO : public QObject
{
    Q_OBJECT

public:
    static DeviceDAO *instance();

    // 1. 获取所有设备基本信息
    QList<DeviceBasicInfo> getAllDevices();

    // 2. 获取设备实时数据
    QJsonObject getDeviceRealtime(const QString &deviceId);

    // 3. 获取设备历史数据（用于曲线）
    QJsonArray getDeviceHistory(const QString &deviceId,
                                const QDateTime &start,
                                const QDateTime &end);

    // 4. 获取设备日志
    QJsonArray getDeviceLogs(const QString &deviceId, int limit = 50);

private:
    explicit DeviceDAO(QObject *parent = nullptr);
    static DeviceDAO *m_instance;
};

#endif // DEVICE_DAO_H
