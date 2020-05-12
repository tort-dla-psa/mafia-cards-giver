#pragma once
#include "players.h"
#include "id_gen.h"
#include "rooms.h"
#include "roles.h"
#include <tgbot/tgbot.h>
#include <memory>
#include <iostream>
#include <vector>

class Server{
    WaitingRoom wait_room;
    std::vector<std::unique_ptr<GameRoom>> rooms;

    IdGenerator gen;
    const std::string key;
    std::unique_ptr<TgBot::Bot> bot;
    TgBot::CurlHttpClient curl;
    void process_reqs(std::shared_ptr<Player> plr, const std::vector<std::shared_ptr<Room::request>> reqs){
        for(auto &req:reqs){
            auto write_cast = std::dynamic_pointer_cast<Room::write_request>(req);
            if(write_cast){
                writeTo(write_cast->id, write_cast->mes);
                continue;
            }
            auto join_cast = std::dynamic_pointer_cast<Room::join_request>(req);
            if(join_cast){
                onRoomJoinRequested(plr, join_cast->id, join_cast->pass);
                continue;
            }
            auto create_cast = std::dynamic_pointer_cast<Room::create_request>(req);
            if(create_cast){
                onRoomCreateRequested(plr, create_cast->pass);
                continue;
            }
        }
    }
public:
    Server(const std::string &key)
        :key(key)
    {}
    ~Server(){}

    void onTgStartMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        std::cout<<"git /start message in chat "<<id<<"\n";
        auto plr = wait_room.findPlayer(id);
        if(plr){
            std::cerr<<"double /start from "<<plr->get_nick()<<"\n";
            plr->id = id;
            return;
        }
        for(auto &r:rooms){
            plr = r->findPlayer(id);
            if(plr){
                std::cerr<<"double /start from "<<plr->get_nick()<<"\n";
                plr->id = id;
                return;
            }
        }
        plr = std::make_shared<Player>(id, message->chat->username);
        wait_room.addPlayer(plr);
    }

    void onTgMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        auto plr = wait_room.findPlayer(id);
        if(plr){
            wait_room.processMessage(message);
            auto reqs = wait_room.get_requests();
            process_reqs(plr, reqs);
            return;
        }
        for(auto &r:rooms){
            plr = r->findPlayer(id);
            if(plr){
                r->processMessage(message);
                auto reqs = r->get_requests();
                process_reqs(plr, reqs);
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
    void onRoomJoinRequested(std::shared_ptr<Player> plr, std::string id, std::string pass){
	    std::cout<<"got join request, user:"<<plr->get_nick()<<" room:"<<id<<" pass:"<<pass<<"\n";
        auto predicate = [&id](const auto &room){
            if(room->get_id() == id){
                return true;
            }else{
                return false;
            }
        };
        auto room_it = std::find_if(rooms.begin(), rooms.end(), predicate);
        if(room_it == rooms.end()){
            writeTo(plr->get_id(), "there's no room with id "+id);
        }else{
            auto &room = *room_it;
            if(room->get_pass() == pass){
                auto plr_ = room->findPlayer(plr->get_id());
                if(plr_){
                    writeTo(plr_->get_id(),"you're allready in room '"+room->get_id()+"'");
                    return;
                }
                wait_room.removePlayer(plr);
                room->addPlayer(plr);
                writeTo(plr->get_id(),"welcome to room '"+room->get_id()+"'");
            }else{
                writeTo(plr->get_id(),"your pass '"+pass+"' is invalid");
            }
        }
    }
    void onRoomCreateRequested(std::shared_ptr<Player> plr, std::string pass){
        wait_room.removePlayer(plr);
        auto adm = std::make_shared<Admin>(plr);
        auto room = std::make_unique<GameRoom>(adm, pass);
        room->id = gen.get_id();
        std::cout<<"new room, id:"<<room->get_id()
            <<" pass:"<<room->get_pass()<<"\n";
        writeTo(adm->get_id(), "your room id: "+room->get_id()+
            " pass:"+room->get_pass()); 
        rooms.emplace_back(std::move(room));
    }
    template<class It>
    void onRoomEmpty(It room_it){
        auto &room = *room_it;
        rooms.erase(room_it);
        std::cout<<"room id:"<<room->get_id()<<" is empty now\n";
    }
    void writeTo(int id, std::string mes){
        try{
            bot->getApi().sendMessage(id, mes);
        } catch (TgBot::TgException& e) {
            std::cout<<"ERROR: "<<e.what()<<std::endl;
        }
    }
};
