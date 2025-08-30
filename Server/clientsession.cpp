#include "clientsession.h"
#include "qhostaddress.h"
#include "qjsonarray.h"
#include "workordermanager.h"
#include "workorder.h"
#include "common.h"
#include "mediarelay.h"
#include "messagerouter.h"
#include "filerouter.h"
#include "deviceproxy.h"
#include "userdao.h"
#include "devicedao.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ClientSession::ClientSession(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_currentTicket(nullptr)
    , m_clientIp(socket->peerAddress().toString())
    , m_clientPort(socket->peerPort())
{
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);

    connect(this, &ClientSession::textMessageReceived,
            MessageRouter::instance(), &MessageRouter::routeTextMessage);

    connect(this, &ClientSession::mediaDataReceived,
            MediaRelay::instance(), &MediaRelay::relayMedia);

    connect(this, &ClientSession::deviceDataRequest,
            DeviceProxy::instance(), [](ClientSession* sender, const QJsonObject& req) {
                DeviceProxy::instance()->requestData(sender, req);
            });

    // connect(this, &ClientSession::fileUploadRequest,
    //         FileRouter::instance(), &FileRouter::handleFileUploadRequest);

    connect(this, &ClientSession::fileUploadStart,
            FileRouter::instance(), &FileRouter::handleFileUploadStart);

    connect(this, &ClientSession::fileUploadChunk,
            FileRouter::instance(), &FileRouter::handleFileUploadChunk);

    // connect(this, &ClientSession::fileUploadEnd,
    //         FileRouter::instance(), &FileRouter::handleFileUploadEnd);

    connect(FileRouter::instance(), &FileRouter::fileUploaded,
            this, [this](const QString &ticketId, const QJsonObject &info) {
                // 只有订阅了该工单才推送
                if (m_currentTicket->ticketId == ticketId){
                    sendMessage(QJsonDocument(info).toJson(QJsonDocument::Compact));
                }
            });// 你有新文件可下载广播

    connect(this, &ClientSession::fileDownloadRequest,
            FileRouter::instance(), &FileRouter::handleFileDownloadRequest);
}

ClientSession::~ClientSession() // 应该在析构函数中添加一个清理函数 防止意外连接中断时m_uploads不会被清除 不过不必要
{
    if (m_socket) {
        m_socket->deleteLater();
    }
}

QTcpSocket *ClientSession::socket() const
{
    return m_socket;
}

WorkOrder *ClientSession::currentTicket() const
{
    return m_currentTicket;
}

void ClientSession::setCurrentTicket(WorkOrder *ticket)
{
    m_currentTicket = ticket;
}

void ClientSession::sendMessage(const QByteArray &data)
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(packMessage(data));
    }
}

void ClientSession::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    QByteArray message;
    while (unpackMessage(m_buffer, message)) {
        handleMessage(message);
    }
}

void ClientSession::onDisconnected()
{
    emit disconnected(this);
}

