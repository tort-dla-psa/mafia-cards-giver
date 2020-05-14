#pragma once
#include "players.h"
#include "roles.h"
#include <tgbot/tgbot.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "helpers.h"

struct settings{
    size_t mafia_count=0,
        medic_count=0,
        cops_count=0,
        lover_count=0,
        killer_count=0;
};

class Room{
public:
    struct request{
        virtual ~request(){}
    };
    struct write_request:request{
        int id;
        std::string mes;
        write_request(int id, std::string mes):id(id),mes(mes),request(){}
    };
    struct join_request:request{
        std::string id;
        std::string pass;
        join_request(std::string id, std::string pass):id(id),pass(pass),request(){}
    };
    struct create_request:request{
        std::string pass;
        create_request(std::string pass):pass(pass),request(){}
    };
protected:
    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::shared_ptr<request>> reqs;
    void write_to(int id, std::string mes){
        std::shared_ptr<request> req = std::make_shared<write_request>(id, mes);
        reqs.emplace_back(req);
    }
public:
    Room(){}
    ~Room(){
        players.clear();
    }
    auto begin(){
        return players.begin();
    }
    auto end(){
        return players.end();
    }
    auto get_requests(){
        return std::move(reqs);
    }
    void addPlayer(std::shared_ptr<Player> pl){
        players.emplace_back(pl);
    }

    auto removePlayer(decltype(players)::iterator it){
        return players.erase(it);
    }
    void removePlayer(std::shared_ptr<Player> plr){
        if(!plr){
            return;
        }
        auto it = std::find(players.begin(), players.end(), plr);
        players.erase(it);
    }
    std::shared_ptr<Player> findPlayer(int id){
        auto it = std::find_if(players.begin(), players.end(), 
            [&id](const auto &pl){
                return pl->get_id() == id;
            });
        if(it == players.end()){
            return nullptr;
        }
        return *it;
    }
    std::shared_ptr<Player> findPlayer(const std::string &nick){
        auto it = std::find_if(players.begin(), players.end(), 
            [&nick](const auto &pl){
                return pl->get_nick() == nick;
            });
        if(it == players.end()){
            return nullptr;
        }
        return *it;
    }
    bool empty()const{
        return size()==0;
    }
    size_t size()const{
        return players.size();
    }
    virtual void processMessage(TgBot::Message::Ptr mes)=0;
};

class GameRoom:public Room{
    friend class Server;
    std::shared_ptr<Admin> adm;
    std::string pass, id;
    const std::string set_lovers;
    const std::string set_cops;
    const std::string set_killers;
    const std::string set_mafias;
    const std::string set_medics;
    const std::string shuffle_cmd;

    std::vector<std::unique_ptr<Role>> roles;
    settings set;

    void check_settings_cmd(const std::vector<std::string> &list){
        if(list.at(0) != "/set"){
            return;
        }
        if(list.size()!=3){
            write_to(adm->get_id(), "Usage: /set [role] [num]");
            return;
        }
        auto cmd = list.at(1);
        auto param = std::stoi(list.at(2));
        if(cmd == set_cops){
            set.cops_count = param;
        }else if(cmd == set_lovers){
            set.lover_count = param;
        }else if(cmd == set_medics){
            set.medic_count = param;
        }else if(cmd == set_killers){
            set.killer_count = param;
        }else if(cmd == set_mafias){
            set.mafia_count = param;
        }else{
            std::stringstream ss;
            ss<<"Unknown cmd:"<<cmd;
            std::cerr<<ss.str()<<"\n";
            write_to(adm->get_id(), ss.str());
            return;
        }
        auto sets_str = get_settings();
        std::cout<<sets_str;
        std::cout.flush();
        write_to(adm->get_id(), sets_str);
    }

