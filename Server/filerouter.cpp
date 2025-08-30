// TODO:ai生成还未修改
#include "filerouter.h"
#include "workordermanager.h"
#include "workorder.h"
#include "clientsession.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QUuid>
#include <QDebug>
#include <QJsonObject>

const int CHUNK_SIZE = 65536; // 分块大小64kb 需要在发送端加以限制

FileRouter *FileRouter::m_instance = nullptr;

FileRouter *FileRouter::instance()
{
    if (!m_instance) {
        m_instance = new FileRouter;
    }
    return m_instance;
}

FileRouter::FileRouter(QObject *parent)
    : QObject(parent)
{
    connect(FileRouter::instance(), &FileRouter::fileUploaded,
            FileRouter::instance(), &FileRouter::newFileUploaded);// 你有新文件可下载广播
}

QString FileRouter::generateFileId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces); // 使用uuid算法创建唯一文件id
}

void FileRouter::handleFileUploadStart(ClientSession *sender, const QJsonObject &data)
{
    QString fileName = data["file_name"].toString();
    qint64 fileSize = data["file_size"].toVariant().toLongLong(); // qt版本过旧 没有toInteger()方法
    QString ticketId = data["ticket_id"].toString(); // 创建工单后将工单号返回给客户端了吗？

    if (fileName.isEmpty() || fileSize <= 0 || ticketId.isEmpty()) {
        qWarning() << "Invalid upload start request from client";
        return;
    }

    // 生成唯一 fileId
    QString fileId = generateFileId();

    // 创建存储目录
    QString dirPath = "uploads/" + ticketId;
    QDir().mkpath(dirPath);

    // 存储路径：使用 fileId 避免冲突
    QString filePath = dirPath + "/" + fileName;

    // 创建上传上下文
    UploadContext ctx;
    ctx.fileId = fileId;
    ctx.fileName = fileName;
    ctx.fileSize = fileSize;
    ctx.ticketId = ticketId;
    ctx.filePath = filePath;
    ctx.receivedBytes = 0;
    ctx.client = sender;
    ctx.uploadTime = QDateTime::currentDateTime();
    ctx.file = new QFile(ctx.filePath);
    if (!ctx.file->open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot create file:" << ctx.filePath;
        delete ctx.file;
        return;
    }

    m_uploads[sender] = ctx;

    // 回复客户端：告知 fileId
    QJsonObject reply;
    reply["type"] = "upload_started";
    QJsonObject rData;
    rData["file_id"] = fileId;
    rData["message"] = "Upload started. Use this fileId for chunks.";
    reply["data"] = rData;

    sender->sendMessage(QJsonDocument(reply).toJson(QJsonDocument::Compact));

    qDebug() << "Upload session started:" << fileName << "(fileId:" << fileId << ")";
}

void FileRouter::handleFileUploadChunk(ClientSession *sender, const QJsonObject &data)
{
    QString fileId = data["file_id"].toString();
    QString chunkDataB64 = data["chunk_data"].toString();

    // 检查是否有活跃上传
    if (!m_uploads.contains(sender)) {
        qWarning() << "Received chunk from client with no active upload session";
        return;
    }

    UploadContext &ctx = m_uploads[sender];

    if (!ctx.file || !ctx.file->isOpen()) {
        qWarning() << "File not open for writing.";
        return;
    }

    // 校验 fileId
    if (ctx.fileId != fileId) {
        qWarning() << "fileId mismatch for client upload";
        return;
    }

    // 解码并写入
    QByteArray chunkData = QByteArray::fromBase64(chunkDataB64.toUtf8());
    if (chunkData.isEmpty()) {
        qWarning() << "Invalid chunk data";
        return;
    }

    if (ctx.file->write(chunkData) == -1) {
        qWarning() << "Failed to write chunk to file:" << ctx.filePath;
        return;
    }

    ctx.receivedBytes += chunkData.size();

    // 判断是否最后一块
    bool isLast = data["is_last"].toBool(); // 传输过程中需要带一个bool变量用来标识是否是最后一块

    if (isLast) {
        // 记录文件信息映射（用于下载）
        QString key = ctx.ticketId + "/" + ctx.fileId;
        m_fileContextMap[key] = ctx; // 保存了一个空的file指针 不过没有影响

        ctx.file->close();
        delete ctx.file;
        ctx.file = nullptr;

        m_uploads.remove(sender); // 移除会话

        // 通知工单系统：文件已上传
        QJsonObject notify;
        notify["type"] = "file_uploaded";
        QJsonObject nData;
        nData["file_id"] = ctx.fileId;
        nData["file_name"] = ctx.fileName;
        nData["file_size"] = ctx.fileSize;
        nData["ticket_id"] = ctx.ticketId;
        notify["data"] = nData;

        emit fileUploaded(sender, notify); // TODO:广播文件上传成功

        qDebug() << "Upload completed:" << ctx.fileName << "saved as" << ctx.filePath;
    }
}

