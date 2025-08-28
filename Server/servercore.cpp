#include "servercore.h"
#include "clientsession.h"
#include <QTcpSocket>
#include <QDebug>

ServerCore::ServerCore(quint16 port, QObject *parent)
    : QTcpServer(parent)
{
    if (!listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server:" << errorString();
    }
}

void ServerCore::incomingConnection(qintptr handle)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(handle);

    ClientSession *session = new ClientSession(socket, this);

    connect(session, &ClientSession::disconnected, this, [session]() {
        qDebug() << "Client disconnected:" << session->socket()->peerAddress().toString();
        session->deleteLater();
    });

    qDebug() << "New client connected:" << socket->peerAddress().toString();
}
