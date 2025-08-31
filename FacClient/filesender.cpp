#include "filesender.h"
#include <QDebug>
#include <QFileInfo>
#include <QUuid>
#include <QDir>

// 向session提供了两个接口
// 第一个为startFileUpload接收参数为(Session* m_session, const QString& filePath)
// 第二个为onSessionMessage接收参数为(Session* session, const QJsonObject& message) 其中message为服务端传回的json
// 需要将uploadStarted()信号在session中做连接
// 逻辑：客户端提出上传文件请求->服务端处理请求并返回fileid->客户端上传文件

const int CHUNK_SIZE = 65536; // 64KB

FileSender::FileSender(QObject *parent)
    : QObject(parent)
    , m_session(nullptr)
    , m_file(nullptr)
    , m_isUploading(false)
    , m_uploadedBytes(0)
    , m_totalBytes(0)
    , m_chunkSize(CHUNK_SIZE)
{
}

FileSender::~FileSender()
{
    // cleanup();
}

bool FileSender::startFileUpload(Session* m_session, const QString& filePath)
{
    // 判断是否在上传文件
    if (m_isUploading) {
        emit uploadError("文件上传已在进行中");
        return false;
    }

    // 检查文件路径是否正确
    if (filePath.isEmpty()) {
        emit uploadError("文件路径为空");
        return false;
    }

    // 检查文件是否存在
    QFile file(filePath);
    if (!file.exists()) {
        emit uploadError("文件不存在: " + filePath);
        return false;
    }

    // 检查文件是否可打开
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadError("无法打开文件: " + filePath);
        return false;
    }

    // 初始化文件信息
    m_filePath = filePath;
    // 获取工单号
    m_ticketId = m_session->getTicketId();
    m_fileSize = file.size();
    m_fileName = QFileInfo(filePath).fileName();
    m_uploadedBytes = 0;
    m_totalBytes = m_fileSize;

    // 保存文件句柄
    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        delete m_file;
        m_file = nullptr;
        emit uploadError("无法打开文件进行读取: " + filePath);
        return false;
    }

    // 发送上传开始请求
    QJsonObject startMessage;
    startMessage["type"] = "file_upload_start";
    QJsonObject data;
    data["file_name"] = m_fileName;
    data["file_size"] = m_fileSize;
    data["ticket_id"] = m_ticketId;
    startMessage["data"] = data;

    if (m_session) {
        QJsonDocument doc(startMessage);
        m_session->sendMessage(doc.toJson(QJsonDocument::Compact));
    }

    m_isUploading = true;
    emit uploadStarted();
    qDebug() << "File upload started:" << m_fileName << "(" << m_fileSize << "bytes)";
    return true;
}

void FileSender::sendNextChunk()
{
    if (!m_isUploading || !m_file || !m_session) {
        return;
    }

    if (m_file->atEnd()) {
        // 上传完成
        m_isUploading = false;
        // cleanup();
        emit uploadCompleted(m_fileId); // 上传完成信号
        qDebug() << "File upload completed:" << m_fileName;
        return;
    }

    // 读取下一个块
    QByteArray chunkData = m_file->read(m_chunkSize);
    if (chunkData.isEmpty()) {
        // cleanup();
        emit uploadError("读取文件块失败");
        return;
    }

    // Base64 编码
    QString base64Data = QString::fromUtf8(chunkData.toBase64());

    // 构造分块消息
    QJsonObject chunkMessage;
    chunkMessage["type"] = "file_upload_chunk";
    QJsonObject data;
    data["file_id"] = m_fileId;
    data["chunk_data"] = base64Data;
    data["is_last"] = m_file->atEnd();
    chunkMessage["data"] = data;

    // 发送分块
    QJsonDocument doc(chunkMessage);
    m_session->sendMessage(doc.toJson(QJsonDocument::Compact));

    m_uploadedBytes += chunkData.size();
    emit uploadProgress(m_uploadedBytes, m_totalBytes);
}

void FileSender::onSessionMessage(Session* session, const QJsonObject& message)
{
    QString type = message["type"].toString();
    m_session = session;

    if (type == "upload_started") {
        // 服务端确认可以开始上传
        QJsonObject data = message["data"].toObject();
        QString fileId = data["file_id"].toString();

        m_fileId = fileId;
        while(m_isUploading){
        sendNextChunk();
        }
    }

    else {
        qDebug()<<"upload error!";
    }
}

// void FileSender::cancelUpload()
// {
//     if (m_isUploading) {
//         m_isUploading = false;
//         cleanup();
//         emit uploadError("上传已取消");
//     }
// }

// void FileSender::cleanup()
// {
//     if (m_file) {
//         if (m_file->isOpen()) {
//             m_file->close();
//         }
//         delete m_file;
//         m_file = nullptr;
//     }
// }
