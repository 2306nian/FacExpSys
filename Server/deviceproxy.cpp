#include "deviceproxy.h"
#include "clientsession.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include "workorder.h"

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
    connect(this,&DeviceProxy::deviceDataUpdated, this, &DeviceProxy::sendDeviceData);
    m_updateTimer->start(3000); // 每3秒更新一次
}

void DeviceProxy::sendDeviceData(const QJsonObject &deviceData)
{
    // WorkOrder *order = sender->currentTicket();
    // if (!order){
    //     qDebug() << "order not found!";
    //     return;
    // }

    // for (ClientSession *client : order->clients) {
    //     client->sendMessage(QJsonDocument(deviceData).toJson(QJsonDocument::Compact));
    // }
}

// void DeviceProxy::sendControlCommand(const QJsonObject &command)
// {
//     QString deviceId = command["device_id"].toString();
//     QString action = command["action"].toString();



//     qDebug() << "Control command sent to device" << deviceId << ":" << action;
//     // 这里可以添加实际的串口或网络控制代码
// }

void DeviceProxy::onUpdateTimer()
{
    static int counter = 0;

    QJsonObject deviceData;
    deviceData["type"] = "device_data";
    QJsonObject device_data{
        {"device_id","PLC_1001"},
        {"pressure", 80 + (qrand() % 20)},
        {"temperature", 50 + (qrand() % 20)},
        {"status", (counter++ % 5 == 0) ? "warning" : "normal"},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };
    deviceData["data"] = device_data;

    emit deviceDataUpdated(deviceData);
}

int DeviceProxy::qrand()
{
    // 使用 Qt5.10+ 推荐的随机数生成器
    return QRandomGenerator::global()->bounded(1, 101);  // [1, 100]
}
