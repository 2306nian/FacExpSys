#ifndef FILESENDER_H
#define FILESENDER_H

#include <QObject>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include "session.h"

class FileSender : public QObject
{
    Q_OBJECT

public:
    explicit FileSender(QObject *parent = nullptr);
    ~FileSender();

    // 文件发送控制
    bool startFileUpload(Session* m_session, const QString& filePath);
    void cancelUpload();

    // 状态查询
    bool isUploading() const { return m_isUploading; }
    qint64 getUploadedBytes() const { return m_uploadedBytes; }
    qint64 getTotalBytes() const { return m_totalBytes; }
    double getProgress() const { return m_totalBytes > 0 ? (double)m_uploadedBytes / m_totalBytes : 0.0; }

signals:
    void uploadStarted();
    void uploadProgress(qint64 uploadedBytes, qint64 totalBytes);
    void uploadCompleted(const QString& fileId);
    void uploadError(const QString& error);

private slots:
    void sendNextChunk();
    void onSessionMessage(Session* session, const QJsonObject& message);

private:
    // void cleanup();

    Session *m_session;
    QFile* m_file;

    QString m_fileId;
    QString m_filePath;
    QString m_ticketId;
    QString m_fileName;
    qint64 m_fileSize;

    bool m_isUploading;
    qint64 m_uploadedBytes;
    qint64 m_totalBytes;
    qint64 m_chunkSize;
};

#endif // FILESENDER_H
