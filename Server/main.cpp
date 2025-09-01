#include <QCoreApplication>
#include "servercore.h"
#include "database.h"
#include "deviceproxy.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 初始化数据库
   if (!Database::instance()->initialize("/home/haoming/practiceOfComputerScience/FacExpSys/Db")) { // 这里只能用绝对路径建数据库，你们自己改
        qCritical() << "Cannot initialize database. Exiting.";
        return -1;
    }

    DeviceProxy::instance();

    quint16 port = 8888;
    if (argc > 1) {
        port = QString(argv[1]).toUShort();
    }

    ServerCore server(port);
    qDebug() << "Remote Support Server started on port" << port;

    return app.exec();
}
