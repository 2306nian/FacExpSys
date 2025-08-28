#include "mediarelay.h"
#include "clientsession.h"
#include "workorder.h"
#include "workordermanager.h"

MediaRelay *MediaRelay::m_instance = nullptr;

MediaRelay *MediaRelay::instance()
{
    if (!m_instance) {
        m_instance = new MediaRelay;
    }
    return m_instance;
}

MediaRelay::MediaRelay(QObject *parent)
    : QObject(parent)
{
}

void MediaRelay::relayMedia(ClientSession *sender, const QByteArray &mediaData)
{
    WorkOrder *order = sender->currentTicket();
    if (!order) return;

    for (ClientSession *client : order->clients) {
        if (client != sender) {
            client->sendMessage(mediaData);
        }
    }
}
