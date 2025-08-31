#include "camerastreamer.h"
#include "session.h"
#include "handlers.h"
#include <QDebug>
#include <QCameraInfo>
#include <QVideoEncoderSettings>
#include <QAudioEncoderSettings>
#include <QThread>
#include <QFile>
#include <QDateTime>

CameraStreamer::CameraStreamer(Session* session, QObject *parent, QString name)
    : QObject(parent)
    , m_session(session)
    , m_name(name)
    , m_camera(nullptr)
    , m_recorder(nullptr)
    , m_isStreaming(false)
    , m_viewfinder(nullptr)
    , m_ffmpegProcess(new QProcess(this))
{
    initializeCamera();

    // 连接 FFmpeg 进程信号
    connect(m_ffmpegProcess, &QProcess::started, [this]() {
        m_isStreaming = true;
        emit streamingStarted();
        qDebug() << "FFmpeg streaming started";
    });

    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_isStreaming = false;
                emit streamingStopped();

                if (exitCode != 0) {
                    // 获取标准错误输出
                    QByteArray errorOutput = m_ffmpegProcess->readAllStandardError();
                    QString errorStr = QString::fromLocal8Bit(errorOutput);

                    // 获取标准输出
                    QByteArray standardOutput = m_ffmpegProcess->readAllStandardOutput();
                    QString outputStr = QString::fromLocal8Bit(standardOutput);

                    qDebug() << "=== FFmpeg Error Details ===";
                    qDebug() << "Exit code:" << exitCode;
                    qDebug() << "Exit status:" << (exitStatus == QProcess::NormalExit ? "Normal" : "Crash");
                    qDebug() << "Standard Error:" << errorStr;
                    qDebug() << "Standard Output:" << outputStr;
                    qDebug() << "==========================";

                    emit errorOccurred(QString("FFmpeg failed (code %1): %2").arg(exitCode).arg(errorStr));
                } else {
                    qDebug() << "FFmpeg streaming finished successfully";
                }
            });
    connect(m_ffmpegProcess, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        QString errorStr;
        switch (error) {
        case QProcess::FailedToStart:
            errorStr = "FFmpeg failed to start";
            break;
        case QProcess::Crashed:
            errorStr = "FFmpeg crashed";
            break;
        case QProcess::Timedout:
            errorStr = "FFmpeg timeout";
            break;
        case QProcess::WriteError:
            errorStr = "FFmpeg write error";
            break;
        case QProcess::ReadError:
            errorStr = "FFmpeg read error";
            break;
        case QProcess::UnknownError:
            errorStr = "FFmpeg unknown error";
            break;
        }
        emit errorOccurred(errorStr);
    });
}

CameraStreamer::~CameraStreamer()
{
    stopStreaming();
    if (m_camera) {
        delete m_camera;
    }
    if (m_recorder) {
        delete m_recorder;
    }
}

void CameraStreamer::initializeCamera()
{
    // 获取可用的摄像头列表
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        emit errorOccurred("No camera available");
        return;
    }

    // 使用默认摄像头
    m_camera = new QCamera(cameras.first(), this);

    // 创建媒体录制器
    m_recorder = new QMediaRecorder(m_camera, this);

    // 连接信号
    connect(m_recorder, &QMediaRecorder::stateChanged,
            this, &CameraStreamer::onRecorderStateChanged);
    connect(m_recorder, &QMediaRecorder::statusChanged,
            this, &CameraStreamer::onRecorderStatusChanged);
    connect(m_camera, &QCamera::statusChanged,
            this, &CameraStreamer::onCameraStatusChanged);
    connect(m_camera, static_cast<void(QCamera::*)(QCamera::Error)>(&QCamera::error),
            this, &CameraStreamer::onCameraError);

    connect(m_session, &Session::rtmpStreamStartSend, RTMPHandler::instance(), &RTMPHandler::handleRTMPStartSend);
    connect(m_session, &Session::rtmpStreamStopSend, RTMPHandler::instance(), &RTMPHandler::handleRTMPStopSend);

    // 设置取景器（如果提供了）
    if (m_viewfinder) {
        m_camera->setViewfinder(m_viewfinder);
    }
}

