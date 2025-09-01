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
    explicit CameraStreamer(Session* session, QObject *parent = nullptr, QString name = "test");
    ~CameraStreamer();

    bool startStreaming(const QString &rtmpUrl);
    void stopStreaming();
    bool isStreaming() const;
    void setViewfinder(QCameraViewfinder* viewfinder);

signals:
    void streamingStarted();
    void streamingStopped();
    void errorOccurred(const QString &error);
    void cameraStatusChanged(const QString &status);

private slots:
    void onRecorderStateChanged(QMediaRecorder::State state);
    void onRecorderStatusChanged(QMediaRecorder::Status status);
    void onCameraStatusChanged(QCamera::Status status);
    void onCameraError(QCamera::Error error);

private:
    void initializeCamera();
    void setupRecorder();
    void startFFmpegStreaming(const QString &rtmpUrl);

    Session* m_session;
    QString m_name;
    QCamera *m_camera;
    QMediaRecorder *m_recorder;
    QString m_rtmpUrl;
    bool m_isStreaming;
    QCameraViewfinder* m_viewfinder;
    QProcess *m_ffmpegProcess;

};

#endif // CAMERASTREAMER_H
