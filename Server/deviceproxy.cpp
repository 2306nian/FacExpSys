#include "deviceproxy.h"
#include "clientsession.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>

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
}

void DeviceProxy::requestData(ClientSession *requester, const QJsonObject &request)
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
}

void DeviceProxy::sendControlCommand(const QJsonObject &command)
{
    QString deviceId = command["device_id"].toString();
    QString action = command["action"].toString();

    qDebug() << "Control command sent to device" << deviceId << ":" << action;
    // 这里可以添加实际的串口或网络控制代码
}

void DeviceProxy::onUpdateTimer()
{
    static int counter = 0;

    QJsonObject data{
        {"pressure", 80 + (qrand() % 20)},
        {"temperature", 50 + (qrand() % 20)},
        {"status", (counter++ % 5 == 0) ? "warning" : "normal"},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };

    emit deviceDataUpdated("PLC_1001", data);
}

int DeviceProxy::qrand()
{
    // 使用 Qt5.10+ 推荐的随机数生成器
    return QRandomGenerator::global()->bounded(1, 101);  // [1, 100]
}
