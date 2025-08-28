#ifndef MEDIARELAY_H
#define MEDIARELAY_H


#include <QObject>

class ClientSession;

class MediaRelay : public QObject
{
    Q_OBJECT
public:
    static MediaRelay* instance();
    void relayMedia(ClientSession* sender, const QByteArray& mediaPacket);

private:
    explicit MediaRelay(QObject* parent = nullptr);
    static MediaRelay* m_instance;
};


#endif // MEDIARELAY_H
