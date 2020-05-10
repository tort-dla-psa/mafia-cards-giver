#include <QTcpSocket>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QCoreApplication>
#include <iostream>
#include "helpers.h"
#include "roles.h"

class MesGetter:public QObject{
    Q_OBJECT
    QTcpSocket* sock;
public:
    MesGetter(QTcpSocket* sock):QObject(){
        this->sock = sock;
        connect(sock, &QTcpSocket::readyRead,
            this, &MesGetter::onReadyRead);
    }
public slots:
    void run(){}
    void onReadyRead(){
        size_t size;
        read_all(sock, &size, sizeof(size));
        char mes[size+1];
        read_all(sock, &mes, size);
        mes[size] = '\0';
        std::string std_mes = mes;
        QString message = QString::fromStdString(std_mes);
        std::cout.flush();
        emit gotMessage(message);
    }
signals:
    void gotMessage(QString);
};

class MesGetterConsole:public QObject{
    Q_OBJECT
public:
    MesGetterConsole():QObject(){ }
public slots:
    void run(){
        while(true){
            std::string mes;
            std::getline(std::cin, mes);
            auto mes_qt = QString::fromStdString(mes);
            emit gotMessage(mes_qt);
            if(mes == "/quit"){
                break;
            }
        }
        emit ended();
    }
signals:
    void gotMessage(QString);
    void ended();
};

class Client:public QObject{
    Q_OBJECT
    QString addr;
    quint16 port;
    QTcpSocket* sock;
    QThread* thr_con, * thr_sock;
    MesGetter* worker_sock;
    MesGetterConsole* worker_con;
public:
    Client(const std::string &ip, quint16 port):QObject(){
        this->addr = QString::fromStdString(ip);
        this->port = port;
        sock = new QTcpSocket();
        worker_sock = new MesGetter(sock);
        worker_con = new MesGetterConsole();
        thr_con = new QThread();
        thr_sock = new QThread();
    }
    ~Client(){
        if(sock){
            sock->disconnectFromHost();
        }
        thr_con->deleteLater();
        thr_sock->deleteLater();
        delete worker_con;
        delete worker_sock;
    }
public slots:
    void run(){
        sock->connectToHost(addr, port);
        sock->waitForConnected();
        connect(thr_sock, &QThread::started,
            worker_sock, &MesGetter::run);
        connect(worker_sock, &MesGetter::gotMessage,
            this, &Client::onWorkerMes);
        worker_sock->moveToThread(thr_sock);

        connect(thr_con, &QThread::started,
            worker_con, &MesGetterConsole::run);
        connect(worker_con, &MesGetterConsole::gotMessage,
            this, &Client::onConsoleMes);
        connect(worker_con, &MesGetterConsole::ended,
            this, &Client::onConsoleEndRequested);
        worker_con->moveToThread(thr_con);

        thr_con->start();
        thr_sock->start();
    }
    void onWorkerMes(QString mes){
        auto std_str = mes.toStdString();
        std::cout<<"got message:"<<std_str<<"\n";
        bool status;
        int id = mes.toUInt(&status);
        if(status){
            auto role = Converter::get_role(id);
            std::cout<<"now you're: "<<role->get_role_name()<<"\n";
        }
    }
    void onConsoleMes(QString mes){
        auto data = mes.toStdString();
        auto size = data.size();
        write_all(sock, &size, sizeof(size));
        write_all(sock, data.data(), size);
    }
    void onConsoleEndRequested(){
        thr_con->quit();
        thr_sock->quit();
        emit ended();
    }
signals:
    void ended();
};

#include "main.moc"

int main(int argc, char* argv[]){
    std::string addr;
    quint16 port;
    if(argc == 3){
        addr = std::string(argv[1]);
        port = std::atoi(argv[2]);
    }else{
        std::cout << "enter ip:port\n";
        std::string tmp;
        std::cin>>tmp; //127.0.0.1:1337
        auto place = tmp.find(':');
        addr = tmp.substr(0, place);//127.0.0.1
        port = std::stoi(tmp.substr(place+1)); //1337
    }
    QCoreApplication app(argc, argv);
    Client cli(addr, port);
    QObject::connect(&cli, &Client::ended,
        &app, &QCoreApplication::quit);
    QTimer::singleShot(0, &cli, &Client::run);
    return app.exec();
}