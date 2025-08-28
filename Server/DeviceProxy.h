// DeviceProxy.h
#ifndef DEVICEPROXY_H
#define DEVICEPROXY_H

#include <QObject>
#include <QTimer>
#include <QSerialPort>

class ClientSession;

class DeviceProxy : public QObject
{
    Q_OBJECT
public:
    static DeviceProxy* instance();
    void requestDataFromDevice(ClientSession* requester, const QJsonObject& request);
    void sendControlCommand(const QJsonObject& cmd);

signals:
    void deviceDataUpdated(const QString& devId, const QJsonObject& data);

private slots:
    void onTimerUpdate(); // 模拟周期性采集
    void onSerialDataReceived(); // 串口数据

private:
    explicit DeviceProxy(QObject* parent = nullptr);
    static DeviceProxy* m_instance;
    QTimer* m_timer;
    QSerialPort* m_serial;
};

#endif // DEVICEPROXY_H
