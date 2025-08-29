// TODO:ai生成还未修改
#include "filerouter.h"
#include "workordermanager.h"
#include "workorder.h"
#include "ClientSession.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QUuid>
#include <QDebug>
#include <QJsonObject>

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
}

QString FileRouter::generateFileId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces); // 使用uuid算法创建唯一文件id
}

void FileRouter::handleFileUploadRequest(ClientSession *sender, const QJsonObject &fileData)
{
    qDebug() << "FileRouter: Handling file upload request from client.";

    QString fileName = fileData["file_name"].toString();
    qint64 fileSize = fileData["file_size"].toVariant().toLongLong(); // tointeger已弃用 使用更安全的转换方式转换
    QString fileType = fileData["file_type"].toString();

    WorkOrder *order = sender -> currentTicket();
    QString ticketId = order -> ticketId;

    if (ticketId.isEmpty() || fileName.isEmpty() || fileSize <= 0) {
        qDebug() << "FileRouter: Invalid file metadata.";
        return;
    }

    if (ticketId.isEmpty()) {
        qDebug() << "FileRouter: Invalid ticket ID:" << ticketId;
        return;
    }

    QString storageDir = "uploads/" + ticketId + "/"; // 服务端文件存放路径
    QDir().mkpath(storageDir);

    QString fileId = generateFileId();
    QString filePath = storageDir + fileId;

    qDebug() << "FileRouter: Uploading file to" << filePath;

    QString fileDataBase64 = fileData["file_data"].toString();
    if (fileDataBase64.isEmpty()) {
        qDebug() << "FileRouter: No file data received.";
        return;
    }

    QByteArray fileDataBinary = QByteArray::fromBase64(fileDataBase64.toUtf8());
    if (fileDataBinary.isEmpty()) {
        qDebug() << "FileRouter: Base64 decoding failed.";
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "FileRouter: Cannot create file:" << file.errorString();
        return;
    }

    if (file.write(fileDataBinary) != fileDataBinary.size()) {
        qDebug() << "FileRouter: Failed to write complete file.";
        file.close();
        QFile::remove(filePath);
        return;
    }
    file.close();

    FileInfo fileInfo;
    fileInfo.id = fileId;
    // fileInfo.ticketId = ticketId;
    fileInfo.fileName = fileName;
    fileInfo.fileType = fileType;
    fileInfo.fileSize = fileSize;
    fileInfo.storagePath = filePath;
    fileInfo.uploadTime = QDateTime::currentDateTime();

    m_files[fileId] = fileInfo;

    qDebug() << "FileRouter: File uploaded successfully. File ID:" << fileId;

    QJsonObject notifyMsg;
    notifyMsg["type"] = "file_available";

    QJsonObject notifyData;
    notifyData["file_id"] = fileId;
    notifyData["file_name"] = fileName;
    notifyData["file_size"] = fileSize;
    notifyData["file_type"] = fileType;
    notifyData["upload_time"] = fileInfo.uploadTime.toString(Qt::ISODate);
    notifyMsg["data"] = notifyData;

    emit fileUploaded(ticketId, notifyMsg); // 广播文件上传成功 TODO:连接信号和槽 向专家端发送有文件上传成功
}

void FileRouter::handleFileDownloadRequest(ClientSession *sender, const QJsonObject &request)
{
    qDebug() << "FileRouter: Handling file download request from client.";

    QString fileId = request["file_id"].toString();
    if (fileId.isEmpty()) {
        qDebug() << "FileRouter: Download request missing file_id.";
        return;
    }

    if (!m_files.contains(fileId)) {
        qDebug() << "FileRouter: File not found:" << fileId;
        return;
    }

    FileInfo fileInfo = m_files[fileId];

    QFile file(fileInfo.storagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "FileRouter: Cannot read file:" << file.errorString();
        return;
    }

    QByteArray fileDataBinary = file.readAll();
    file.close();

    if (fileDataBinary.size() != fileInfo.fileSize) {
        qDebug() << "FileRouter: File read size mismatch.";
        return;
    }

    QString fileDataBase64 = fileDataBinary.toBase64();

    QJsonObject response;
    response["type"] = "file_download_response";

    QJsonObject responseData;
    responseData["file_id"] = fileId;
    responseData["file_name"] = fileInfo.fileName;
    responseData["file_size"] = fileInfo.fileSize;
    responseData["file_type"] = fileInfo.fileType;
    // responseData["upload_time"] = fileInfo.uploadTime.toString(Qt::ISODate);
    responseData["file_data"] = fileDataBase64;
    response["data"] = responseData;

    QJsonDocument doc(response); // 用 QJsonObject 构造 QJsonDocument
    QByteArray downloadFile = doc.toJson(QJsonDocument::Compact);

    sender->sendMessage(downloadFile);

    qDebug() << "FileRouter: File download response sent for ID:" << fileId;
}
