#include "videoplayer.h"
#include <QUrl>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>

VideoPlayer::VideoPlayer(int delay, QWidget *parent)
    : QWidget(parent)
    , m_delay(delay)
    , m_maxDelay(50)  // 最大延迟3秒
    , m_lowLatencyMode(true)
    , m_player(new QMediaPlayer(this))
    , m_videoWidget(new QVideoWidget(this))
    , m_playButton(new QPushButton("Play", this))
    , m_stopButton(new QPushButton("Stop", this))
    , m_urlInput(new QLineEdit(this))
    , m_syncTimer(new QTimer(this))
    , m_lowLatencyCheckBox(new QCheckBox("Low Latency", this))
{
    m_player->setVideoOutput(m_videoWidget);

    // 配置同步定时器
    m_syncTimer->setInterval(10);  // 每10ms检查一次
    connect(m_syncTimer, &QTimer::timeout, this, &VideoPlayer::updateLiveSync);

    // UI 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *controlLayout = new QHBoxLayout();
    QHBoxLayout *optionLayout = new QHBoxLayout();

    m_urlInput->setPlaceholderText("Enter RTMP URL...");
    m_lowLatencyCheckBox->setChecked(true);

    controlLayout->addWidget(m_urlInput);
    controlLayout->addWidget(m_playButton);
    controlLayout->addWidget(m_stopButton);

    optionLayout->addWidget(m_lowLatencyCheckBox);
    optionLayout->addStretch();

    mainLayout->addWidget(m_videoWidget);
    mainLayout->addLayout(controlLayout);
    mainLayout->addLayout(optionLayout);

    setLayout(mainLayout);

    // 连接信号
    connect(m_playButton, &QPushButton::clicked, this, &VideoPlayer::onPlayButtonClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &VideoPlayer::onStopButtonClicked);
    connect(m_lowLatencyCheckBox, &QCheckBox::toggled, this, &VideoPlayer::setLowLatencyMode);

    // 播放请求处理
    connect(this, &VideoPlayer::playRequested, this, [this](const QString &url) {
        qDebug() << "开始从" << url << "拉流";

        // 配置播放器为低延迟模式
        if (m_lowLatencyMode) {
            configureLowLatency();
        }

        // 连接信号
        connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &VideoPlayer::onMediaStatusChanged);
        connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &VideoPlayer::onPlayerError);

        // 开始播放
        m_player->setMedia(QUrl(url));
        m_player->play();

        // 启动同步定时器
        m_syncTimer->start();
    });

    connect(this, &VideoPlayer::stopRequested, this, [this]() {
        m_syncTimer->stop();
        m_player->stop();
    });
}

void VideoPlayer::configureLowLatency()
{
    // 设置 QMediaPlayer 的低延迟属性（如果支持）
    m_player->setProperty("lowlatency", true);
    m_player->setProperty("bufferSize", 512);
    m_player->setProperty("maxBufferTime", 1000);  // 最大缓冲时间1秒
    m_player->setProperty("networkTimeout", 3000); // 网络超时3秒

    qDebug() << "配置播放器为低延迟模式";
}

void VideoPlayer::playStream(const QString &rtmpUrl)
{
    qDebug() << "播放流:" << rtmpUrl;
    emit playRequested(rtmpUrl);
}

void VideoPlayer::stopStream()
{
    emit stopRequested();
}

void VideoPlayer::setLowLatencyMode(bool enable)
{
    m_lowLatencyMode = enable;
    if (enable) {
        configureLowLatency();
    }
}

void VideoPlayer::setMaxDelay(int milliseconds)
{
    m_maxDelay = milliseconds;
}

void VideoPlayer::updateLiveSync()
{
    if (m_player->state() != QMediaPlayer::PlayingState) {
        return;
    }

    qint64 duration = m_player->duration();
    qint64 position = m_player->position();

    if (duration > 0 && position > 0) {
        qint64 delay = duration - position;

        // 如果延迟过大，强制跳转
        if (delay > m_maxDelay) {
            qDebug() << "检测到高延迟:" << delay << "ms，强制同步到最新位置";
            seekToLivePosition();
        }
    }
}

void VideoPlayer::seekToLivePosition()
{
    qint64 duration = m_player->duration();
    if (duration > m_delay) {
        qint64 targetPosition = duration - m_delay;

        m_player->setPosition(targetPosition);
    }
}

void VideoPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::InvalidMedia:
        qDebug() << "无法连接到流媒体服务器或流格式不支持";
        break;
    case QMediaPlayer::BufferedMedia:
        qDebug() << "开始播放流媒体";
        break;
    case QMediaPlayer::LoadingMedia:
        qDebug() << "正在加载流媒体...";
        break;
    case QMediaPlayer::BufferingMedia:
        qDebug() << "正在缓冲流媒体...";
        break;
    default:
        break;
    }
}

void VideoPlayer::onPlayerError(QMediaPlayer::Error error)
{
    QString errorStr = m_player->errorString();
    qDebug() << "播放器错误 (" << error << "):" << errorStr;
}

VideoPlayer::~VideoPlayer()
{
    m_syncTimer->stop();
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