void CameraStreamer::startFFmpegStreaming(const QString &rtmpUrl)
{
    QStringList arguments;

#ifdef Q_OS_LINUX
    // Linux 平台 - 兼容 FFmpeg 4.2.7 的参数
    arguments << "-f" << "v4l2"                          // 视频输入格式
              << "-thread_queue_size" << "512"           // 增加队列大小防止阻塞
              << "-i" << "/dev/video0"                   // 摄像头设备
              << "-f" << "alsa"                          // 音频输入格式
              << "-thread_queue_size" << "512"
              << "-i" << "default"                       // 默认音频设备
              // 视频编码参数（直播优化）
              << "-vcodec" << "libx264"                  // H.264 编码
              << "-preset" << "ultrafast"                // 超快速编码
              << "-tune" << "zerolatency"                // 零延迟优化
              << "-profile:v" << "baseline"              // 基础配置文件（兼容性好）
              << "-level" << "3.0"                       // 编码级别
              << "-pix_fmt" << "yuv420p"                 // 像素格式
              << "-s" << "1280x720"                      // 分辨率
              << "-r" << "30"                            // 帧率
              << "-g" << "30"                            // GOP大小（关键帧间隔）
              << "-keyint_min" << "30"                   // 最小关键帧间隔
              << "-sc_threshold" << "0"                  // 禁用场景切换关键帧
              << "-b:v" << "2000k"                       // 视频码率
              << "-maxrate" << "2000k"                   // 最大码率
              << "-bufsize" << "2000k"                   // 缓冲区大小
              // 音频编码参数
              << "-acodec" << "aac"                      // AAC 音频编码
              << "-ar" << "44100"                        // 音频采样率
              << "-ac" << "2"                            // 音频通道数
              << "-b:a" << "128k"                        // 音频码率
              // 输出参数（移除了不支持的选项）
              << "-f" << "flv"                           // FLV 格式
              << "-flvflags" << "no_duration_filesize"   // 不写入持续时间和文件大小
              << rtmpUrl;                                // RTMP URL

#elif defined(Q_OS_WIN)
    // Windows 平台
    arguments << "-f" << "dshow"                         // DirectShow 输入
              << "-thread_queue_size" << "512"
              << "-i" << "video=\"USB Camera\":audio=\"麦克风\""  // 设备名称
              // 视频编码参数
              << "-vcodec" << "libx264"
              << "-preset" << "ultrafast"
              << "-tune" << "zerolatency"
              << "-profile:v" << "baseline"
              << "-level" << "3.0"
              << "-pix_fmt" << "yuv420p"
              << "-s" << "1280x720"
              << "-r" << "30"
              << "-g" << "30"
              << "-keyint_min" << "30"
              << "-sc_threshold" << "0"
              << "-b:v" << "2000k"
              << "-maxrate" << "2000k"
              << "-bufsize" << "2000k"
              // 音频编码参数
              << "-acodec" << "aac"
              << "-ar" << "44100"
              << "-ac" << "2"
              << "-b:a" << "128k"
              // 输出参数
              << "-f" << "flv"
              << "-flvflags" << "no_duration_filesize"
              << rtmpUrl;

#elif defined(Q_OS_MAC)
    // macOS 平台
    arguments << "-f" << "avfoundation"                  // AVFoundation 输入
              << "-thread_queue_size" << "512"
              << "-i" << "0:0"                           // 设备索引
              // 视频编码参数
              << "-vcodec" << "libx264"
              << "-preset" << "ultrafast"
              << "-tune" << "zerolatency"
              << "-profile:v" << "baseline"
              << "-level" << "3.0"
              << "-pix_fmt" << "yuv420p"
              << "-s" << "1280x720"
              << "-r" << "30"
              << "-g" << "30"
              << "-keyint_min" << "30"
              << "-sc_threshold" << "0"
              << "-b:v" << "2000k"
              << "-maxrate" << "2000k"
              << "-bufsize" << "2000k"
              // 音频编码参数
              << "-acodec" << "aac"
              << "-ar" << "44100"
              << "-ac" << "2"
              << "-b:a" << "128k"
              // 输出参数
              << "-f" << "flv"
              << "-flvflags" << "no_duration_filesize"
              << rtmpUrl;
#endif

    // 添加全局选项
    arguments.prepend("-fflags");
    arguments.insert(1, "+genpts");  // 生成正确的PTS

    qDebug() << "=== Starting FFmpeg with arguments ===";
    for (int i = 0; i < arguments.size(); ++i) {
        qDebug() << "Arg" << i << ":" << arguments.at(i);
    }
    qDebug() << "=====================================";

    m_ffmpegProcess->start("ffmpeg", arguments);

    if (!m_ffmpegProcess->waitForStarted(5000)) {
        emit errorOccurred("FFmpeg failed to start within 5 seconds");
        return;
    }

    qDebug() << "FFmpeg process started with PID:" << m_ffmpegProcess->processId();
}

