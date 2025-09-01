#include "videoplayer.h"
#include <QDebug>
#include <QUrl>
#include <QPixmap>

VideoPlayer::VideoPlayer(int delay, QWidget *parent)
    : QWidget(parent)
    , m_ffmpegPlayer(new QProcess(this))
    , m_playButton(new QPushButton("Play", this))
    , m_stopButton(new QPushButton("Stop", this))
    , m_urlInput(new QLineEdit(this))
    , m_statusLabel(new QLabel("Ready", this))
    , m_videoDisplay(new QLabel(this))
    , m_displayTimer(new QTimer(this))
    , m_frameWidth(640)
    , m_frameHeight(480)
    , m_frameReady(false)
{
    setupUI();

    // 连接信号
    connect(m_playButton, &QPushButton::clicked, this, &VideoPlayer::onPlayButtonClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &VideoPlayer::onStopButtonClicked);

    connect(m_ffmpegPlayer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoPlayer::onPlayerFinished);

    connect(m_ffmpegPlayer, &QProcess::readyReadStandardOutput, this, &VideoPlayer::readVideoData);
    connect(m_ffmpegPlayer, &QProcess::readyReadStandardError, this, &VideoPlayer::readPlayerError);
    connect(m_ffmpegPlayer, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        qDebug() << "FFmpeg player error:" << error;
        m_statusLabel->setText("Error: " + QString::number(error));
    });

    // 设置显示定时器
    m_displayTimer->setInterval(33); // ~30 FPS
    connect(m_displayTimer, &QTimer::timeout, this, &VideoPlayer::updateDisplay);
}

VideoPlayer::~VideoPlayer()
{
    stopStream();
}

void VideoPlayer::setupUI()
{
    m_urlInput->setPlaceholderText("Enter RTMP URL...");
    m_videoDisplay->setStyleSheet("background-color: black;");
    m_videoDisplay->setMinimumSize(640, 480);
    m_videoDisplay->setAlignment(Qt::AlignCenter);
    m_videoDisplay->setText("No video");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 视频显示区域
    mainLayout->addWidget(m_videoDisplay, 1);

    // 控制区域
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->addWidget(m_urlInput);
    controlLayout->addWidget(m_playButton);
    controlLayout->addWidget(m_stopButton);

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_statusLabel);

    setLayout(mainLayout);
    setWindowTitle("Video Player");
    resize(800, 600);
}

void VideoPlayer::playStream(const QString &rtmpUrl)
{
    m_currentUrl = rtmpUrl;
    startFFmpegPlayer(rtmpUrl);
}

void VideoPlayer::startFFmpegPlayer(const QString &rtmpUrl)
{
    // 停止当前播放
    stopStream();

    QStringList arguments;

    // FFmpeg 参数：输出原始视频数据到管道
    arguments << "-fflags" << "nobuffer"              // 无缓冲
              << "-flags" << "low_delay"              // 低延迟标志
              << "-probesize" << "32"                 // 减少探测大小
              << "-analyzeduration" << "0"            // 不分析持续时间
              << "-i" << rtmpUrl                      // 输入RTMP流
              << "-vf" << QString("scale=%1:%2").arg(m_frameWidth).arg(m_frameHeight)  // 调整尺寸
              << "-vcodec" << "rawvideo"              // 输出原始视频
              << "-pix_fmt" << "rgb24"                // RGB24格式
              << "-f" << "rawvideo"                   // 原始视频格式
              << "-";                                 // 输出到标准输出

    qDebug() << "Starting FFmpeg with arguments:" << arguments;

    m_ffmpegPlayer->start("ffmpeg", arguments);

    if (m_ffmpegPlayer->waitForStarted(5000)) {
        m_statusLabel->setText("Playing: " + rtmpUrl);
        m_displayTimer->start();  // 启动显示定时器
        qDebug() << "FFmpeg player started successfully";
    } else {
        m_statusLabel->setText("Failed to start FFmpeg player");
        qDebug() << "Failed to start FFmpeg player";
    }
}

void VideoPlayer::stopStream()
{
    m_displayTimer->stop();

    if (m_ffmpegPlayer->state() == QProcess::Running) {
        m_ffmpegPlayer->terminate();
        if (!m_ffmpegPlayer->waitForFinished(3000)) {
            m_ffmpegPlayer->kill();
        }
        m_statusLabel->setText("Stopped");
        m_videoDisplay->setText("No video");
        qDebug() << "FFmpeg player stopped";
    }
}

void VideoPlayer::onPlayButtonClicked()
{
    QString url = m_urlInput->text().trimmed();
    if (!url.isEmpty()) {
        playStream(url);
    }
}

void VideoPlayer::onStopButtonClicked()
{
    stopStream();
}

void VideoPlayer::onPlayerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_displayTimer->stop();
    QString statusStr = (exitStatus == QProcess::NormalExit) ? "Normal" : "Crash";
    m_statusLabel->setText(QString("Player finished (Code: %1, Status: %2)").arg(exitCode).arg(statusStr));
    m_videoDisplay->setText("Stream ended");
    qDebug() << "FFmpeg player finished - Exit code:" << exitCode << "Status:" << statusStr;
}

void VideoPlayer::readVideoData()
{
    QByteArray newData = m_ffmpegPlayer->readAllStandardOutput();
    if (!newData.isEmpty()) {
        m_frameBuffer.append(newData);
        processRawVideoData(m_frameBuffer);
    }
}

void VideoPlayer::processRawVideoData(const QByteArray& data)
{
    int frameSize = m_frameWidth * m_frameHeight * 3; // RGB24: 3 bytes per pixel

    // 检查是否有完整的帧数据
    while (m_frameBuffer.size() >= frameSize) {
        // 提取一帧数据
        QByteArray frameData = m_frameBuffer.left(frameSize);
        m_frameBuffer = m_frameBuffer.mid(frameSize);

        // 转换为 QImage
        QImage frame((const uchar*)frameData.data(), m_frameWidth, m_frameHeight, QImage::Format_RGB888);

        if (!frame.isNull()) {
            // 更新显示
            QPixmap pixmap = QPixmap::fromImage(frame);
            m_videoDisplay->setPixmap(pixmap.scaled(m_videoDisplay->size(),
                                                    Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation));
            m_frameReady = true;
        }
    }
}

void VideoPlayer::updateDisplay()
{
    // 这个函数可以用于额外的显示更新逻辑
    // 目前数据已经在 processRawVideoData 中处理了
}

void VideoPlayer::readPlayerError()
{
    QByteArray error = m_ffmpegPlayer->readAllStandardError();
    if (!error.isEmpty()) {
        QString errorStr = QString::fromLocal8Bit(error);

        // 如果是致命错误，停止播放
        if (errorStr.contains("error", Qt::CaseInsensitive) ||
            errorStr.contains("failed", Qt::CaseInsensitive)) {
            qDebug() << "FFmpeg error:" << errorStr;
            m_statusLabel->setText("Error: " + errorStr.trimmed());
        }
    }
}
