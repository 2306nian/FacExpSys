#include "screenrecorder.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QPixmap>
#include <QScreen>

// 在session中连接
// connect(m_recorder, &ScreenRecorder::recordingStarted, this, ...);
// connect(m_recorder, &ScreenRecorder::recordingStopped, this, ...);
// connect(m_recorder, &ScreenRecorder::recordingError, this, ...);

ScreenRecorder::ScreenRecorder(QObject *parent)
    : QObject(parent)
    , m_isRecording(false)
    , m_videoFrameCount(0)
    , m_audioFrameCount(0)
    , m_formatContext(nullptr)
    , m_videoStream(nullptr)
    , m_audioStream(nullptr)
    , m_videoCodecContext(nullptr)
    , m_audioCodecContext(nullptr)
    , m_videoFrame(nullptr)
    , m_rgbFrame(nullptr)
    , m_swsContext(nullptr)
    , m_audioInput(nullptr)
    , m_audioBuffer(nullptr)
    , m_captureTimer(nullptr)
    , m_audioTimer(nullptr)
    , m_swrContext(nullptr)
    , m_audioFrame(nullptr)
    , m_audioFifo(nullptr)
    , m_convertedAudioData(nullptr)
    , m_maxAudioSamples(0)
{
    // 初始化音频输入（使用预设参数）
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(AUDIO_SAMPLE_RATE);
    audioFormat.setChannelCount(AUDIO_CHANNELS);
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo inputDevice = QAudioDeviceInfo::defaultInputDevice();
    // 检查默认音频输入设备是否支持该格式
    if (inputDevice.isFormatSupported(audioFormat)) {
        // 创建音频输入和缓冲区
        m_audioInput = new QAudioInput(inputDevice, audioFormat, this);
        m_audioBuffer = new QBuffer(this);
        m_audioBuffer->open(QIODevice::WriteOnly);

        // 音频数据就绪信号
        connect(m_audioInput, &QAudioInput::notify, this, &ScreenRecorder::onAudioDataReady);
    }

    // 初始化定时器
    m_captureTimer = new QTimer(this);
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenRecorder::captureScreenFrame);

    m_audioTimer = new QTimer(this);
    connect(m_audioTimer, &QTimer::timeout, this, &ScreenRecorder::processAudioData);
}

ScreenRecorder::~ScreenRecorder()
{
    if (m_isRecording) {
        stopRecording();
    }
    cleanupFFmpeg();

    if (m_audioBuffer) {
        m_audioBuffer->close();
    }
}

bool ScreenRecorder::startRecording(const QString& filePath)
{
    if (m_isRecording) {
        qDebug()<<"录制已在经行中";
        emit recordingError("录制已在进行中");
        return false;
    }

    // 创建目录
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    // 分配FFmpeg格式上下文
    avformat_alloc_output_context2(&m_formatContext, nullptr, nullptr, filePath.toStdString().c_str());
    if (!m_formatContext) {
        qDebug()<<"创建输出上下文失败";
        emit recordingError("创建输出上下文失败");
        return false;
    }

    // 设置视频和音频流
    if (!setupVideoStream() || !setupAudioStream()) {
        cleanupFFmpeg();
        return false;
    }

    // 打开输出文件
    if (avio_open(&m_formatContext->pb, filePath.toStdString().c_str(), AVIO_FLAG_WRITE) < 0) {
        cleanupFFmpeg();
        qDebug()<<"打开输出文件失败";
        emit recordingError("打开输出文件失败: " + filePath);
        return false;
    }

    // 写入文件头
    if (avformat_write_header(m_formatContext, nullptr) < 0) {
        cleanupFFmpeg();
        qDebug()<<"写入文件失败";
        emit recordingError("写入文件头失败");
        return false;
    }

    // 开始录制
    m_isRecording = true;
    m_videoFrameCount = 0;
    m_audioFrameCount = 0;

    // 启动音频输入
    if (m_audioInput && m_audioBuffer) {
        m_audioInput->start(m_audioBuffer);
        m_audioDataBuffer.clear();
    }

    // 启动屏幕捕获定时器（固定30fps）
    m_captureTimer->start(1000 / FRAME_RATE);  // 33ms间隔
    m_audioTimer->start(20);  // 每20ms处理一次音频

    qDebug()<<"recordingStarted";
    emit recordingStarted();
    return true;
}

