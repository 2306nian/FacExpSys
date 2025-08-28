#include <QCoreApplication>
#include "servercore.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    quint16 port = 8888;
    if (argc > 1) {
        port = QString(argv[1]).toUShort();
    }

    ServerCore server(port);

    qDebug() << "Remote Support Server started on port" << port;

    return app.exec();
}
