#pragma once
#include <QVector>
#include <QTcpSocket>
#include <QTcpServer>
#include <QObject>
#include <QString>
#include "players.h"
#include "id_gen.h"
#include "rooms.h"
#include "roles.h"
#include <tgbot/tgbot.h>
#include <memory>
#include <iostream>
#include <vector>

class Server:public QObject{
    Q_OBJECT

    WaitingRoom* wait_room;
    QVector<GameRoom*> rooms;

    IdGenerator gen;
    const std::string key;
    std::unique_ptr<TgBot::Bot> bot;
    TgBot::CurlHttpClient curl;
public:
    Server(const std::string &key, QObject* parent=nullptr)
        :QObject(parent),
        key(key)
    {
        wait_room = new WaitingRoom();
        connect(wait_room, &WaitingRoom::roomJoinRequested,
            this, &Server::onRoomJoinRequested);
        connect(wait_room, &WaitingRoom::roomCreateRequested,
            this, &Server::onRoomCreateRequested);
        connect(wait_room, &WaitingRoom::writeTo,
            this, &Server::writeTo);
    }
    ~Server(){
        delete wait_room;
    }
public slots:
    void onTgStartMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        auto plr_it = wait_room->findPlayer(id);
        if(plr_it && *plr_it){
            std::cerr<<"double /start from "<<(*plr_it)->get_nick()<<"\n";
            (*plr_it)->id = id;
            return;
        }
        for(auto r:rooms){
            plr_it = r->findPlayer(id);
            if(plr_it && *plr_it){
                std::cerr<<"double /start from "<<(*plr_it)->get_nick()<<"\n";
                (*plr_it)->id = id;
                return;
            }
        }
        auto plr = new Player(id, message->chat->username);
        wait_room->addPlayer(plr);
    }
    void onTgMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        auto plr_it = wait_room->findPlayer(id);
        if(plr_it && *plr_it){
            wait_room->processMessage(message);
            return;
        }
        for(auto r:rooms){
            plr_it = r->findPlayer(id);
            if(plr_it && *plr_it){
                r->processMessage(message);
                return;
            }
        }
    }
    void run(){
        bot = std::make_unique<TgBot::Bot>(key, curl);
        bot->getEvents().onAnyMessage(
            [this](TgBot::Message::Ptr message) {
                if(message->text == "/start"){
                    onTgStartMessage(message);
                }else{
                    onTgMessage(message);
                }
        });
        try {
            TgBot::TgLongPoll longPoll(*bot);
            while (true) {
                longPoll.start();
            }
        } catch (TgBot::TgException& e) {
            std::cout<<"ERROR: "<<e.what()<<std::endl;
        }
    }
    void onRoomJoinRequested(Player* plr, std::string id, std::string pass){
	    std::cout<<"got join request, user:"<<plr->get_nick()<<" room:"<<id<<" pass:"<<pass<<"\n";
        auto predicate = [&id](GameRoom* room){
            if(room->get_id() == id){
                return true;
            }else{
                return false;
            }
        };
        auto room_it = std::find_if(rooms.begin(), rooms.end(), predicate);
        if(room_it == rooms.end() || !*room_it){
            writeTo(plr->get_id(), "there's no room with id "+id);
        }else{
            auto room = *room_it;
            if(room->get_pass() == pass){
                wait_room->removePlayer(plr);
                room->addPlayer(plr);
                writeTo(plr->get_id(),"welcome to room  '"+room->get_id()+"'");
            }else{
                writeTo(plr->get_id(),"your pass '"+pass+"' is invalid");
            }
        }
    }
    void onRoomCreateRequested(Player* plr, std::string pass){
        wait_room->removePlayer(plr);
        auto adm = new Admin(plr);
        delete plr;

        auto room = new GameRoom(adm, pass);
        room->id = gen.get_id();
        connect(room, &GameRoom::writeTo,
            this, &Server::writeTo);
        std::cout<<"new room, id:"<<room->get_id()
            <<" pass:"<<room->get_pass()<<"\n";
        writeTo(adm->get_id(), "your room id: "+room->get_id()+
            " pass:"+room->get_pass()); 
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
    void writeTo(int id, std::string mes){
        try{
            bot->getApi().sendMessage(id, mes);
        } catch (TgBot::TgException& e) {
            std::cout<<"ERROR: "<<e.what()<<std::endl;
        }
    }
signals:
    void finished();
};
