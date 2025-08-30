#ifndef STREAMMANAGER_H
#define STREAMMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QMutex>

class RTMPSession;

class StreamManager : public QObject
{
    Q_OBJECT

public:
    explicit StreamManager(QObject *parent = nullptr);

    void publishStream(const QString &streamName, RTMPSession *publisher);
    void unpublishStream(const QString &streamName, RTMPSession *publisher);
    void addSubscriber(const QString &streamName, RTMPSession *subscriber);
    void removeSubscriber(const QString &streamName, RTMPSession *subscriber);

    bool isStreamPublished(const QString &streamName) const;
    QList<QString> getPublishedStreams() const;

signals:
    void streamDataReceived(const QString &streamName, const QByteArray &data);

private:
    QMap<QString, RTMPSession*> m_publishers;
    QMap<QString, QList<RTMPSession*>> m_subscribers;
    mutable QMutex m_mutex;
};

#endif // STREAMMANAGER_H