void ClientSession::handleMessage(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Invalid JSON message:" << data;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "create_ticket") {
        QJsonObject dataObj = obj["data"].toObject();
        QString username = dataObj["username"].toString();
        QJsonArray deviceArray = dataObj["device_ids"].toArray();

        QStringList deviceIds;
        for (const QJsonValue &val : deviceArray) {
            deviceIds << val.toString();
        }

        // 使用当前 session 的真实连接信息
        QString ticketId = WorkOrderManager::instance()->createTicket(
            this,           // ← 传入当前 session
            deviceIds,
            username
            );

        QJsonObject response{
            {"type", "ticket_created"},
            {"data", QJsonObject{{"ticket_id", ticketId}}} // 返回ticket_id
        };
        sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
    else if (type == "join_ticket") {
        QString ticketId = obj["data"].toObject()["ticket_id"].toString();
        if (WorkOrderManager::instance()->joinTicket(ticketId, this)) {
            QJsonObject response{
                {"type", "joined_ticket"},
                {"data", QJsonObject{{"ticket_id", ticketId}}}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            emit readyForMedia(ticketId);
        }
    }
    else if (type == "text_msg") {
        emit textMessageReceived(this, data); // 文字传输功能已实现
    }
    else if (type == "video_frame" || type == "audio_chunk") {
        emit mediaDataReceived(this, data);
    }
    else if (type == "request_device_data") {
        emit deviceDataRequest(this, obj["data"].toObject());
    }
    else if (type == "control_command") {
        emit controlCommandReceived(obj["data"].toObject());
    }

    // else if (type == "file_upload") {
    //     emit fileUploadRequest(this, obj["data"].toObject()); // 客户端发送文件
    // }
    else if (type == "file_upload_start") {
        emit fileUploadStart(this, obj["data"].toObject());
    }
    else if (type == "file_upload_chunk") {
        emit fileUploadChunk(this, obj["data"].toObject());
    }
    // else if (type == "file_upload_end") {
    //     emit fileUploadEnd(this, obj["data"].toObject());
    // }
    else if (type == "file_download") {
        emit fileDownloadRequest(this, obj["data"].toObject()); // 发出接收文件请求信号
    }

    // 专家接单
    else if (type == "accept_ticket") {
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();
        QString expertUsername = dataObj["expert_username"].toString();
        QString expertIp = m_socket->peerAddress().toString();
        int expertPort = m_socket->peerPort();

        WorkOrderManager::instance()->acceptTicket(ticketId, expertUsername, expertIp, expertPort);
    }

    // 工单完成
    else if (type == "complete_ticket") {
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();
        QString description = dataObj["description"].toString();
        QString solution = dataObj["solution"].toString();

        WorkOrderManager::instance()->completeTicket(ticketId, description, solution);
    }


    // 注册
    else if (type == "register") {
        QJsonObject dataObj = obj["data"].toObject();
        QString username = dataObj["username"].toString();
        QString password = dataObj["password"].toString();
        QString userType = dataObj["user_type"].toString();  // "client" 或 "expert"
        QDateTime createdAt = QDateTime::currentDateTime();

        // 调用 UserDAO 注册
        bool success = UserDAO::instance()->registerUser(username, password, userType, createdAt);

        // 回复客户端
        QJsonObject response{
            {"type", "register_result"},
            {"success", success}
        };
        if (!success) {
            response["message"] = "Registration failed. Username may already exist.";
        }
        sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }

    // 登陆——验证
    else if (type == "login") {
        QJsonObject dataObj = obj["data"].toObject();
        QString username = dataObj["username"].toString();
        QString password = dataObj["password"].toString();

        // 参数检查
        if (username.isEmpty() || password.isEmpty()) {
            QJsonObject response{
                {"type", "login_result"},
                {"success", false},
                {"message", "Username or password cannot be empty"}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        // 查询用户是否存在
        if (!UserDAO::instance()->userExists(username)) {
            QJsonObject response{
                {"type", "login_result"},
                {"success", false},
                {"message", "User does not exist"}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        // 验证密码
        bool isValid = UserDAO::instance()->verifyUser(username, password);
        if (!isValid) {
            QJsonObject response{
                {"type", "login_result"},
                {"success", false},
                {"message", "Incorrect password"}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        // 登录成功
        QString userType = UserDAO::instance()->getUserType(username);

        QJsonObject response{
            {"type", "login_result"},
            {"success", true},
            {"message", "Login successful"}
        };
        sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }

    // 客户端登陆时需要获取设备基础信息，实时数据由另一边转发
    else if (type == "get_device_list") {
        QList<DeviceBasicInfo> devices = DeviceDAO::instance()->getAllDevices();
        QJsonArray arr;
        for (const auto &dev : devices) {
            // 获取该设备的实时数据
            QJsonObject realtime = DeviceDAO::instance()->getDeviceRealtime(dev.deviceId);

            arr.append(QJsonObject{
                {"device_id", dev.deviceId},
                {"name", dev.name},
                {"type", dev.type},
                {"location", dev.location},
                {"online_status", dev.onlineStatus},
                {"pressure", realtime["pressure"]},      // 加入实时数据
                {"temperature", realtime["temperature"]},
                {"status", realtime["status"]}
            });
        }
        QJsonObject response{
            {"type", "device_list"},
            {"data", arr}
        };
        sendMessage(QJsonDocument(response).toJson());
    }

    else {
        qWarning() << "Unknown message type:" << type;
    }
}

QString ClientSession::clientIp() const
{
    return m_clientIp;
}

int ClientSession::clientPort() const
{
    return m_clientPort;
}
