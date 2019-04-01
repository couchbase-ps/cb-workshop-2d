#include <QApplication>
#include <QDebug>
#include <stdio.h>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    qDebug() << "Starting demo application ...";
    CBDataSourceFactory::Create("couchbase://localhost/travel-sample", QString("application"), QString("couchbase"));

    //Window
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    int applicationresult = a.exec();
    CBDataSourceFactory::Instance().Destroy();
    return applicationresult;
}
