#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QCheckBox>

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer(int delay = 1000, QWidget *parent = nullptr);
    ~VideoPlayer();

    void playStream(const QString &rtmpUrl);
    void stopStream();
    void setLowLatencyMode(bool enable);
    void setMaxDelay(int milliseconds);

signals:
    void playRequested(const QString &url);
    void stopRequested();

private slots:
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayerError(QMediaPlayer::Error error);
    void updateLiveSync();
    void seekToLivePosition();

private:
    void configureLowLatency();

    int m_delay;
    int m_maxDelay;
    bool m_lowLatencyMode;
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    QPushButton *m_playButton;
    QPushButton *m_stopButton;
    QLineEdit *m_urlInput;
    QTimer *m_syncTimer;
    QCheckBox *m_lowLatencyCheckBox;
};

#endif // VIDEOPLAYER_H
