#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QProcess>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QImage>
#include <QMessageBox>
#include <QFileInfo>

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer(int delay = 1000, QWidget *parent = nullptr);
    ~VideoPlayer();

    void playStream(const QString &rtmpUrl);
    void stopStream();

private slots:
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onPlayerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readVideoData();
    void readPlayerError();
    void updateDisplay();

private:
    void startFFmpegPlayer(const QString &rtmpUrl);
    void setupUI();
    void processRawVideoData(const QByteArray& data);

    QProcess *m_ffmpegPlayer;
    QPushButton *m_playButton;
    QPushButton *m_stopButton;
    QLineEdit *m_urlInput;
    QLabel *m_statusLabel;
    QLabel *m_videoDisplay;  // 用于显示视频帧
    QTimer *m_displayTimer;

    QString m_currentUrl;
    QByteArray m_frameBuffer;
    int m_frameWidth;
    int m_frameHeight;
    int m_bytesPerFrame;
    bool m_frameReady;
};

#endif // VIDEOPLAYER_H
