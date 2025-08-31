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
#include "rtmpmanager.h" // 新增RTMP管理器
<<<<<<< HEAD
#include "screensharemanager.h"
=======
#include "workorderdao.h"
>>>>>>> fd637657edae9c87494b0eaa1fde8ce1100cd062
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
            DeviceProxy::instance(), &DeviceProxy::requestData);

    connect(this, &ClientSession::controlCommandReceived,
            DeviceProxy::instance(), &DeviceProxy::receiveControlCommand);

    connect(this, &ClientSession::controlCommandReceived, DeviceProxy::instance(),
            &DeviceProxy::receiveControlCommand);

    connect(this, &ClientSession::fileUploadStart,
            FileRouter::instance(), &FileRouter::handleFileUploadStart);

    connect(this, &ClientSession::fileUploadChunk,
            FileRouter::instance(), &FileRouter::handleFileUploadChunk);

    connect(this, &ClientSession::fileDownloadRequest,
            FileRouter::instance(), &FileRouter::handleFileDownloadRequest);

    // 屏幕共享信号连接
    connect(this, &ClientSession::newScreenShare,
            ScreenShareManager::instance(), &ScreenShareManager::startScreenShare);

    // RTMP流信号连接
    connect(this, &ClientSession::rtmpStreamStarted,
            RTMPManager::instance(), &RTMPManager::onStreamStarted);

    connect(this, &ClientSession::rtmpStreamStopped,
            RTMPManager::instance(), &RTMPManager::onStreamStopped);

    connect(this, &ClientSession::rtmpStreamDataReceived,
            RTMPManager::instance(), &RTMPManager::relayStreamData);

}

ClientSession::~ClientSession() // 应该在析构函数中添加一个清理函数 防止意外连接中断时m_uploads不会被清除 不过不必要
{
    if (m_socket) {
        m_socket->deleteLater();
    }

    // 如果正在推流，通知停止
    if (m_isStreaming && m_currentTicket) {
        emit rtmpStreamStopped(this, m_currentTicket->ticketId);
    }
}

//RTMP
void ClientSession::setRtmpStreamUrl(const QString &url)
{
    m_rtmpStreamUrl = url;
}

QString ClientSession::rtmpStreamUrl() const
{
    return m_rtmpStreamUrl;
}

bool ClientSession::isStreaming() const
{
    return m_isStreaming;
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
        WorkOrderManager::instance()->createTicket(
            this,           // ← 传入当前 session
            deviceIds,
            username
<<<<<<< HEAD
            );

        QJsonObject response{
            {"type", "ticket_created"},
            {"data", QJsonObject{{"ticket_id", ticketId}}} // 返回ticket_id
        };
        sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
        emit newTicketCreated(this,ticketId);
=======
        );
