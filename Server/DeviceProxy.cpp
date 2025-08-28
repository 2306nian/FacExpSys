// DeviceProxy.cpp
#include "DeviceProxy.h"
#include "ClientSession.h"
#include <QJsonDocument>

DeviceProxy* DeviceProxy::m_instance = nullptr;

DeviceProxy *DeviceProxy::instance()
{
    if (!m_instance)
        m_instance = new DeviceProxy;
    return m_instance;
}

DeviceProxy::DeviceProxy(QObject *parent)
    : QObject(parent), m_serial(nullptr)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DeviceProxy::onTimerUpdate);
    m_timer->start(2000); // 每2秒更新一次模拟数据
}

void DeviceProxy::requestDataFromDevice(ClientSession *requester, const QJsonObject &request)
{
    QString devId = request["device_id"].toString();
    // 模拟立即响应
    QJsonObject data{
        {"pressure", 85.5},
        {"temperature", 62.3},
        {"status", "normal"},
        {"logs", QJsonArray{"Log1", "Log2"}},
        {"faults", QJsonArray{}}
    };

    QJsonObject resp{
        {"type", "device_data"},
        {"data", data}
    };

    requester->sendMessage(QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

void DeviceProxy::sendControlCommand(const QJsonObject &cmd)
{
    QString devId = cmd["device_id"].toString();
    QString action = cmd["action"].toString();
    qInfo() << "Send control:" << devId << action;
    // 实际可通过串口或 TCP 发送
    if (m_serial && m_serial->isOpen()) {
        m_serial->write(action.toUtf8());
    }
}

void DeviceProxy::onTimerUpdate()
{
    static int counter = 0;
    QJsonObject fakeData{
        {"pressure", 80 + (qrand() % 20)},
        {"temperature", 50 + (qrand() % 20)},
        {"status", (counter++ % 5 == 0) ? "warning" : "normal"},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };
    emit deviceDataUpdated("PLC_1001", fakeData);
}
