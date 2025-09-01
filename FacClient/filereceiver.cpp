
#include "filereceiver.h"
#include <QDebug>
#include <QJsonDocument>
#include <QDir>
#include <QFileInfo>

// 实现文件接收
// 定义了startFileDownload和onSessionMessage两个接口
// 第一个startFileDownload用于接收到服务端有新文件上传广播file_uploaded之后向服务端提出下载文件请求 其中的fileId在file_uploaded被告知
// 第二个onSessionMessage用于实现文件下载
// 逻辑：服务端file_uploaded广播->客户端提出下载请求->服务端处理请求并分块发送文件->客户端接收下载

const int DOWNLOAD_CHUNK_SIZE = 65536; // 64KB

FileReceiver::FileReceiver(QObject *parent)
    : QObject(parent)
    , m_session(nullptr)
    , m_file(nullptr)
    , m_fileSize(0)
    , m_downloadedBytes(0)
    , m_isDownloading(false)
{
}

FileReceiver::~FileReceiver()
{
    if (m_file) {
        if (m_file->isOpen()) {
            m_file->close();
        }
        delete m_file;
    }
}

bool FileReceiver::startFileDownload(Session* session, const QString& fileId, const QString& savePath)
{
    // 检查是否正在下载
    if (m_isDownloading) {
        emit downloadError("文件下载已在进行中");
        return false;
    }

    // 检查参数
    if (fileId.isEmpty()) {
        emit downloadError("文件ID为空");
        return false;
    }

    // 检查保存路径
    QFileInfo fileInfo(savePath);
    QDir dir(fileInfo.absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit downloadError("无法创建保存目录: " + fileInfo.absolutePath());
            return false;
        }
    }

    // 初始化参数
    m_session = session;
    m_fileId = fileId;
    m_ticketId = session->getTicketId();
    m_savePath = savePath;
    m_fileName = fileInfo.fileName();
    m_downloadedBytes = 0;
    m_fileSize = 0;

    // 创建文件
    m_file = new QFile(savePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        delete m_file;
        m_file = nullptr;
        emit downloadError("无法创建文件: " + savePath);
        return false;
    }

    // 发送下载请求
    QJsonObject request;
    request["type"] = "file_download";
    QJsonObject data;
    data["file_id"] = fileId;
    data["ticket_id"] = session->getTicketId();
    request["data"] = data;

    if (m_session) {
        QJsonDocument doc(request);
        m_session->sendMessage(doc.toJson(QJsonDocument::Compact));
    }

    m_isDownloading = true;
    qDebug() << "File download request sent for file:" << fileId;
    return true;
}

void FileReceiver::onSessionMessage(Session* session, const QJsonObject& message)
{
    QString type = message["type"].toString();
    m_session = session;

    if (type == "file_meta") {
        // 接收到文件元信息
        QJsonObject data = message["data"].toObject();
        m_fileId = data["file_id"].toString();
        m_fileName = data["file_name"].toString();
        m_fileSize = data["file_size"].toVariant().toLongLong();

        emit downloadStarted(m_fileName, m_fileSize);
        qDebug() << "Download started:" << m_fileName << "(" << m_fileSize << "bytes)";

    } else if (type == "file_chunk") {
        // 接收到文件分块
        if (!m_isDownloading || !m_file) {
            qDebug()<<"未启动下载却收到文件块";
            return;
        }

        QJsonObject data = message["data"].toObject();
        QString base64Data = data["data"].toString();
        bool isLast = data["is_last"].toBool();

        // Base64 解码
        QByteArray chunkData = QByteArray::fromBase64(base64Data.toUtf8());

        // 写入文件
        if (m_file->write(chunkData) == -1) {
            qDebug()<<"写入文件失败";
            m_isDownloading = false;
            m_file->close();
            return;
        }

        m_downloadedBytes += chunkData.size();
        emit downloadProgress(m_downloadedBytes, m_fileSize);

        if (isLast) {
            // 下载完成
            m_file->flush();
            m_file->close();
            m_isDownloading = false;
            emit downloadCompleted(m_savePath);
            qDebug() << "File download completed:" << m_savePath;
        }
    }
}
