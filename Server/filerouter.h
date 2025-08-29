// TODO：ai生成还未修改
#ifndef FILEROUTER_H
#define FILEROUTER_H

#include <QObject>
#include <QMap>
#include <QFile>
#include <QHash>
#include <QDateTime>

class ClientSession; // 前向声明ClientSession 减少编译时间

struct UploadContext {
    QString fileId;
    QString fileName;
    qint64 fileSize;
    QString ticketId;
    QString filePath;
    qint64 receivedBytes;
    // QFile* file;
    ClientSession* client;
    QDateTime uploadTime;
};

class FileRouter : public QObject
{
    Q_OBJECT

public:
    static FileRouter *instance();

    // void handleFileUploadRequest(ClientSession *sender, const QJsonObject &fileData);
    void handleFileUploadStart(ClientSession *sender, const QJsonObject &data);
    void handleFileUploadChunk(ClientSession *sender, const QJsonObject &data);
    void handleFileUploadEnd(ClientSession *sender, const QJsonObject &data);
    void handleFileDownloadRequest(ClientSession *sender, const QJsonObject &request);

    QString generateFileId();

signals:
    void fileUploaded(const QString &ticketId, const QJsonObject &notification); // 文件上传成功后 进行工单内部广播

private:
    explicit FileRouter(QObject *parent = nullptr); // 单例实现
    static FileRouter *m_instance;

    // 当前上传会话：以 ClientSession 为键
    QHash<ClientSession*, UploadContext> m_uploads;

    // 文件信息映射：ticketId/fileId -> 文件信息
    QHash<QString, UploadContext> m_fileContextMap;
};

#endif // FILEROUTER_H