void ScreenRecorder::stopRecording()
{
    if (!m_isRecording) return;

    m_isRecording = false;

    // 停止定时器
    m_captureTimer->stop();
    m_audioTimer->stop();

    // 停止音频输入
    if (m_audioInput) {
        m_audioInput->stop();
    }

    // 处理剩余的音频数据
    processAudioData();
    flushEncoders();

    // 写入文件尾
    if (m_formatContext) {
        av_write_trailer(m_formatContext);
    }

    cleanupFFmpeg();
    qDebug()<<"录制暂停";
    emit recordingStopped();
}

void ScreenRecorder::cleanupFFmpeg()
{
    // 清理视频相关
    if (m_videoFrame) {
        av_frame_free(&m_videoFrame);
    }
    if (m_rgbFrame) {
        av_frame_free(&m_rgbFrame);
    }
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
    }
    if (m_videoCodecContext) {
        avcodec_free_context(&m_videoCodecContext);
    }

    // 清理音频相关
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
    }
    if (m_swrContext) {
        swr_free(&m_swrContext);
    }
    if (m_audioFifo) {
        av_audio_fifo_free(m_audioFifo);
    }
    if (m_convertedAudioData) {
        if (m_convertedAudioData[0]) {
            av_freep(&m_convertedAudioData[0]);
        }
        av_freep(&m_convertedAudioData);
    }
    if (m_audioCodecContext) {
        avcodec_free_context(&m_audioCodecContext);
    }

    // 清理格式上下文
    if (m_formatContext) {
        if (m_formatContext->pb) {
            avio_closep(&m_formatContext->pb);
        }
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
}

bool ScreenRecorder::setupVideoStream()
{
    // 查找H.264编码器
    AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!videoCodec) {
        qDebug()<<"未找到H.264编码器";
        emit recordingError("未找到H.264编码器");
        return false;
    }

    // 创建视频流
    m_videoStream = avformat_new_stream(m_formatContext, videoCodec);
    if (!m_videoStream) {
        qDebug()<<"创建视频流失败";
        emit recordingError("创建视频流失败");
        return false;
    }

    // 分配编码器上下文
    m_videoCodecContext = avcodec_alloc_context3(videoCodec);
    if (!m_videoCodecContext) {
        qDebug()<<"分配视频编码器上下文失败";
        emit recordingError("分配视频编码器上下文失败");
        return false;
    }

    // 获取屏幕分辨率
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    int width = screenRect.width();
    int height = screenRect.height();

    // 设置编码参数（使用预设值）
    m_videoCodecContext->codec_id = AV_CODEC_ID_H264;
    m_videoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    m_videoCodecContext->width = width;
    m_videoCodecContext->height = height;
    m_videoCodecContext->time_base = {1, FRAME_RATE}; // 时间基
    m_videoCodecContext->framerate = {FRAME_RATE, 1}; // 帧率
    m_videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P; // YUV420P格式
    m_videoCodecContext->bit_rate = VIDEO_BITRATE; // 比特率
    m_videoCodecContext->gop_size = 10; // GOP大小
    m_videoCodecContext->max_b_frames = 1; // B帧数量
    // 设置编码器参数（x264）
    av_opt_set(m_videoCodecContext->priv_data, "preset", "medium", 0); // 编码速度质量平衡
    av_opt_set(m_videoCodecContext->priv_data, "crf", "23", 0); // 恒定质量因子

    // 打开编码器
    if (avcodec_open2(m_videoCodecContext, videoCodec, nullptr) < 0) {
        qDebug()<<"打开视频编码器失败";
        emit recordingError("打开视频编码器失败");
        return false;
    }

    // 复制参数到流
    if (avcodec_parameters_from_context(m_videoStream->codecpar, m_videoCodecContext) < 0) {
        qDebug()<<"复制视频参数失败";
        emit recordingError("复制视频参数失败");
        return false;
    }

    // 分配帧
    m_videoFrame = av_frame_alloc(); // YUV帧
    m_rgbFrame = av_frame_alloc(); // RGB帧
    if (!m_videoFrame || !m_rgbFrame) {
        qDebug()<<"分配视频帧失败";
        emit recordingError("分配视频帧失败");
        return false;
    }

    m_videoFrame->format = m_videoCodecContext->pix_fmt;
    m_videoFrame->width = m_videoCodecContext->width;
    m_videoFrame->height = m_videoCodecContext->height;

    m_rgbFrame->format = AV_PIX_FMT_RGB24;
    m_rgbFrame->width = m_videoCodecContext->width;
    m_rgbFrame->height = m_videoCodecContext->height;


    // 分配帧缓冲区
    if (av_frame_get_buffer(m_videoFrame, 32) < 0 ||
        av_frame_get_buffer(m_rgbFrame, 32) < 0) {
        qDebug()<<"分配视频帧缓冲失败";
        emit recordingError("分配视频帧缓冲失败");
        return false;
    }

    // 初始化转换上下文
    m_swsContext = sws_getContext(
        m_videoCodecContext->width, m_videoCodecContext->height, AV_PIX_FMT_RGB24,
        m_videoCodecContext->width, m_videoCodecContext->height, m_videoCodecContext->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_swsContext) {
        qDebug()<<"初始化视频转换上下文失败";
        emit recordingError("初始化视频转换上下文失败");
        return false;
    }

    // 设置流时间基
    m_videoStream->time_base = {1, 90000};
    return true;
}

