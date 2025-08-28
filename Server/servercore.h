#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <QTcpServer>

class ServerCore : public QTcpServer
{
    Q_OBJECT

public:
    explicit ServerCore(quint16 port, QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr handle) override;
};

#endif // SERVERCORE_H
