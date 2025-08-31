// screensharereceiver.h
#ifndef SCREENSHARERECEIVER_H
#define SCREENSHARERECEIVER_H

#include <QObject>
#include <QLabel>
#include <QImage>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include "session.h"

class ScreenShareReceiver : public QObject
{
    Q_OBJECT

public:
    explicit ScreenShareReceiver(Session* session, QLabel* displayLabel, QObject *parent = nullptr);
    ~ScreenShareReceiver();

    // 控制方法
    void handleScreenShareMessage(const QJsonObject& message);

    // 状态查询
    bool isReceiving() const { return m_isReceiving; }
    int getFrameCount() const { return m_frameCount; }
    qint64 getLastTimestamp() const { return m_lastTimestamp; }

signals:
    void sharingStarted();
    void sharingStopped();
    void sharingError(const QString& error);
    void frameReceived();

private:
    void handleScreenShareData(const QJsonObject& data);

    Session* m_session;
    QLabel* m_displayLabel; // 这里是一个显示控件 按需添加在其他window中
    bool m_isReceiving;
    int m_frameCount;
    qint64 m_lastTimestamp;
};

#endif // SCREENSHARERECEIVER_H
