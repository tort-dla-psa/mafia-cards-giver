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
        std::cout<<"got /start message in chat "<<id<<"\n";
        auto plr = wait_room.findPlayer(id);
        if(plr){
            std::cerr<<"double /start from "<<plr->get_nick()<<"\n";
            plr->id = id;
            writeTo(plr->id, "You are already in waiting room");
            return;
        }
        for(auto &r:rooms){
            plr = r->findPlayer(id);
            if(plr){
                std::cerr<<"double /start from "<<plr->get_nick()<<"\n";
                plr->id = id;
                writeTo(plr->id, "You are already in waiting room");
                return;
            }
        }
        plr = std::make_shared<Player>(id, message->chat->username);
        wait_room.addPlayer(plr);
    }

    void onTgQuitMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        std::cout<<"got /quit message in chat "<<id
            <<" from @"<<message->chat->username<<"\n";
        std::shared_ptr<Player> plr;
        {
            auto wait_ptr = wait_room.findPlayer(id);
            if(wait_ptr){
                std::string mes = "You are already in waiting room";
                writeTo(id, mes);
                return;
            }
        }
        for(auto r_it = rooms.begin(); r_it != rooms.end(); r_it++){
            auto &r = *r_it;
            //find player and room where event happened
            plr = r->findPlayer(id);
            if(!plr){
                continue;
            }
            std::stringstream ss;
            ss<<"User @"<<plr->get_nick()<<" exited room "<<r->get_id();
            std::cout<<ss.str()<<"\n";
            auto adm = r->getAdmin();
            if(adm->get_id() != plr->get_id()){ //exited user is not admin
                //send exited user info to room admin
                r->removePlayer(plr); //remove exited player
                writeTo(plr->get_id(), "you've successfully exited room "+r->get_id()+
                    "\nrun cmd /join to enter another room or /help to list available commands");
                writeTo(adm->get_id(), ss.str());
            }else{ //exited user is admin
		writeTo(adm->get_id(), "you've just successfully closed room "+r->get_id()+
                        "\nrun cmd /join to enter new room, /create to make a new one or /help to list available commands");
                r->removePlayer(plr); //remove exited player
                plr = std::make_shared<Player>(adm->get_id(), adm->get_nick());
                r->adm = nullptr;
                std::string mes = "room host @"+plr->get_nick()+" closed room, now it'll be destroyed";
                auto pl_it = r->begin();
                while(pl_it!=r->end()){
                    auto pl = *pl_it;
                    writeTo(pl->get_id(), mes);
                    writeTo(pl->get_id(), "you've just successfully exited room "+r->get_id()+
                        "\nrun cmd /join to enter new room, /create to make a new one or /help to list available commands");
                    pl_it = r->removePlayer(pl_it); //remove player
                    wait_room.addPlayer(pl); //add him to waiting room
                }
            }
            wait_room.addPlayer(plr); //add him to waiting room
            if(r->size() == 0){
                std::cout<<"deleting empty room "<<r->get_id()<<"\n";
                r_it = rooms.erase(r_it);
            }
            return;
        }
    }

    void onTgRolesMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        std::stringstream ss;
        ss<<"Available roles:\n"
            <<"\ncops\n"
            <<"lovers\n"
            <<"mafias\n"
            <<"medics\n"
            <<"killers";
        writeTo(id, ss.str());
    }

    void onTgHelpMessage(TgBot::Message::Ptr message){
        auto id = message->chat->id;
        auto mes = "Welcome to mafia bot! I support these commands:\n"
            "/create - make room\n"
            "/create 1 - make room with password \"1\"\n"
            "/join abcdef - join room with id \"abcdef\"\n"
            "/join abcdef 1 - join room with id \"abcdef\" and pass 1\n"
            "/shuffle - command for admin to give roles\n"
            "/set [role] [num] - set quantity of a role to given number\n"
            "/roles - print out roles\n"
            "/quit - exit room\n"
            "/help - this message";
        writeTo(id, mes);
        std::stringstream ss;
        ss<<"waiting room:\n";
        for(const auto &pl:wait_room){
            ss<<pl->get_nick()<<":"<<pl->get_id()<<"\n";
        }
        for(const auto &r_ptr:rooms){
            const auto &r = *r_ptr;
            ss<<"\nRoom "<<r.get_id()<<"\n";
            for(const auto &pl:r){
                ss<<pl->get_nick()<<":"<<pl->get_id()<<"\n";
            }
        }
        std::cout<<ss.str()<<"\n";
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
                if(message->text.find("/start") != std::string::npos){
                    onTgStartMessage(message);
                }else if(message->text.find("/quit") != std::string::npos){
                    onTgQuitMessage(message);
                }else if(message->text.find("/roles") != std::string::npos){
                    onTgRolesMessage(message);
                }else if(message->text.find("/help") != std::string::npos){
                    onTgHelpMessage(message);
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
            writeTo(plr->get_id(), "There's no room with id "+id);
        }else{
            auto &room = *room_it;
            if(room->get_pass() == pass){
                auto plr_ = room->findPlayer(plr->get_id());
                if(plr_){
                    writeTo(plr_->get_id(),"You're allready in room '"+room->get_id()+"'");
                    return;
                }
                wait_room.removePlayer(plr);
                room->addPlayer(plr);
                auto mes = "Welcome to room '"+room->get_id()+"'.\
                    Now wait while admin will give you a role.";
                writeTo(plr->get_id(), mes);
                writeTo(room->adm->get_id(), "User @"+plr->get_nick()+" joined room");
            }else{
                writeTo(plr->get_id(),"Your pass '"+pass+"' is invalid");
            }
        }
    }
    void onRoomCreateRequested(std::shared_ptr<Player> plr, std::string pass){
        wait_room.removePlayer(plr);
        auto adm = std::make_shared<Admin>(plr->get_id(), plr->get_nick());
        auto room = std::make_unique<GameRoom>(adm, pass);
        room->id = gen.get_id();
        std::cout<<"new room, id:"<<room->get_id()
            <<" pass:"<<room->get_pass()<<"\n";
        std::stringstream ss;
        ss << "Your room id: " << room->get_id()
            << " pass:"+room->get_pass() << "\n\n"
            << "Now run command /set [role] [num] to change settings, otherwise all the players will be citizens.\n"
            << "Resend the message below to your friends:";
        writeTo(adm->get_id(), ss.str());
        writeTo(adm->get_id(), "/join " + room->get_id() +" "+room->get_pass());
        rooms.emplace_back(std::move(room));
    }
    template<class It>
    void onRoomEmpty(It room_it){
        auto &room = *room_it;
        rooms.erase(room_it);
        std::cout<<"Room id:"<<room->get_id()<<" is empty now\n";
    }
    void writeTo(int id, std::string mes){
        try{
            bot->getApi().sendMessage(id, mes);
        } catch (TgBot::TgException& e) {
            std::cout<<"ERROR: "<<e.what()<<std::endl;
        }
    }
};
