#ifndef MESSAGEROUTER_H
#define MESSAGEROUTER_H

#include <QObject>

class ClientSession;

class MessageRouter : public QObject
{
    Q_OBJECT

public:
    static MessageRouter *instance();
    void routeTextMessage(ClientSession *sender, const QByteArray &message);

private:
    explicit MessageRouter(QObject *parent = nullptr);
    static MessageRouter *m_instance;
};

#endif // MESSAGEROUTER_H
