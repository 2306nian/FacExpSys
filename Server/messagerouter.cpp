#include "messagerouter.h"
#include "clientsession.h"
#include "workorder.h"
#include "workordermanager.h"
#include <QDebug>

MessageRouter *MessageRouter::m_instance = nullptr;

MessageRouter *MessageRouter::instance()
{
    if (!m_instance) {
        m_instance = new MessageRouter;
    }
    return m_instance;
}

MessageRouter::MessageRouter(QObject *parent)
    : QObject(parent)
{
}

void MessageRouter::routeTextMessage(ClientSession *sender, const QByteArray &message)
{
    WorkOrder *order = sender->currentTicket();
    if (!order){
        qDebug() << "order not found!";
        return;
    }

    for (ClientSession *client : order->clients) {
        if (client != sender) {
            client->sendMessage(message);
        }
    }
}