bool CameraStreamer::startStreaming(const QString &rtmpUrl)
{
    if (m_isStreaming) {
        emit errorOccurred("Already streaming");
        return false;
    }

    if (!m_session) {
        emit errorOccurred("Session not initialized");
        return false;
    }

    m_rtmpUrl = rtmpUrl;

    qDebug() << "=== Starting Live Stream ===";
    qDebug() << "RTMP URL:" << rtmpUrl;
    qDebug() << "Platform:" <<
#ifdef Q_OS_LINUX
        "Linux";
#elif defined(Q_OS_WIN)
        "Windows";
#elif defined(Q_OS_MAC)
        "macOS";
#else
        "Unknown";
#endif

    // 检查 FFmpeg 是否可用
    QProcess checkProcess;
    checkProcess.start("ffmpeg", QStringList() << "-version");
    if (!checkProcess.waitForFinished(3000) || checkProcess.exitCode() != 0) {
        emit errorOccurred("FFmpeg not found. Please install FFmpeg and add it to PATH.");
        return false;
    }

    // 检查摄像头设备是否存在（Linux）
#ifdef Q_OS_LINUX
    QFile videoDevice("/dev/video0");
    if (!videoDevice.exists()) {
        emit errorOccurred("Camera device /dev/video0 not found");
        return false;
    }
#endif

    // 启动 FFmpeg 推流
    startFFmpegStreaming(rtmpUrl);
    emit m_session->rtmpStreamStartSend(m_session, rtmpUrl);

    return true;
}

void CameraStreamer::stopStreaming()
{
    emit m_session->rtmpStreamStopSend(m_session, m_rtmpUrl);
    // 确保 FFmpeg 进程被终止
    if (m_ffmpegProcess) {
        if (m_ffmpegProcess->state() == QProcess::Running) {
            qDebug() << "Terminating FFmpeg process in destructor...";

            // 先尝试正常终止
            m_ffmpegProcess->terminate();

            // 等待进程退出（最多5秒）
            if (!m_ffmpegProcess->waitForFinished(5000)) {
                qDebug() << "Force killing FFmpeg process...";
                m_ffmpegProcess->kill();

                // 再等待强制终止完成
                if (!m_ffmpegProcess->waitForFinished(2000)) {
                    qDebug() << "Warning: FFmpeg process may not have terminated properly";
                }
            }
        }
    }
}

bool CameraStreamer::isStreaming() const
{
    return m_isStreaming;
}

void CameraStreamer::setViewfinder(QCameraViewfinder* viewfinder)
{
    m_viewfinder = viewfinder;
    if (m_camera && m_viewfinder) {
        m_camera->setViewfinder(m_viewfinder);
    }
}

void CameraStreamer::onRecorderStateChanged(QMediaRecorder::State state)
{
    QString stateStr;
    switch (state) {
    case QMediaRecorder::StoppedState:
        stateStr = "Stopped";
        break;
    case QMediaRecorder::RecordingState:
        stateStr = "Recording";
        break;
    case QMediaRecorder::PausedState:
        stateStr = "Paused";
        break;
    }
    qDebug() << "Recorder state changed:" << stateStr;
}

void CameraStreamer::onRecorderStatusChanged(QMediaRecorder::Status status)
{
    QString statusStr;
    switch (status) {
    case QMediaRecorder::UnavailableStatus:
        statusStr = "Unavailable";
        break;
    case QMediaRecorder::UnloadedStatus:
        statusStr = "Unloaded";
        break;
    case QMediaRecorder::LoadingStatus:
        statusStr = "Loading";
        break;
    case QMediaRecorder::LoadedStatus:
        statusStr = "Loaded";
        break;
    case QMediaRecorder::StartingStatus:
        statusStr = "Starting";
        break;
    case QMediaRecorder::RecordingStatus:
        statusStr = "Recording";
        break;
    case QMediaRecorder::PausedStatus:
        statusStr = "Paused";
        break;
    case QMediaRecorder::FinalizingStatus:
        statusStr = "Finalizing";
        break;
    }
    qDebug() << "Recorder status changed:" << statusStr;
}

void CameraStreamer::onCameraStatusChanged(QCamera::Status status)
{
    QString statusStr;
    switch (status) {
    case QCamera::UnloadedStatus:
        statusStr = "Unloaded";
        break;
    case QCamera::LoadingStatus:
        statusStr = "Loading";
        break;
    case QCamera::LoadedStatus:
        statusStr = "Loaded";
        break;
    case QCamera::ActiveStatus:
        statusStr = "Active";
        break;
    }

    emit cameraStatusChanged(statusStr);
    qDebug() << "Camera status changed:" << statusStr;
}

void CameraStreamer::onCameraError(QCamera::Error error)
{
    QString errorStr;
    switch (error) {
    case QCamera::NoError:
        errorStr = "No error";
        break;
    case QCamera::CameraError:
        errorStr = "Camera error";
        break;
    case QCamera::InvalidRequestError:
        errorStr = "Invalid request";
        break;
    case QCamera::ServiceMissingError:
        errorStr = "Service missing";
        break;
    case QCamera::NotSupportedFeatureError:
        errorStr = "Not supported";
        break;
    }

    emit errorOccurred(errorStr);
    qDebug() << "Camera error:" << errorStr;
}
