#include "streammanager.h"
#include "rtmpsession.h"
#include <QDebug>

StreamManager::StreamManager(QObject *parent)
    : QObject(parent)
{
}

void StreamManager::publishStream(const QString &streamName, RTMPSession *publisher)
{
    QMutexLocker locker(&m_mutex);

    if (m_publishers.contains(streamName)) {
        qWarning() << "Stream already published:" << streamName;
        return;
    }

    m_publishers[streamName] = publisher;
    qDebug() << "Stream published:" << streamName;
}

void StreamManager::unpublishStream(const QString &streamName, RTMPSession *publisher)
{
    QMutexLocker locker(&m_mutex);

    if (m_publishers.value(streamName) == publisher) {
        m_publishers.remove(streamName);

        // 通知所有订阅者流已结束
        if (m_subscribers.contains(streamName)) {
            m_subscribers[streamName].clear();
        }

        qDebug() << "Stream unpublished:" << streamName;
    }
}

void StreamManager::addSubscriber(const QString &streamName, RTMPSession *subscriber)
{
    QMutexLocker locker(&m_mutex);

    if (m_publishers.contains(streamName)) {
        m_subscribers[streamName].append(subscriber);
        qDebug() << "Subscriber added to stream:" << streamName;
    }
}

void StreamManager::removeSubscriber(const QString &streamName, RTMPSession *subscriber)
{
    QMutexLocker locker(&m_mutex);

    if (m_subscribers.contains(streamName)) {
        m_subscribers[streamName].removeOne(subscriber);
        qDebug() << "Subscriber removed from stream:" << streamName;
    }
}

bool StreamManager::isStreamPublished(const QString &streamName) const
{
    QMutexLocker locker(&m_mutex);
    return m_publishers.contains(streamName);
}

QList<QString> StreamManager::getPublishedStreams() const
{
    QMutexLocker locker(&m_mutex);
    return m_publishers.keys();
}
