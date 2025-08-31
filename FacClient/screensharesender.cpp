#include "screensharesender.h"
#include <QDebug>
#include <QScreen>
#include <QApplication>
#include <QBuffer>
#include <QJsonObject>
#include <QPixmap>
#include <QJsonDocument>
#include <QDateTime>

ScreenCaptureWorker::ScreenCaptureWorker(QObject *parent)
    : QObject(parent)
    , m_captureInterval(100)  // 10 FPS
    , m_quality(70)           // 中等质量
{
    m_captureTimer = new QTimer(this);
    m_captureTimer->setInterval(m_captureInterval);
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenCaptureWorker::captureScreen);
}

void ScreenCaptureWorker::startCapture()
{
    m_captureTimer->start();
}

void ScreenCaptureWorker::stopCapture()
{
    m_captureTimer->stop();
}

void ScreenCaptureWorker::captureScreen()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) {
        emit error("无法获取屏幕");
        return;
    }

    // 捕获屏幕
    QPixmap pixmap = screen->grabWindow(0);

    // 调整大小以减少数据量
    int maxWidth = 1280;
    int maxHeight = 720;
    if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
        pixmap = pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // 转换为 JPG 格式
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);

    if (!pixmap.save(&buffer, "JPG", m_quality)) {
        emit error("图像保存失败");
        return;
    }

    emit screenCaptured(imageData);
}

ScreenShareSender::ScreenShareSender(Session* sender, QObject *parent) // 屏幕共享接口
    : QObject(parent)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
    , m_isSharing(false)
    , m_isPaused(false)
    , m_frameCount(0)
    , m_sender(sender)
    , m_captureInterval(100)  // 10 FPS
    , m_quality(70)           // 中等质量
{
}

ScreenShareSender::~ScreenShareSender()
{
    cleanup();
}

bool ScreenShareSender::startSharing()
{
    if (m_isSharing) {
        emit sharingError("屏幕共享已在进行中");
        return false;
    }

    try {
        initializeCapture();

        if (m_worker) {
            m_worker->startCapture();
            m_isSharing = true;
            m_isPaused = false;
            m_frameCount = 0;
            emit sharingStarted();
            qDebug() << "Screen sharing started";
            return true;
        }

    } catch (const std::exception& e) {
        qWarning() << "Failed to start screen sharing:" << e.what();
        cleanup();
        emit sharingError(QString("无法开始屏幕共享: %1").arg(e.what()));
        return false;
    }

    return false;
}

void ScreenShareSender::stopSharing()
{
    if (!m_isSharing) {
        return;
    }

    try {
        if (m_worker) {
            m_worker->stopCapture();
        }

        m_isSharing = false;
        m_isPaused = false;

        // TODO：发送停止消息

        emit sharingStopped();
        qDebug() << "TCP Screen sharing stopped. Total frames:" << m_frameCount;

    } catch (const std::exception& e) {
        qWarning() << "Error stopping screen sharing:" << e.what();
        emit sharingError(QString("停止屏幕共享时出错: %1").arg(e.what()));
    }
}

void ScreenShareSender::pauseSharing()
{
    if (!m_isSharing || m_isPaused) {
        return;
    }

    if (m_worker) {
        m_worker->stopCapture();
    }
    m_isPaused = true;

    // TODO：发送暂停消息
}

void ScreenShareSender::resumeSharing()
{
    if (!m_isSharing || !m_isPaused) {
        return;
    }

    if (m_worker) {
        m_worker->startCapture();
    }
    m_isPaused = false;

    // TODO：发送恢复消息
}

void ScreenShareSender::initializeCapture()
{
    // 创建工作线程
    m_workerThread = new QThread(this);
    m_worker = new ScreenCaptureWorker();

    m_worker->setCaptureInterval(m_captureInterval);
    m_worker->setQuality(m_quality);

    m_worker->moveToThread(m_workerThread);

    // 连接信号槽
    connect(m_workerThread, &QThread::started, m_worker, &ScreenCaptureWorker::startCapture);
    connect(m_worker, &ScreenCaptureWorker::screenCaptured, this, &ScreenShareSender::onScreenCaptured);
    connect(m_worker, &ScreenCaptureWorker::error, this, &ScreenShareSender::onWorkerError);
    connect(m_workerThread, &QThread::finished, m_worker, &ScreenCaptureWorker::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);

    m_workerThread->start();
}

void ScreenShareSender::onScreenCaptured(const QByteArray& imageData)
{
    if (!m_isSharing || m_isPaused) {
        return;
    }

    sendScreenData(imageData);
    m_frameCount++;
}

void ScreenShareSender::sendScreenData(const QByteArray& imageData)
{
    QJsonObject message;
    message["type"] = "screensharedata";

    QJsonObject data;
    data["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    data["frame_id"] = m_frameCount;
    // 将图像数据转换为 base64 字符串
    data["image_data"] = QString::fromLatin1(imageData.toBase64());
    data["image_size"] = imageData.size();

    message["data"] = data;

    // 转换为 JSON
    QJsonDocument doc(message);
    QByteArray screenShareData = doc.toJson(QJsonDocument::Compact);

    m_sender->sendMessage(screenShareData);
}

void ScreenShareSender::onWorkerError(const QString& error)
{
    qWarning() << "Screen capture error:" << error;
    emit sharingError(QString("屏幕捕获错误: %1").arg(error));
    stopSharing();
}

void ScreenShareSender::cleanup()
{
    if (m_worker) {
        m_worker->stopCapture();
    }

    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(3000);  // 等待最多3秒
    }
}
