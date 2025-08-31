#ifndef SCREENSHARESENDER_H
#define SCREENSHARESENDER_H

#include <QObject>
#include <QTimer>
#include <QScreen>
#include <QApplication>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include "session.h"

class ScreenCaptureWorker : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCaptureWorker(QObject *parent = nullptr);

    void startCapture();
    void stopCapture();
    void setCaptureInterval(int msec) { m_captureInterval = msec; }
    void setQuality(int quality) { m_quality = quality; }

public slots:
    void captureScreen();

signals:
    void screenCaptured(const QByteArray& imageData);
    void error(const QString& message);

private:
    QTimer* m_captureTimer;
    int m_captureInterval;
    int m_quality;
};

class ScreenShareSender : public QObject
{
    Q_OBJECT

public:
    explicit ScreenShareSender(Session *sender, QObject *parent = nullptr);
    ~ScreenShareSender();

    // 控制方法
    bool startSharing();
    void stopSharing();
    void pauseSharing();
    void resumeSharing();

    // 状态查询
    bool isSharing() const { return m_isSharing; }
    bool isPaused() const { return m_isPaused; }

signals:
    void sharingStarted();
    void sharingStopped();
    void sharingError(const QString& error);

private slots:
    void onScreenCaptured(const QByteArray& imageData);
    void onWorkerError(const QString& error);

private:
    void initializeCapture();
    void cleanup();
    void sendScreenData(const QByteArray& imageData);

    ScreenCaptureWorker* m_worker;
    QThread* m_workerThread;

    bool m_isSharing;
    bool m_isPaused;
    int m_frameCount;
    Session* m_sender;

    // 性能控制
    int m_captureInterval;  // 捕获间隔(毫秒)
    int m_quality;          // 图像质量(0-100)

    QMutex m_socketMutex;
};

#endif // ScreenShareSender_H
