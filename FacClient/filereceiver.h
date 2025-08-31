#ifndef FILERECEIVER_H
#define FILERECEIVER_H

#include <QObject>
#include <QFile>
#include <QJsonObject>
#include "session.h"  // 假设你的 Session 类头文件

class FileReceiver : public QObject
{
    Q_OBJECT

public:
    explicit FileReceiver(QObject *parent = nullptr);
    ~FileReceiver();

    // 启动文件下载
    bool startFileDownload(Session* session, const QString& fileId, const QString& savePath);

    // 处理服务端消息
    void onSessionMessage(Session* session, const QJsonObject& message);

signals:
    void downloadStarted(const QString& fileName, qint64 fileSize);
    void downloadProgress(qint64 downloaded, qint64 total);
    void downloadCompleted(const QString& filePath);
    void downloadError(const QString& error);

private:
    Session* m_session;
    QFile* m_file;
    QString m_fileId;
    QString m_ticketId;
    QString m_fileName;
    QString m_savePath;
    qint64 m_fileSize;
    qint64 m_downloadedBytes;
    bool m_isDownloading;
};

#endif // FILERECEIVER_H
