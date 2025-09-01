// devicedao.h
#ifndef DEVICE_DAO_H
#define DEVICE_DAO_H

#include "clientsession.h"

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QDateTime>

// 设备基本信息
struct DeviceBasicInfo {
    QString deviceId;           //设备Id
    QString name;               //设备名字
    QString type;               //设备类型
    QString location;           //设备位置
    QString manufacturer;       //出厂商
    QString model;              //设备型号
    QString onlineStatus;       //在线状态
    QDateTime lastHeartbeat;    //最后心跳时间
    QDateTime createdAt;        //注册时间
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

    // 用于登陆时初始化设备信息列表
    void sendDeviceListTo(ClientSession *client); // 由 DeviceDAO 主动发送

private:
    explicit DeviceDAO(QObject *parent = nullptr);
    static DeviceDAO *m_instance;
};

#endif // DEVICE_DAO_H