>>>>>>> fd637657edae9c87494b0eaa1fde8ce1100cd062
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
    else if (type == "request_device_data") {
        emit deviceDataRequest(this, obj["data"].toObject());
    }
    else if (type == "deviceControl"){
        emit deviceControlRequest(this, obj["data"].toObject());
    }
    else if (type == "control_command") {
        emit controlCommandReceived(obj["data"].toObject());
    }
    else if (type == "file_upload_start") {
        emit fileUploadStart(this, obj["data"].toObject());
    }
    else if (type == "file_upload_chunk") {
        emit fileUploadChunk(this, obj["data"].toObject());
    }
    else if (type == "file_download") {
        emit fileDownloadRequest(this, obj["data"].toObject()); // 发出接收文件请求信号
    }
    // 屏幕共享
    else if (type == "screensharedata") {
        emit newScreenShare(this, data);
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
            {"data", QJsonObject{
                         {"success", success},
                         {"message", success ? "Registration successful" : "Registration failed. Username may already exist."}
                     }}
        };
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
                {"data", QJsonObject{
                             {"success", false},
                             {"message", "Username or password cannot be empty"}
                         }}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        // 查询用户是否存在
        if (!UserDAO::instance()->userExists(username)) {
            QJsonObject response{
                {"type", "login_result"},
                {"data", QJsonObject{
                             {"success", false},
                             {"message", "User does not exist"}
                         }}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        // 验证密码
        bool isValid = UserDAO::instance()->verifyUser(username, password);
        if (!isValid) {
            QJsonObject response{
                {"type", "login_result"},
                {"data", QJsonObject{
                             {"success", false},
                             {"message", "Incorrect password"}
                         }}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }

        // 登录成功：保存用户信息
        QString m_username = username;
        QString m_userType = UserDAO::instance()->getUserType(username);

        // 登录成功响应
        QJsonObject loginResponse{
            {"type", "login_result"},
            {"data", QJsonObject{
                         {"success", true},
                         {"message", "Login successful"},
                         {"user_type", m_userType}  // 告诉客户端角色
                     }}
        };
        sendMessage(QJsonDocument(loginResponse).toJson(QJsonDocument::Compact));

        // 获取并推送设备列表
        {
            QList<DeviceBasicInfo> devices = DeviceDAO::instance()->getAllDevices();
            QJsonArray arr;
            for (const auto &dev : devices) {
                QJsonObject realtime = DeviceDAO::instance()->getDeviceRealtime(dev.deviceId);
                arr.append(QJsonObject{
                    {"device_id", dev.deviceId},
                    {"name", dev.name},
                    {"type", dev.type},
                    {"location", dev.location},
                    {"online_status", dev.onlineStatus},
                    {"pressure", realtime["pressure"]},
                    {"temperature", realtime["temperature"]},
                    {"status", realtime["status"]}
                });
            }
            QJsonObject deviceResponse{
                {"type", "device_list"},
                {"data", QJsonObject{{"devices", arr}}}
            };
            sendMessage(QJsonDocument(deviceResponse).toJson());
        }

        // 如果是工厂端，自动推送“已创建”工单列表
        if (m_userType == "client") {
            QList<WorkOrderRecord> allOrders = WorkOrderDAO::instance()->getClientWorkOrders(m_username);

            QJsonArray arr;
            for (const auto &r : allOrders) {
                arr.append(QJsonObject{
                    {"ticket_id", r.ticketId},
                    {"status", r.status},
                    {"created_at", r.createdAt.toString(Qt::ISODate)},
                    {"device_ids", QJsonArray::fromStringList(r.deviceIds)}
                });
            }

            QJsonObject workOrdersResponse{
                {"type", "work_orders_initial"},
                {"scope", "all"},
                {"data", arr}
            };
            sendMessage(QJsonDocument(workOrdersResponse).toJson(QJsonDocument::Compact));
        }

        // 如果是专家端，可以推送“可承接”工单
        if (m_userType == "expert") {
            QList<WorkOrderRecord> pendingOrders = WorkOrderDAO::instance()->getPendingWorkOrders();

            QJsonArray arr;
            for (const auto &r : pendingOrders) {
                arr.append(QJsonObject{
                    {"ticket_id", r.ticketId},
                    {"client_username", r.clientUsername},
                    {"created_at", r.createdAt.toString(Qt::ISODate)},
                    {"device_ids", QJsonArray::fromStringList(r.deviceIds)}
                });
            }

            QJsonObject pendingResponse{
                {"type", "work_orders"},
                {"scope", "pending"},
                {"data", arr}
            };
            sendMessage(QJsonDocument(pendingResponse).toJson(QJsonDocument::Compact));
        }
    }

    //RTMP
    else if (type == "rtmp_stream_start") {
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();
        QString streamName = dataObj["stream_name"].toString();

        // 验证客户端是否在正确的工单中
        if (m_currentTicket && m_currentTicket->ticketId == ticketId) {
            // 生成RTMP流URL
            QString streamUrl = QString("rtmp://localhost/live/%1_%2")
                                    .arg(ticketId)
                                    .arg(streamName);

            m_rtmpStreamUrl = streamUrl;
            m_isStreaming = true;
            m_streamStartTime = QDateTime::currentDateTime();

            // 通知RTMP管理器开始流
            emit rtmpStreamStarted(this, ticketId, streamUrl);

            // 回复客户端确认
            QJsonObject response{
                {"type", "rtmp_stream_started"},
                {"data", QJsonObject{
                             {"ticket_id", ticketId},
                             {"stream_url", streamUrl},
                             {"message", "Stream started successfully"}
                         }}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));

            qDebug() << "RTMP stream started for ticket:" << ticketId
                     << "by client:" << m_clientIp;
        }
    }
    else if (type == "rtmp_stream_stop") {
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();

        if (m_currentTicket && m_currentTicket->ticketId == ticketId && m_isStreaming) {
            m_isStreaming = false;
            emit rtmpStreamStopped(this, ticketId);

            // 回复客户端确认
            QJsonObject response{
                {"type", "rtmp_stream_stopped"},
                {"data", QJsonObject{
                             {"ticket_id", ticketId},
                             {"message", "Stream stopped successfully"}
                         }}
            };
            sendMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));

            qDebug() << "RTMP stream stopped for ticket:" << ticketId
                     << "by client:" << m_clientIp;
        }
    }
    else if (type == "rtmp_stream_data") {
        // 处理RTMP流数据（如果通过TCP传输）
        QJsonObject dataObj = obj["data"].toObject();
        QString ticketId = dataObj["ticket_id"].toString();
        QByteArray streamData = QByteArray::fromBase64(dataObj["stream_data"].toString().toUtf8());

        if (m_currentTicket && m_currentTicket->ticketId == ticketId && m_isStreaming) {
            // 转发流数据给同工单的其他客户端
            emit rtmpStreamDataReceived(this, streamData);
        }
    }
    else if (type == "get_work_orders") {

        QJsonObject dataObj = obj["data"].toObject();
        QString scope = dataObj["scope"].toString();
        QString m_username = dataObj["username"].toString();
        QList<WorkOrderRecord> records;

        QString m_userType = UserDAO::instance()->getUserType(m_username);

        if (m_userType == "client") {
            if (scope == "all") {
                records = WorkOrderDAO::instance()->getClientWorkOrders(m_username);
            } else if (scope == "pending") {
                records = WorkOrderDAO::instance()->getClientPendingWorkOrders(m_username);
            } else if (scope == "in_progress") {
                records = WorkOrderDAO::instance()->getClientInProgressWorkOrders(m_username);
            } else if (scope == "completed") {
                records = WorkOrderDAO::instance()->getClientCompletedWorkOrders(m_username);
            }
        } else if (m_userType == "expert") {
            if (scope == "pending") {
                records = WorkOrderDAO::instance()->getPendingWorkOrders();
            } else if (scope == "in_progress") {
                records = WorkOrderDAO::instance()->getExpertInProgressWorkOrders(m_username);
            } else if (scope == "completed") {
                records = WorkOrderDAO::instance()->getExpertCompletedWorkOrders(m_username);
            }
        }

        QJsonArray arr;
        for (const auto &r : records) {
            arr.append(QJsonObject{
                {"ticket_id", r.ticketId},
                {"client_username", r.clientUsername},
                {"status", r.status},
                {"created_at", r.createdAt.toString(Qt::ISODate)},
                {"accepted_at", r.acceptedAt.toString(Qt::ISODate)},
                {"completed_at", r.completedAt.toString(Qt::ISODate)},
                {"device_ids", QJsonArray::fromStringList(r.deviceIds)}
            });
        }

        QJsonObject response{
            {"type", "work_orders"},
            {"scope", scope},
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

