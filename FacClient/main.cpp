#include <QApplication>
#include<pageorder.h>
#include<clientcore.h>
#include<chatroom.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClientCore core;
    core.show();
    // ChatRoom c1;
    // c1.show();
    return a.exec();
}
