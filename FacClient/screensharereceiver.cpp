#include "screensharereceiver.h"
#include <QDebug>
#include <QImage>
#include <QBuffer>
#include <QDateTime>

ScreenShareReceiver::ScreenShareReceiver(Session* session, QLabel* displayLabel, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_displayLabel(displayLabel)
    , m_isReceiving(false)
    , m_frameCount(0)
    , m_lastTimestamp(0)
{
    if (m_displayLabel) {
        m_displayLabel->setAlignment(Qt::AlignCenter);
        m_displayLabel->setText("等待屏幕共享...");
        m_displayLabel->setStyleSheet("background-color: black; color: white; border: 1px solid gray;");
        m_displayLabel->setMinimumSize(640, 360);
    }
}

ScreenShareReceiver::~ScreenShareReceiver()
{
}

void ScreenShareReceiver::handleScreenShareMessage(const QJsonObject& message)
{
    QString type = message["type"].toString();

    if (type == "screensharedata") {
        QJsonObject data = message["data"].toObject();
        handleScreenShareData(data);
    }
    // 可以添加其他控制消息的处理
    // else if (type == "screenshare_stop") {
    //     m_isReceiving = false;
    //     if (m_displayLabel) {
    //         QJsonObject data = message["data"].toObject();
    //         int frameCount = data["frame_count"].toInt(0);
    //         m_displayLabel->setText(QString("屏幕共享已结束\n共接收 %1 帧").arg(frameCount));
    //     }
    //     emit sharingStopped();
    //     qDebug() << "Screen sharing stopped";
    // }
    // else if (type == "screenshare_pause") {
    //     if (m_displayLabel) {
    //         m_displayLabel->setText("屏幕共享已暂停");
    //     }
    //     qDebug() << "Screen sharing paused";
    // }
    // else if (type == "screenshare_resume") {
    //     if (m_displayLabel) {
    //         m_displayLabel->setText("正在接收屏幕共享...");
    //     }
    //     qDebug() << "Screen sharing resumed";
    // }
}

void ScreenShareReceiver::handleScreenShareData(const QJsonObject& data)
{
    try {
        // 提取数据
        qint64 timestamp = data["timestamp"].toVariant().toLongLong();
        int frameId = data["frame_id"].toInt();
        QString base64ImageData = data["image_data"].toString();
        int imageSize = data["image_size"].toInt();

        // 解码图像数据
        QByteArray imageData = QByteArray::fromBase64(base64ImageData.toLatin1());

        // // 验证数据大小（可选的安全检查）
        // if (imageData.size() != imageSize) {
        //     qWarning() << "Image data size mismatch:" << imageData.size() << "vs" << imageSize;
        //     // 可以选择继续处理或返回错误
        // }

        // 加载并显示图像
        QImage image;
        if (image.loadFromData(imageData, "JPG")) {
            // 第一帧时标记为开始接收
            if (!m_isReceiving) {
                m_isReceiving = true;
                m_frameCount = 0;
                emit sharingStarted();
                if (m_displayLabel) {
                    m_displayLabel->setText("正在接收屏幕共享...");
                }
                qDebug() << "Screen sharing started (receiver)";
            }

            m_frameCount++;
            m_lastTimestamp = timestamp;

            // 显示图像
            if (m_displayLabel) {
                QPixmap pixmap = QPixmap::fromImage(image);

                // 调整图像大小以适应显示区域（保持比例）
                if (pixmap.width() > m_displayLabel->width() ||
                    pixmap.height() > m_displayLabel->height()) {
                    pixmap = pixmap.scaled(m_displayLabel->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
                }

                m_displayLabel->setPixmap(pixmap); // 在pixmap上显示图像
            }

            emit frameReceived(); // TODO
        } else {
            qWarning() << "Failed to load image from data, frame:" << frameId;
            emit sharingError("无法加载图像数据"); // TODO
        }

    } catch (const std::exception& e) {
        qWarning() << "Error handling screen share data:" << e.what();
        emit sharingError(QString("处理屏幕数据时出错: %1").arg(e.what()));
    }
}