bool ScreenRecorder::setupAudioStream()
{
    // 查找AAC编码器
    AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audioCodec) {
        qDebug()<<"未找到AAC编码器";
        emit recordingError("未找到AAC编码器");
        return false;
    }

    // 创建音频流
    m_audioStream = avformat_new_stream(m_formatContext, audioCodec);
    if (!m_audioStream) {
        qDebug()<<"创建音频流失败";
        emit recordingError("创建音频流失败");
        return false;
    }

    // 分配编码器上下文
    m_audioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!m_audioCodecContext) {
        qDebug()<<"分配音频编码器上下文失败";
        emit recordingError("分配音频编码器上下文失败");
        return false;
    }

    // 设置音频参数（使用预设值）
    m_audioCodecContext->codec_id = audioCodec->id;
    m_audioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
    m_audioCodecContext->sample_rate = AUDIO_SAMPLE_RATE; // 44100Hz
    m_audioCodecContext->channels = AUDIO_CHANNELS; // 2声道
    m_audioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO; // 立体声布局
    m_audioCodecContext->bit_rate = 128000;  // 128kbps
    m_audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP; // 浮点平面格式

    // 打开编码器
    if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) < 0) {
        qDebug()<<"打开音频编码器失败";
        emit recordingError("打开音频编码器失败");
        return false;
    }

    // 复制参数到流
    if (avcodec_parameters_from_context(m_audioStream->codecpar, m_audioCodecContext) < 0) {
        qDebug()<<"复制音频参数失败";
        emit recordingError("复制音频参数失败");
        return false;
    }

    // 分配音频帧
    m_audioFrame = av_frame_alloc();
    if (!m_audioFrame) {
        qDebug()<<"分配音频帧失败";
        emit recordingError("分配音频帧失败");
        return false;
    }

    m_audioFrame->format = m_audioCodecContext->sample_fmt;
    m_audioFrame->channels = m_audioCodecContext->channels;
    m_audioFrame->channel_layout = m_audioCodecContext->channel_layout;
    m_audioFrame->sample_rate = m_audioCodecContext->sample_rate;
    m_audioFrame->nb_samples = m_audioCodecContext->frame_size ?
                                   m_audioCodecContext->frame_size : 1024;

    if (av_frame_get_buffer(m_audioFrame, 0) < 0) {
        qDebug()<<"分配音频帧缓冲失败";
        emit recordingError("分配音频帧缓冲失败");
        return false;
    }

    // 初始化音频FIFO
    m_audioFifo = av_audio_fifo_alloc(m_audioCodecContext->sample_fmt,
                                      m_audioCodecContext->channels,
                                      30 * m_audioCodecContext->sample_rate);
    if (!m_audioFifo) {
        qDebug()<<"分配音频FIFO失败";
        emit recordingError("分配音频FIFO失败");
        return false;
    }

    // 初始化重采样上下文
    m_swrContext = swr_alloc();
    if (!m_swrContext) {
        qDebug()<<"分配音频重采样上下文失败";
        emit recordingError("分配音频重采样上下文失败");
        return false;
    }

    av_opt_set_int(m_swrContext, "in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(m_swrContext, "out_channel_layout", m_audioCodecContext->channel_layout, 0);
    av_opt_set_int(m_swrContext, "in_sample_rate", AUDIO_SAMPLE_RATE, 0);
    av_opt_set_int(m_swrContext, "out_sample_rate", m_audioCodecContext->sample_rate, 0);
    av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", m_audioCodecContext->sample_fmt, 0);

    if (swr_init(m_swrContext) < 0) {
        qDebug()<<"初始化音频重采样失败";
        emit recordingError("初始化音频重采样失败");
        return false;
    }

    // 分配转换后的音频数据缓冲区
    m_maxAudioSamples = 4096;
    m_convertedAudioData = (uint8_t**)av_mallocz_array(m_audioCodecContext->channels, sizeof(*m_convertedAudioData));
    if (!m_convertedAudioData) {
        qDebug()<<"分配音频数据缓冲失败";
        emit recordingError("分配音频数据缓冲失败");
        return false;
    }

    if (av_samples_alloc(m_convertedAudioData, nullptr, m_audioCodecContext->channels,
                         m_maxAudioSamples, m_audioCodecContext->sample_fmt, 0) < 0) {
        qDebug()<<"分配音频样本缓冲失败";
        emit recordingError("分配音频样本缓冲失败");
        return false;
    }

    m_audioStream->time_base = {1, m_audioCodecContext->sample_rate};
    return true;
}

