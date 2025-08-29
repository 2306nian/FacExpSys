// TODO：ai生成还未修改
#ifndef FILEROUTER_H
#define FILEROUTER_H

#include <QObject>
#include <QMap>
#include <QDateTime>

class ClientSession; // 前向声明ClientSession 减少编译时间

// 存储文件元信息的结构体
struct FileInfo {
    QString id;           // 唯一文件ID
    QString ticketId;     // 所属工单ID
    QString fileName;     // 原始文件名
    QString fileType;     // MIME类型
    qint64 fileSize;     // 文件大小（字节）
    QString storagePath;  // 服务器上的存储路径
    QDateTime uploadTime; // 上传时间
};

class FileRouter : public QObject
{
    Q_OBJECT

public:
    static FileRouter *instance();

    void handleFileUploadRequest(ClientSession *sender, const QJsonObject &fileData);
    void handleFileDownloadRequest(ClientSession *sender, const QJsonObject &request);

signals:
    void fileUploaded(const QString &ticketId, const QJsonObject &notification); // 文件上传成功后 进行工单内部广播

private:
    explicit FileRouter(QObject *parent = nullptr); // 单例实现
    static FileRouter *m_instance;

    QString generateFileId(); // 生成文件唯一ID
    QMap<QString, FileInfo> m_files; // 文件ID -> FileInfo 的注册表 文件的核心数据结构
};

#endif // FILEROUTER_H
