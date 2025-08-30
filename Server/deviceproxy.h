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

signals:
    void deviceDataUpdated(const QString &deviceId, const QJsonObject &data);

private slots:
    void onUpdateTimer();

private:
    explicit DeviceProxy(QObject *parent = nullptr);
    int getControlCount(const QString &deviceId);
    static DeviceProxy *m_instance;
    QTimer *m_updateTimer;
    int qrand();
};

#endif // DEVICEPROXY_H
