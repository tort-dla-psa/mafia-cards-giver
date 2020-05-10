#include <QVector>
#include <QTcpSocket>
#include <QTcpServer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QCoreApplication>
#include <QtCore>
#include <iostream>
#include "server.h"

int main(int argc, char* argv[]){
    size_t port = 1337;
    if(argc == 1){
        std::cout<<"WARNING:default port is:"<<port<<"\n";
    }else{
        port = std::atoi(argv[1]);
    }
    QCoreApplication a(argc, argv);
    Server s(port);
    QObject::connect(&s, &Server::finished,
        &a, &QCoreApplication::quit);
    QTimer::singleShot(0, &s, &Server::run);

    return a.exec();
}