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
    // 写入长度头
    stream << static_cast<quint32>(data.size());
    // 全部使用 QDataStream 写入数据体
    stream.writeRawData(data.constData(), data.size()); // 此处之前混合使用了stream和append进行打包 可能出现问题
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
