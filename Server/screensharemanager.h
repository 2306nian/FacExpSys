#ifndef SCREENSHAREMANAGER_H
#define SCREENSHAREMANAGER_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>
#include "clientsession.h"
#include "workorder.h"

class ScreenShareManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenShareManager(QObject *parent = nullptr);
    static ScreenShareManager* instance();

    // 屏幕共享控制
    bool startScreenShare(ClientSession* sender, const QString& workOrderId);
    void stopScreenShare(ClientSession* sender);
    void pauseScreenShare(ClientSession* sender);
    void resumeScreenShare(ClientSession* sender);

    // 数据转发
    void forwardScreenShareData(ClientSession* sender, const QJsonObject& message);

    // 状态查询
    bool isScreenSharing(const QString& workOrderId) const;
    ClientSession* getCurrentSender(const QString& workOrderId) const;

private:
    struct ScreenShareSession {
        ClientSession* sender;
        QString workOrderId;
        bool isPaused;
        int frameCount;
        QDateTime startTime;
    };

    QMap<QString, ScreenShareSession> m_activeSessions;  // key: workOrderId
    QMap<ClientSession*, QString> m_sessionToWorkOrder;  // 快速查找

    static ScreenShareManager* m_instance;
};

#endif // SCREENSHAREMANAGER_H
