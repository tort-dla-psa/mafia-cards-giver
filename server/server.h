#pragma once
#include <QVector>
#include <QTcpSocket>
#include <QTcpServer>
#include <QObject>
#include <QThread>
#include <QString>
#include "players.h"
#include "id_gen.h"
#include "rooms.h"
#include "roles.h"
#include "tg_wrapper.h"
#include <iostream>
#include <vector>
#include <memory>

class Server:public QObject{
    Q_OBJECT

    WaitingRoom* wait_room;
    QVector<GameRoom*> rooms;

    IdGenerator gen;
    QThread* tg_thread;
    TgWrapper* wrapper;
public:
    Server(const std::string &api_key, QObject* parent=nullptr)
        :QObject(parent)
    {
        wait_room = new WaitingRoom();
        connect(wait_room, &WaitingRoom::roomJoinRequested,
            this, &Server::onRoomJoinRequested);
        connect(wait_room, &WaitingRoom::roomCreateRequested,
            this, &Server::onRoomCreateRequested);
        wrapper = new TgWrapper(api_key);
        connect(wrapper, &TgWrapper::gotMessage, 
            this, &Server::onTgMessage);
        connect(wrapper, &TgWrapper::gotStartMessage, 
            this, &Server::onTgStartMessage);
        connect(this, &Server::writeTo,
            wrapper, &TgWrapper::WriteTo);
        connect(wait_room, &WaitingRoom::writeTo,
            wrapper, &TgWrapper::WriteTo);
        tg_thread = new QThread();
        wrapper->moveToThread(tg_thread);
    }
    ~Server(){
        tg_thread->quit();
        tg_thread->deleteLater();
        delete wrapper;
        delete wait_room;
    }
public slots:
    void onTgStartMessage(TgWrapper::msg message){
        auto plr = new Player(message->chat->id, message->chat->username);
        wait_room->addPlayer(plr);
    }
    void onTgMessage(TgWrapper::msg message){
        auto id = message->chat->id;
        auto plr_it = wait_room->findPlayer(id);
        if(plr_it){
            wait_room->processMessage(message);
            return;
        }
        for(auto r:rooms){
            plr_it = r->findPlayer(id);
            if(plr_it){
                r->processMessage(message);
                return;
            }
        }
    }
    void run(){
        tg_thread->start();
    }
    void onRoomJoinRequested(Player* plr, std::string id, std::string pass){
        auto predicate = [&id](GameRoom* room){
            if(room->get_id() == id){
                return true;
            }else{
                return false;
            }
        };
        auto room_it = std::find_if(rooms.begin(), rooms.end(), predicate);
        if(room_it == rooms.end()){
        }else{
            auto room = *room_it;
            if(room->get_pass() == pass){
                wait_room->removePlayer(plr);
                room->addPlayer(plr);
            }
        }
    }
    void onRoomCreateRequested(Player* plr, std::string pass){
        wait_room->removePlayer(plr);
        auto adm = new Admin(plr);
        delete plr;

        auto room = new GameRoom(adm, pass);
        room->id = gen.get_id();
        //ISSUE: is it legal?
        connect(room, &GameRoom::writeTo,
            wrapper, &TgWrapper::WriteTo);
        std::cout<<"new room, id:"<<room->get_id()
            <<" pass:"<<room->get_pass()<<"\n";
        emit writeTo(adm->get_id(), "your room id: "+room->get_id()); 
        connect(room, &Room::empty,
            this, &Server::onRoomEmpty);
        rooms.push_back(room);
    }
    void onRoomEmpty(){
        auto room = (GameRoom*)sender();
        rooms.removeOne(room);
        std::cout<<"room id:"<<room->get_id()<<" is empty now\n";
        delete room;
    }
signals:
    void finished();
    void writeTo(int, std::string);
};