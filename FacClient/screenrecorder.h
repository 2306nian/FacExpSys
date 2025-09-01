#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include <QObject>
#include <QTimer>
#include <QAudioInput>
#include <QAudioFormat>
#include <QBuffer>
#include <QMutex>
#include <QScreen>
#include <QApplication>
#include <QGuiApplication>
#include <QMutexLocker>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

class ScreenRecorder : public QObject
{
    Q_OBJECT

public:
    explicit ScreenRecorder(QObject *parent = nullptr);
    ~ScreenRecorder();

    // 核心接口 - 只需要这两个
    bool startRecording(const QString& filePath);
    void stopRecording();
    bool isRecording() const { return m_isRecording; }

signals:
    void recordingStarted();
    void recordingStopped();
    void recordingError(const QString& error);

private slots:
    void captureScreenFrame();
    void processAudioData();
    void onAudioDataReady();

private:
    void cleanupFFmpeg();
    bool setupVideoStream();
    bool setupAudioStream();
    void writeVideoFrame(const QImage& frame);
    void writeAudioFrame();
    void flushEncoders();

    // 状态
    bool m_isRecording; // 是否正在录制
    qint64 m_videoFrameCount; // 视频帧计数器
    qint64 m_audioFrameCount; // 音频帧计数器

    // 预设参数（已固定）
    static const int FRAME_RATE = 30;        // 30fps
    static const int VIDEO_BITRATE = 3000000; // 3Mbps
    static const int AUDIO_SAMPLE_RATE = 44100; // 44.1kHz
    static const int AUDIO_CHANNELS = 2;     // 立体声

    // FFmpeg相关
    AVFormatContext *m_formatContext; // 容器格式上下文（mp4文件）
    AVStream *m_videoStream; // 视频流
    AVStream *m_audioStream; // 音频流
    AVCodecContext *m_videoCodecContext; // 视频编码器上下文
    AVCodecContext *m_audioCodecContext; // 音频编码器上下文

    // 视频编码相关
    AVFrame *m_videoFrame; // YUV格式的视频帧（编码用）
    AVFrame *m_rgbFrame; // RGB格式的视频帧（转换用）
    struct SwsContext *m_swsContext;  // 视频格式转换上下文

    // 音频相关
    QAudioInput *m_audioInput;   // Qt音频输入设备
    QBuffer *m_audioBuffer; // 音频数据缓冲区
    QTimer *m_captureTimer; // 屏幕捕获定时器（30fps）
    QTimer *m_audioTimer; // 音频处理定时器（每20ms）
    QMutex m_audioMutex; // 音频数据访问互斥锁
    QByteArray m_audioDataBuffer; // 音频数据临时存储

    // 音频编码相关
    SwrContext *m_swrContext; // 音频重采样上下文
    AVFrame *m_audioFrame; // 音频帧
    AVAudioFifo *m_audioFifo; // 音频FIFO缓冲区
    uint8_t **m_convertedAudioData; // 重采样后的音频数据
    int m_maxAudioSamples; // 最大音频样本数
};

#endif // SCREENRECORDER_H
