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
<<<<<<< HEAD
    int getControlCount(const QString &deviceId);
=======

>>>>>>> 449d4400eef9ef85c3987aa5c19d0f16aad235d9
signals:
    void deviceDataUpdated(const QString &deviceId, const QJsonObject &data);

private slots:
    void onUpdateTimer();

private:
    explicit DeviceProxy(QObject *parent = nullptr);
    static DeviceProxy *m_instance;
    QTimer *m_updateTimer;
    int qrand();
};

#endif // DEVICEPROXY_H