    void shuffle(){
        roles.clear();
        size_t citizen_count = players.size()-1; //without admin
        auto check_mes = [this](const std::string &name, const size_t &count)->bool{
            if(count > players.size()-1){
                std::stringstream ss;
                ss<<"Error: you set category \""<<name<<"\"to be this large:"<<count
                    <<"but there're only "<<players.size()-1<<" players";
                std::cerr<<ss.str()<<"\n";;
                write_to(adm->get_id(), ss.str());
                return false;
            }
            return true;
        };
        for(int i = 0; i<set.cops_count; i++){
            if(check_mes("cop", set.cops_count)){
                return;
            }
            roles.emplace_back(std::make_unique<Cop>());
            citizen_count--;
        }
        for(int i = 0; i<set.killer_count; i++){
            if(check_mes("killer", set.killer_count)){
                return;
            }
            roles.emplace_back(std::make_unique<Killer>());
            citizen_count--;
        }
        for(int i = 0; i<set.medic_count; i++){
            if(check_mes("medic", set.medic_count)){
                return;
            }
            roles.emplace_back(std::make_unique<Medic>());
            citizen_count--;
        }
        for(int i = 0; i<set.lover_count; i++){
            if(check_mes("lover", set.lover_count)){
                return;
            }
            roles.emplace_back(std::make_unique<lover>());
            citizen_count--;
        }
        for(int i = 0; i<set.mafia_count; i++){
            if(check_mes("mafia", set.mafia_count)){
                return;
            }
            roles.emplace_back(std::make_unique<Mafia>());
            citizen_count--;
        }
        for(int i=0; i<citizen_count; i++){
            roles.emplace_back(std::make_unique<Citizen>());
        }
        std::random_shuffle(roles.begin(), roles.end());
        auto role_it = roles.begin();
        std::stringstream roles_for_adm;
        for(int i=0; i<players.size(); i++){
            auto plr = players.at(i);
            auto adm = std::dynamic_pointer_cast<Admin>(plr);
            if(adm){
                continue;
            }
            auto& role = *role_it;
            role_it++;
            auto mes = "your role is '"+role->get_role_name()+"'. good luck with it!";
            write_to(plr->get_id(), mes);
            roles_for_adm<<"@"<<plr->get_nick()<<" has role '"+role->get_role_name()+"'\n";
            std::cout<<"Room "<<get_id()
                <<": sent role "<<role->get_role_name()
                <<"to "<<plr->get_nick()<<"\n";
        }
        std::cout<<roles_for_adm.str();
        write_to(GameRoom::adm->get_id(), roles_for_adm.str());
    }
public:
    GameRoom(std::shared_ptr<Admin> adm,
        const std::string &pass = "",
        const std::string &set_lovers="lovers",
        const std::string &set_cops="cops",
        const std::string &set_killers="killers",
        const std::string &set_mafias="mafias",
        const std::string &set_medics="medics",
        const std::string &shuffle_cmd="/shuffle")
        :Room(),
        set_lovers(set_lovers),
        set_cops(set_cops),
        set_killers(set_killers),
        set_mafias(set_mafias),
        set_medics(set_medics),
        shuffle_cmd(shuffle_cmd)
    {
        this->adm = adm;
        Room::addPlayer(adm);
        this->pass = pass;
    }

    std::shared_ptr<Admin> getAdmin()const{
        return adm;
    }

    void processMessage(TgBot::Message::Ptr tg_mes)override{
        std::string mes = tg_mes->text;
        auto plr = findPlayer(tg_mes->chat->id);
        if(!plr){
            std::cerr<<"no user with id"<<tg_mes->chat->id<<" in room "+get_id()<<"\n";
            return;
        }
        auto adm = std::dynamic_pointer_cast<Admin>(plr);
        std::cout<<"GameRoom id "<<get_id()<<":";
        if(!adm){
            std::cout<<"player "<<plr->get_nick()
            <<" wrote this:"<<mes<<"\n";
            return;
        }else{
            std::cout<<"admin "<<plr->get_nick()
            <<" wrote this:"<<mes<<"\n";
            auto list = helpers::split(mes);
            check_settings_cmd(list);
            if(list.at(0) == shuffle_cmd){
                auto sum = set.cops_count+
                    set.killer_count+
                    set.mafia_count+
                    set.medic_count+
                    set.lover_count;
                if(sum > players.size()-1){
                    auto err_mes = "settings sum is not equal to players count, "
                        +std::to_string(sum)+"!="+std::to_string(players.size()-1);
                    std::cerr<<err_mes<<"\n";
                    write_to(adm->get_id(), err_mes);
                }else{
                    shuffle();
                }
            }
        }
    }
    const std::string& get_pass()const{
        return pass;
    }
    const std::string& get_id()const{
        return id;
    }
    std::string get_settings()const{
        std::stringstream ss;
        ss<<"settings::cops = "<<set.cops_count<<"\n";
        ss<<"settings::medics = "<<set.medic_count<<"\n";
        ss<<"settings::lovers = "<<set.lover_count<<"\n";
        ss<<"settings::mafias = "<<set.mafia_count<<"\n";
        ss<<"settings::killers = "<<set.killer_count<<"\n";
        return ss.str();
    }
};

class WaitingRoom:public Room{
    const std::string start_cmd, join_cmd;
public:
    WaitingRoom(const std::string start_cmd="/create",
        const std::string join_cmd="/join")
    :Room(),
        start_cmd(start_cmd),
        join_cmd(join_cmd)
    {}
    void processMessage(TgBot::Message::Ptr tg_mes)override{
        auto plr = findPlayer(tg_mes->chat->id);
        if(!plr){
            std::cerr<<"no user with id"<<tg_mes->chat->id<<" in waiting room \n";
            return;
        }
        std::string mes = tg_mes->text;
        std::cout<<"Waiting room:"<<plr->get_nick()
            <<" wrote this:"<<mes<<"\n";
        auto list = helpers::split(mes);
        if(list.size()==0){
            return;
        }
        auto cmd_str = list.at(0);
        if(cmd_str == start_cmd){
            std::string pass = "";
            if(list.size() == 2){
                pass = list.at(1);
            }else if(list.size() != 1){
                write_to(tg_mes->chat->id, "Usage: /create [pass(optional)]");
                return;
            }
            std::shared_ptr<request> req = std::make_shared<create_request>(pass);
            reqs.emplace_back(req);
        }else if(cmd_str == join_cmd){
            std::string pass = "";
            if(list.size() == 3){
                pass = list.at(2);
            }else if(list.size() > 3 || list.size() < 2){
                write_to(tg_mes->chat->id, "Usage: /join [id] [pass(optional)]");
                return;
            }
            auto id = list.at(1);
            std::shared_ptr<request> req = std::make_shared<join_request>(id, pass);
            reqs.emplace_back(req);
        }
    }
};
