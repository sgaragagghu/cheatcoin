#include "mainwindow.h"
#include <QApplication>
#include <unistd.h>
#include "../../dnet_main.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    if (chdir("/mnt/sdcard2/.dnet"))
        chdir("/mnt/sdcard/.dnet");

    dnet_init(argc, argv);

    return a.exec();
}
