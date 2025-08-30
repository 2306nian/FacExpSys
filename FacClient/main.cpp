#include <QApplication>
#include<pageorder.h>
#include<clientcore.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClientCore core;
    core.show();
    return a.exec();
}
