#ifndef DEVICEPROXY_H
#define DEVICEPROXY_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>

class ClientSession;

class DeviceProxy : public QObject
{
    Q_OBJECT

public:
    static DeviceProxy *instance();
    void requestData(ClientSession *requester, const QJsonObject &request);
    void receiveControlCommand(const QJsonObject &command);
    int getControlCount(const QString &deviceId);

signals:
    void deviceDataUpdated(const QJsonArray &devicesData);

private slots:
    void onUpdateTimer();

private:
    explicit DeviceProxy(QObject *parent = nullptr);
    static DeviceProxy *m_instance;
    QTimer *m_updateTimer;
    int qrand();
};

#endif // DEVICEPROXY_H