void ScreenRecorder::captureScreenFrame()
{
    if (!m_isRecording) return;

    // 捕获全屏
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap screenshot = screen->grabWindow(0);  // 捕获全屏
    QImage frame = screenshot.toImage();
    writeVideoFrame(frame);
}

void ScreenRecorder::writeVideoFrame(const QImage& frame)
{
    if (!m_isRecording || !m_videoCodecContext || !m_swsContext) return;

    // 转换图像格式
    QImage scaledFrame = frame.scaled(m_videoCodecContext->width, m_videoCodecContext->height,
                                      Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage rgbFrameImg = scaledFrame.convertToFormat(QImage::Format_RGB888);

    // 填充RGB帧
    av_frame_make_writable(m_rgbFrame);
    for (int y = 0; y < m_videoCodecContext->height; y++) {
        memcpy(m_rgbFrame->data[0] + y * m_rgbFrame->linesize[0],
               rgbFrameImg.scanLine(y),
               m_videoCodecContext->width * 3);
    }

    // 转换颜色空间
    sws_scale(m_swsContext,
              m_rgbFrame->data, m_rgbFrame->linesize,
              0, m_videoCodecContext->height,
              m_videoFrame->data, m_videoFrame->linesize);

    // 设置时间戳
    m_videoFrame->pts = av_rescale_q(m_videoFrameCount++,
                                     AVRational{1, FRAME_RATE},
                                     m_videoStream->time_base);

    // 编码并写入
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    int ret = avcodec_send_frame(m_videoCodecContext, m_videoFrame);
    if (ret < 0) return;

    while (ret >= 0) {
        ret = avcodec_receive_packet(m_videoCodecContext, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            break;
        }

        av_packet_rescale_ts(&pkt, m_videoCodecContext->time_base, m_videoStream->time_base);
        pkt.stream_index = m_videoStream->index;

        av_interleaved_write_frame(m_formatContext, &pkt);
        av_packet_unref(&pkt);
    }
}

void ScreenRecorder::onAudioDataReady()
{
    if (m_audioBuffer && m_audioInput) {
        QMutexLocker locker(&m_audioMutex);
        // 使用安全的方式读取数据
        qint64 bytesReady = m_audioBuffer->bytesAvailable();
        if (bytesReady > 0) {
            QByteArray data = m_audioBuffer->read(bytesReady);
            m_audioDataBuffer.append(data);
        }
    }
}

void ScreenRecorder::processAudioData()
{
    if (!m_isRecording) return;

    QMutexLocker locker(&m_audioMutex);
    if (m_audioDataBuffer.isEmpty()) return;

    // 计算输入样本数
    int bytesPerSample = AUDIO_CHANNELS * sizeof(int16_t);
    int inputSamples = m_audioDataBuffer.size() / bytesPerSample;

    if (inputSamples <= 0) {
        m_audioDataBuffer.clear();
        return;
    }

    // 计算重采样后输出样本数
    int outputSamples = swr_get_out_samples(m_swrContext, inputSamples);
    if (outputSamples <= 0) {
        m_audioDataBuffer.clear();
        return;
    }

    // 如果缓冲区不够，重新分配
    if (outputSamples > m_maxAudioSamples) {
        m_maxAudioSamples = outputSamples + 1024;
        if (m_convertedAudioData) {
            av_freep(&m_convertedAudioData[0]);
            av_freep(&m_convertedAudioData);
        }
        m_convertedAudioData = (uint8_t**)av_mallocz_array(m_audioCodecContext->channels, sizeof(*m_convertedAudioData));
        if (av_samples_alloc(m_convertedAudioData, nullptr, m_audioCodecContext->channels,
                             m_maxAudioSamples, m_audioCodecContext->sample_fmt, 0) < 0) {
            m_audioDataBuffer.clear();
            return;
        }
    }

    // 重采样音频数据
    const uint8_t *inputData[1] = { reinterpret_cast<const uint8_t*>(m_audioDataBuffer.constData()) };

    int convertedSamples = swr_convert(m_swrContext, m_convertedAudioData, outputSamples,
                                       inputData, inputSamples);

    // 写入FIFO缓冲区
    if (convertedSamples > 0) {
        if (av_audio_fifo_write(m_audioFifo, (void**)m_convertedAudioData, convertedSamples) >= 0) {
            while (av_audio_fifo_size(m_audioFifo) >= m_audioFrame->nb_samples) {
                writeAudioFrame();
            }
        }
    }

    m_audioDataBuffer.clear();
}

void ScreenRecorder::writeAudioFrame()
{
    if (!m_isRecording || !m_audioCodecContext || !m_audioFifo) return;

    // 从FIFO读取音频数据
    if (av_audio_fifo_read(m_audioFifo, (void**)m_audioFrame->data, m_audioFrame->nb_samples) <= 0) {
        return;
    }

    // 设置时间戳
    m_audioFrame->pts = av_rescale_q(m_audioFrameCount * m_audioFrame->nb_samples,
                                     AVRational{1, m_audioCodecContext->sample_rate},
                                     m_audioStream->time_base);
    m_audioFrameCount++;

    // 编码并写入
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    int ret = avcodec_send_frame(m_audioCodecContext, m_audioFrame);
    if (ret < 0) return;

    while (ret >= 0) {
        ret = avcodec_receive_packet(m_audioCodecContext, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            break;
        }

        av_packet_rescale_ts(&pkt, m_audioCodecContext->time_base, m_audioStream->time_base);
        pkt.stream_index = m_audioStream->index;

        av_interleaved_write_frame(m_formatContext, &pkt);
        av_packet_unref(&pkt);
    }
}

void ScreenRecorder::flushEncoders()
{
    // 刷新视频编码器
    if (m_videoCodecContext) {
        AVPacket pkt;
        av_init_packet(&pkt);

        int ret = avcodec_send_frame(m_videoCodecContext, nullptr);
        while (ret >= 0) {
            ret = avcodec_receive_packet(m_videoCodecContext, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                break;
            }

            av_packet_rescale_ts(&pkt, m_videoCodecContext->time_base, m_videoStream->time_base);
            pkt.stream_index = m_videoStream->index;
            av_interleaved_write_frame(m_formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // 刷新音频编码器
    if (m_audioCodecContext) {
        // 处理FIFO中剩余的数据
        while (m_audioFifo && av_audio_fifo_size(m_audioFifo) > 0) {
            int remainingSamples = av_audio_fifo_size(m_audioFifo);
            int samplesToRead = qMin(remainingSamples, m_audioFrame->nb_samples);

            if (av_audio_fifo_read(m_audioFifo, (void**)m_audioFrame->data, samplesToRead) > 0) {
                m_audioFrame->nb_samples = samplesToRead;
                // 修正：使用正确的时间戳计算
                m_audioFrame->pts = av_rescale_q(m_audioFrameCount * samplesToRead,
                                                 AVRational{1, m_audioCodecContext->sample_rate},
                                                 m_audioStream->time_base);
                m_audioFrameCount++;

                AVPacket pkt;
                av_init_packet(&pkt);

                int ret = avcodec_send_frame(m_audioCodecContext, m_audioFrame);
                while (ret >= 0) {
                    ret = avcodec_receive_packet(m_audioCodecContext, &pkt);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    } else if (ret < 0) {
                        break;
                    }

                    av_packet_rescale_ts(&pkt, m_audioCodecContext->time_base, m_audioStream->time_base);
                    pkt.stream_index = m_audioStream->index;
                    av_interleaved_write_frame(m_formatContext, &pkt);
                    av_packet_unref(&pkt);
                }
            }
        }

        // 发送NULL帧刷新编码器
        AVPacket pkt;
        av_init_packet(&pkt);

        int ret = avcodec_send_frame(m_audioCodecContext, nullptr);
        while (ret >= 0) {
            ret = avcodec_receive_packet(m_audioCodecContext, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                break;
            }

            av_packet_rescale_ts(&pkt, m_audioCodecContext->time_base, m_audioStream->time_base);
            pkt.stream_index = m_audioStream->index;
            av_interleaved_write_frame(m_formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }
}
