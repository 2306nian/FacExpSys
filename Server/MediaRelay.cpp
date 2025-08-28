// MediaRelay.cpp
#include "MediaRelay.h"
#include "ClientSession.h"
#include "WorkOrderManager.h"

MediaRelay* MediaRelay::m_instance = nullptr;

MediaRelay *MediaRelay::instance()
{
    if (!m_instance)
        m_instance = new MediaRelay;
    return m_instance;
}

MediaRelay::MediaRelay(QObject *parent)
    : QObject(parent)
{
}

void MediaRelay::relayMedia(ClientSession *sender, const QByteArray &mediaPacket)
{
    WorkOrder* order = sender->currentTicket();
    if (!order) return;

    for (ClientSession* client : order->clients) {
        if (client != sender) {
            client->sendMessage(mediaPacket);
        }
    }
}