void FileRouter::handleFileDownloadRequest(ClientSession *sender, const QJsonObject &data)
{
    QString fileId = data["file_id"].toString();
    QString ticketId = data["ticket_id"].toString();

    if (fileId.isEmpty() || ticketId.isEmpty()) {
        qWarning() << "Download request missing file_id or ticket_id from client";
        return;
    }

    QString key = ticketId + "/" + fileId; // 构造查询键

    if (!m_fileContextMap.contains(key)) {
        qWarning() << "Requested file not found:" << fileId << "for ticket" << ticketId;
        return;
    }

    UploadContext ctx = m_fileContextMap[key];
    QString filePath = ctx.filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return;
    }

    // 获取分块大小
    int chunkSize = qBound(1024, CHUNK_SIZE, 1024 * 1024);     // 限制在 1KB ~ 1MB 之间

    // 4. 发送文件元信息（file_meta）
    QJsonObject meta;
    meta["type"] = "file_meta";
    QJsonObject mData;
    mData["file_id"] = ctx.fileId;
    mData["file_name"] = ctx.fileName;
    mData["file_size"] = ctx.fileSize;
    mData["ticket_id"] = ctx.ticketId;
    meta["data"] = mData;

    sender->sendMessage(QJsonDocument(meta).toJson(QJsonDocument::Compact));

    // 分块发送文件内容
    QByteArray buffer;
    qint64 totalSent = 0;
    const qint64 fileSize = file.size();

    qDebug() << "Streaming file to client:" << ctx.fileName
             << "(" << fileSize << "bytes)" << "in chunks of" << chunkSize << "bytes";

    while (!file.atEnd()) {
        // 读取一块
        buffer = file.read(chunkSize);
        if (buffer.isEmpty()) {
            qWarning() << "Failed to read chunk from file:" << filePath;
            file.close();
            return;
        }

        // Base64 编码
        QString base64Data = QString::fromUtf8(buffer.toBase64());

        // 构造分块消息
        QJsonObject chunk;
        chunk["type"] = "file_chunk";
        QJsonObject cData;
        cData["data"] = base64Data;
        cData["is_last"] = file.atEnd();  // 用 is_last 判断结束
        chunk["data"] = cData;

        // 发送
        sender->sendMessage(QJsonDocument(chunk).toJson(QJsonDocument::Compact));

        totalSent += buffer.size();
    }

    file.close();

    qDebug() << "Successfully streamed file:" << ctx.fileName
             << "Total sent:" << totalSent << "bytes";
}

void FileRouter::newFileUploaded(ClientSession *sender, const QJsonObject &notify){
    WorkOrder *order = sender->currentTicket();
    if (!order){
        qDebug() << "order not found!";
        return;
    }

    for (ClientSession *client : order->clients) {
        if (client != sender) {
            client->sendMessage(QJsonDocument(notify).toJson(QJsonDocument::Compact));
        }
    }
}
