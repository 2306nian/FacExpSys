#include "screensharemanager.h"
#include <QDebug>

ScreenShareManager* ScreenShareManager::m_instance = nullptr;

ScreenShareManager::ScreenShareManager(QObject *parent)
    : QObject(parent)
{
}

ScreenShareManager* ScreenShareManager::instance() // 单例实现
{
    if (!m_instance) {
        m_instance = new ScreenShareManager;
    }
    return m_instance;
}

bool ScreenShareManager::startScreenShare(ClientSession* sender, const QByteArray& screenShareData)
{
    // 查询ticketId
    WorkOrder *order = sender->currentTicket();
    QString ticketId = order->ticketId;

    if (!sender || ticketId.isEmpty()) {
        return false;
        qDebug()<<"ticketId is Empty!";
    }

    // 检查该工单是否已经在共享
    if (m_activeSessions.contains(ticketId)) {
        qWarning() << "Work order already has active screen share:" << ticketId;
        return false;
    }

    // 创建新的屏幕共享会话 方便管理共享进程
    ScreenShareSession session;
    session.sender = sender;
    session.ticketId = ticketId;
    session.isPaused = false;
    session.frameCount = 0;
    session.startTime = QDateTime::currentDateTime();

    m_activeSessions[ticketId] = session;
    m_sessionToWorkOrder[sender] = ticketId;

    qDebug() << "Screen share started for work order:" << ticketId;
    return true;
}

void ScreenShareManager::stopScreenShare(ClientSession* sender)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        qDebug()<<"No Screenshare Now!";
        return;
    }

    QString ticketId = m_sessionToWorkOrder[sender];

    if (m_activeSessions.contains(ticketId)) {
        m_activeSessions.remove(ticketId);
    }

    m_sessionToWorkOrder.remove(sender);

    qDebug() << "Screen share stopped for work order:" << ticketId;
}

void ScreenShareManager::pauseScreenShare(ClientSession* sender)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        qDebug()<<"No Screenshare Now!";
        return;
    }

    QString ticketId = m_sessionToWorkOrder[sender];

    if (m_activeSessions.contains(ticketId)) {
        m_activeSessions[ticketId].isPaused = true;
        qDebug() << "Screen share paused for work order:" << ticketId;
    }
}

void ScreenShareManager::resumeScreenShare(ClientSession* sender)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        qDebug()<<"No Screenshare Now!";
        return;
    }

    QString ticketId = m_sessionToWorkOrder[sender];

    if (m_activeSessions.contains(ticketId)) {
        m_activeSessions[ticketId].isPaused = false;
        qDebug() << "Screen share resumed for work order:" << ticketId;
    }
}

void ScreenShareManager::forwardScreenShareData(ClientSession* sender, const QByteArray& messageData)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        return;
    }

    QString ticketId = m_sessionToWorkOrder[sender];
    QJsonDocument doc = QJsonDocument::fromJson(messageData);

    QJsonObject message = doc.object();

    // 如果是第一个数据帧，自动开始共享
    QString type = message["type"].toString();
    if (type == "screensharedata" && !m_activeSessions.contains(ticketId)) {
        startScreenShare(sender, messageData);
    }

    // 更新帧计数
    if (type == "screensharedata" && m_activeSessions.contains(ticketId)) {
        m_activeSessions[ticketId].frameCount++;
    }

    // 获取对应的工单
    WorkOrder* workOrder = sender->m_currentTicket;
    if (!workOrder) {
        qWarning() << "Sender not in any work order";
        return;
    }

    // 转发给同一工单的其他客户端
    int forwardedCount = 0;
    for (ClientSession* client : workOrder->clients) {
        if (client && client != sender) {
            client->sendMessage(doc.toJson(QJsonDocument::Compact));
            forwardedCount++;
        }
    }

    if (type == "screensharedata") {
        qDebug() << "Forwarded screen share frame to" << forwardedCount << "clients in work order" << ticketId;
    }
}

bool ScreenShareManager::isScreenSharing(const QString& ticketId) const
{
    return m_activeSessions.contains(ticketId) &&
           !m_activeSessions[ticketId].isPaused;
}

ClientSession* ScreenShareManager::getCurrentSender(const QString& ticketId) const
{
    if (m_activeSessions.contains(ticketId)) {
        return m_activeSessions[ticketId].sender;
    }
    return nullptr;
}
