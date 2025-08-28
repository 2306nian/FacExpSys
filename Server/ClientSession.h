#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H


#include <QTcpSocket>
#include <QByteArray>

class WorkOrder;

class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket* socket, QObject* parent = nullptr);
    ~ClientSession();

    QTcpSocket* socket() const;
    WorkOrder* currentTicket() const;

    void setCurrentTicket(WorkOrder* ticket);
    void sendMessage(const QByteArray& data);

signals:
    void readyForMedia(const QString& ticketId); // 请求开始转发音视频
    void clientDisconnected(ClientSession* session);

private slots:
    void onReadyRead();
    void onSocketDisconnected();

private:
    QTcpSocket* m_socket;
    WorkOrder* m_currentTicket;
    QByteArray m_buffer;
};


#endif // CLIENTSESSION_H
