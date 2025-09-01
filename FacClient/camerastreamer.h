#ifndef CAMERASTREAMER_H
#define CAMERASTREAMER_H

#include <QProcess>
#include <QStandardPaths>
#include <QObject>
#include <QCamera>
#include <QMediaRecorder>
#include <QUrl>
#include <QCameraInfo>
#include <QCameraViewfinder>

class Session;

class CameraStreamer : public QObject
{
    Q_OBJECT

public:
    explicit CameraStreamer(Session* session, bool viewfinder = false, QObject *parent = nullptr);
    ~CameraStreamer();

    bool startStreaming(const QString &rtmpUrl);
    void stopStreaming();
    bool isStreaming() const;

signals:
    void streamingStarted();
    void streamingStopped();
    void errorOccurred(const QString &error);

private:
    void setupPreviewReceiver();
    void startFFmpegStreaming(const QString &rtmpUrl);

    Session* m_session;
    QString m_rtmpUrl;
    bool m_isStreaming;
    bool m_viewfinder;
    QProcess *m_ffmpegProcess;
    QProcess *m_previewReceiver;

};

#endif // CAMERASTREAMER_H
