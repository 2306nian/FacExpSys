#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <QTcpServer>
#include <QList>
#include <QJsonObject>

class ClientSession;
class DeviceProxy;

class ServerCore : public QTcpServer
{
    Q_OBJECT

public:
    explicit ServerCore(quint16 port, QObject *parent = nullptr);

    // 获取所有客户端（用于广播）
    QList<ClientSession*> clients() const { return m_clients; }

protected:
    void incomingConnection(qintptr handle) override;

private slots:
    // 处理设备数据更新并广播
    void broadcastDeviceData(const QJsonArray &devicesData);
    // 广播工单创建
    void broadcastTicketPending(const QString &ticketId, const QJsonObject &info);

private:
    QList<ClientSession*> m_clients;
};

#endif // SERVERCORE_H
