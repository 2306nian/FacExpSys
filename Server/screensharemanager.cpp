#include "screensharemanager.h"
#include <QDebug>

ScreenShareManager* ScreenShareManager::m_instance = nullptr;

ScreenShareManager::ScreenShareManager(QObject *parent)
    : QObject(parent)
{
}

ScreenShareManager* ScreenShareManager::instance() // 单例
{
    if (!m_instance) {
        m_instance = new ScreenShareManager;
    }
    return m_instance;
}

bool ScreenShareManager::startScreenShare(ClientSession* sender, const QString& workOrderId)
{
    if (!sender || workOrderId.isEmpty()) {
        return false;
    }

    // 检查该工单是否已经在共享
    if (m_activeSessions.contains(workOrderId)) {
        qWarning() << "Work order already has active screen share:" << workOrderId;
        return false;
    }

    // 创建新的屏幕共享会话 方便管理共享进程（非必须）
    ScreenShareSession session;
    session.sender = sender;
    session.workOrderId = workOrderId;
    session.isPaused = false;
    session.frameCount = 0;
    session.startTime = QDateTime::currentDateTime();

    m_activeSessions[workOrderId] = session;
    m_sessionToWorkOrder[sender] = workOrderId;

    qDebug() << "Screen share started for work order:" << workOrderId;
    return true;
}

void ScreenShareManager::stopScreenShare(ClientSession* sender)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        return;
    }

    QString workOrderId = m_sessionToWorkOrder[sender];

    if (m_activeSessions.contains(workOrderId)) {
        m_activeSessions.remove(workOrderId);
    }

    m_sessionToWorkOrder.remove(sender);

    qDebug() << "Screen share stopped for work order:" << workOrderId;
}

void ScreenShareManager::pauseScreenShare(ClientSession* sender)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        return;
    }

    QString workOrderId = m_sessionToWorkOrder[sender];

    if (m_activeSessions.contains(workOrderId)) {
        m_activeSessions[workOrderId].isPaused = true;
        qDebug() << "Screen share paused for work order:" << workOrderId;
    }
}

void ScreenShareManager::resumeScreenShare(ClientSession* sender)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        return;
    }

    QString workOrderId = m_sessionToWorkOrder[sender];

    if (m_activeSessions.contains(workOrderId)) {
        m_activeSessions[workOrderId].isPaused = false;
        qDebug() << "Screen share resumed for work order:" << workOrderId;
    }
}

void ScreenShareManager::forwardScreenShareData(ClientSession* sender, const QJsonObject& message)
{
    if (!sender || !m_sessionToWorkOrder.contains(sender)) {
        return;
    }

    QString workOrderId = m_sessionToWorkOrder[sender];

    // 如果是第一个数据帧，自动开始共享
    QString type = message["type"].toString();
    if (type == "screensharedata" && !m_activeSessions.contains(workOrderId)) {
        startScreenShare(sender, workOrderId);
    }

    // 更新帧计数
    if (type == "screensharedata" && m_activeSessions.contains(workOrderId)) {
        m_activeSessions[workOrderId].frameCount++;
    }

    // 获取对应的工单
    WorkOrder* workOrder = sender->currentTicket();
    if (!workOrder) {
        qWarning() << "Sender not in any work order";
        return;
    }

    // 转发给同一工单的其他客户端
    int forwardedCount = 0;
    for (ClientSession* client : workOrder->clients) {
        if (client && client != sender) {
            QJsonDocument doc(message);
            client->sendMessage(doc.toJson(QJsonDocument::Compact));
            forwardedCount++;
        }
    }

    if (type == "screensharedata") {
        qDebug() << "Forwarded screen share frame to" << forwardedCount << "clients in work order" << workOrderId;
    }
}

bool ScreenShareManager::isScreenSharing(const QString& workOrderId) const
{
    return m_activeSessions.contains(workOrderId) &&
           !m_activeSessions[workOrderId].isPaused;
}

ClientSession* ScreenShareManager::getCurrentSender(const QString& workOrderId) const
{
    if (m_activeSessions.contains(workOrderId)) {
        return m_activeSessions[workOrderId].sender;
    }
    return nullptr;
}
