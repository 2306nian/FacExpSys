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
#include <QDesktopWidget>
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
    bool m_isRecording;
    qint64 m_videoFrameCount;
    qint64 m_audioFrameCount;

    // 预设参数（已固定）
    static const int FRAME_RATE = 30;        // 30fps
    static const int VIDEO_BITRATE = 3000000; // 3Mbps
    static const int AUDIO_SAMPLE_RATE = 44100; // 44.1kHz
    static const int AUDIO_CHANNELS = 2;     // 立体声

    // FFmpeg相关
    AVFormatContext *m_formatContext;
    AVStream *m_videoStream;
    AVStream *m_audioStream;
    AVCodecContext *m_videoCodecContext;
    AVCodecContext *m_audioCodecContext;

    // 视频编码相关
    AVFrame *m_videoFrame;
    AVFrame *m_rgbFrame;
    struct SwsContext *m_swsContext;

    // 音频相关
    QAudioInput *m_audioInput;
    QBuffer *m_audioBuffer;
    QTimer *m_captureTimer;
    QTimer *m_audioTimer;
    QMutex m_audioMutex;
    QByteArray m_audioDataBuffer;

    // 音频编码相关
    SwrContext *m_swrContext;
    AVFrame *m_audioFrame;
    AVAudioFifo *m_audioFifo;
    uint8_t **m_convertedAudioData;
    int m_maxAudioSamples;
};

#endif // SCREENRECORDER_H
