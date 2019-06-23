#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    qDebug() << "Main threadID is :" << QThread::currentThreadId();
    w.show();

    return a.exec();
}
