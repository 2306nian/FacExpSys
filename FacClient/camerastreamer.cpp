#include "camerastreamer.h"
#include "session.h"
#include <QDebug>
#include <QCameraInfo>
#include <QVideoEncoderSettings>
#include <QAudioEncoderSettings>
#include <QThread>
#include <QFile>
#include <QDateTime>

CameraStreamer::CameraStreamer(Session* session, bool viewfinder, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_isStreaming(false)
    , m_viewfinder(viewfinder)
    , m_ffmpegProcess(new QProcess(this))
    , m_ffmpegScreenProcess(new QProcess(this))
{
    // 连接 FFmpeg 进程信号
    connect(m_ffmpegProcess, &QProcess::started, [this]() {
        m_isStreaming = true;
        emit streamingStarted();
        qDebug() << "FFmpeg streaming started";
    });

    connect(m_ffmpegScreenProcess, &QProcess::started, [this]() {
        m_isScreenStreaming = true;
        emit streamingStarted();
        qDebug() << "FFmpeg Screen streaming started";
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
    connect(m_ffmpegScreenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_isStreaming = false;
                emit streamingStopped();

                if (exitCode != 0) {
                    // 获取标准错误输出
                    QByteArray errorOutput = m_ffmpegScreenProcess->readAllStandardError();
                    QString errorStr = QString::fromLocal8Bit(errorOutput);

                    // 获取标准输出
                    QByteArray standardOutput = m_ffmpegScreenProcess->readAllStandardOutput();
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
    connect(m_ffmpegScreenProcess, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
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
}

void CameraStreamer::startFFmpegStreaming(const QString &rtmpUrl)
{
    QStringList arguments;

#ifdef Q_OS_LINUX
    // Linux 平台 - 兼容 FFmpeg 4.2.7 的参数
    arguments << "-f" << "v4l2"
              << "-thread_queue_size" << "512"
              << "-i" << "/dev/video0"
              << "-f" << "alsa"
              << "-thread_queue_size" << "512"
              << "-i" << "default"

              // 视频编码参数（直播优化）
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
              << "-flvflags" << "no_duration_filesize"
              << "-f" << "flv"
              << rtmpUrl

              // 同时创建预览窗口（使用不同的输出格式）
              << "-vf" << "scale=640:480"
              << "-vcodec" << "rawvideo"
              << "-pix_fmt" << "bgr24"
              << "-f" << "sdl"
              << QString("Camera Preview");

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
    if (m_viewfinder){
        setupPreviewReceiver();
    }

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

void CameraStreamer::startScreenStreaming(const QString &rtmpUrl)
{
    QString display = getenv("DISPLAY");
    QStringList arguments;
    arguments << "-f" << "x11grab"                    // X11 屏幕捕获
              << "-framerate" << "30"                 // 帧率
              << "-video_size" << "1920x1080"         // 屏幕分辨率
              << "-i" << display                      // 显示设备（默认显示器）
              << "-f" << "pulse"                      // 音频输入（PulseAudio）
              << "-i" << "default"                    // 默认音频设备

              // 视频编码参数
              << "-vcodec" << "libx264"               // H.264 编码
              << "-preset" << "ultrafast"             // 超快速编码
              << "-tune" << "zerolatency"             // 零延迟优化
              << "-profile:v" << "baseline"           // 基础配置文件
              << "-level" << "3.0"
              << "-pix_fmt" << "yuv420p"
              << "-s" << "1280x720"                   // 输出分辨率
              << "-g" << "30"                         // GOP 大小
              << "-b:v" << "2000k"                    // 视频码率
              << "-maxrate" << "2000k"
              << "-bufsize" << "2000k"

              // 音频编码参数
              << "-acodec" << "aac"
              << "-ar" << "44100"
              << "-ac" << "2"
              << "-b:a" << "128k"

              // 输出参数
              << "-flvflags" << "no_duration_filesize"
              << "-f" << "flv"
              << rtmpUrl;
    m_ffmpegScreenProcess->start("ffmpeg", arguments);
    emit m_session->rtmpStreamStartSend(m_session, rtmpUrl);
}

void CameraStreamer::stopCameraStreaming(){
    emit m_session->rtmpStreamStopSend(m_session, m_screenRtmpUrl);
    // 确保 FFmpeg 进程被终止
    if (m_ffmpegScreenProcess) {
        if (m_ffmpegScreenProcess->state() == QProcess::Running) {
            qDebug() << "Terminating FFmpeg process in destructor...";

            // 先尝试正常终止
            m_ffmpegScreenProcess->terminate();

            // 等待进程退出（最多5秒）
            if (!m_ffmpegScreenProcess->waitForFinished(5000)) {
                qDebug() << "Force killing FFmpeg process...";
                m_ffmpegScreenProcess->kill();

                // 再等待强制终止完成
                if (!m_ffmpegScreenProcess->waitForFinished(2000)) {
                    qDebug() << "Warning: FFmpeg process may not have terminated properly";
                }
            }
        }
    }
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

void CameraStreamer::setupPreviewReceiver()
{
    // 创建预览显示窗口
    m_previewReceiver = new QProcess(this);

    // 使用 ffplay 从管道接收数据
    QStringList args;
    args << "-f" << "rawvideo"
         << "-pixel_format" << "rgb24"
         << "-video_size" << "640x480"
         << "-i" << "pipe:0"  // 从标准输入读取
         << "-window_title" << QString("Preview - %1").arg(m_rtmpUrl)
         << "-autosync" << "1";

    m_previewReceiver->start("ffplay", args);

    // 连接 FFmpeg 的输出到预览接收器的输入
    connect(m_ffmpegProcess, &QProcess::readyReadStandardOutput,
            this, [this]() {
                QByteArray data = m_ffmpegProcess->readAllStandardOutput();

                if (m_previewReceiver && m_previewReceiver->state() == QProcess::Running) {
                    m_previewReceiver->write(data);
                }
            });
}


