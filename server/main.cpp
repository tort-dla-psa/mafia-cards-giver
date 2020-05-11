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
#include "tg_wrapper.h"

Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(TgWrapper::msg)
int main(int argc, char* argv[]){
	qRegisterMetaType<TgWrapper::msg>("msg");
	qRegisterMetaType<std::string>("std_str");
    std::string key;
    if(argc != 2){
        std::cerr<<"ERROR:provide api key\n";
        return 1;
    }else{
        key = std::string(argv[1]);
    }
    QCoreApplication a(argc, argv);
    Server s(key);
    QObject::connect(&s, &Server::finished,
        &a, &QCoreApplication::quit);
    QTimer::singleShot(0, &s, &Server::run);

    return a.exec();
}
