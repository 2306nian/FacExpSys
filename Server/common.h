#ifndef COMMON_H
#define COMMON_H

#include <QtGlobal>
#include <QByteArray>
#include <QDataStream>

// 消息前缀：4字节长度 + JSON数据
inline QByteArray packMessage(const QByteArray& data) {
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << quint32(data.size());
    packet.append(data);
    return packet;
}

inline bool unpackMessage(QByteArray& buffer, QByteArray& message) {
    if (buffer.size() < 4) return false;

    QDataStream stream(buffer);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 length;
    stream >> length;

    if (buffer.size() < 4 + int(length)) return false;

    message = buffer.mid(4, length);
    buffer.remove(0, 4 + length);
    return true;
}


#endif // COMMON_H
