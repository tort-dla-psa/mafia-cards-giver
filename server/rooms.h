#pragma once
#include "players.h"
#include "roles.h"
#include "tg_wrapper.h"
#include <QObject>
#include <QVector>
#include <QString>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>

struct settings{
    size_t mafia_count=0,
        medic_count=0,
        cops_count=0,
        slut_count=0,
        killer_count=0;
};

class Room:public QObject{
    Q_OBJECT
protected:
    QVector<Player*> players;
public:
    Room():QObject(){}
    ~Room(){
        for(auto &cl:players){
            delete cl;
        }
        players.clear();
    }
    void addPlayer(Player* pl){
        players.push_back(pl);
    }
    //Note:removes from container in Room
    void removePlayer(Player* plr){
        players.removeOne(plr);
    }
    Player** findPlayer(int id){
        return std::find_if(players.begin(), players.end(), 
            [&id](const auto &pl){
                return pl->get_id() == id;
            });
    }
    Player** findPlayer(const std::string &nick){
        return std::find_if(players.begin(), players.end(), 
            [&nick](const auto &pl){
                return pl->get_nick() == nick;
            });
    }
    virtual void processMessage(TgWrapper::msg mes)=0;

public slots:
    virtual void onPlayerDisconnect(){
        //TODO:fix
        auto pl = nullptr;
        players.removeOne(pl);
    }
signals:
    void empty();
    void writeTo(int id, std::string mes);
};

class GameRoom:public Room{
    Q_OBJECT
    friend class Server;
    Admin* adm;
    std::string pass, id;
    const std::string set_sluts;
    const std::string set_cops;
    const std::string set_killers;
    const std::string set_mafias;
    const std::string set_medics;
    const std::string shuffle_cmd;

    std::vector<std::unique_ptr<Role>> roles;
    settings set;

    void check_settings_cmd(const QStringList &list){
        if(list.at(0) != "/set"){
            return;
        }
        auto cmd = list.at(1).toStdString();
        auto param = list.at(2).toUInt();
        if(cmd == set_cops){
            set.cops_count = param;
        }else if(cmd == set_sluts){
            set.slut_count = param;
        }else if(cmd == set_medics){
            set.medic_count = param;
        }else if(cmd == set_killers){
            set.killer_count = param;
        }else if(cmd == set_mafias){
            set.mafia_count = param;
        }else{
            std::stringstream ss;
            ss<<"unknown cmd:"<<cmd<<"\n";
            std::cerr<<ss.str();
            emit writeTo(adm->get_id(), ss.str());
        }
        auto sets_str = get_settings();
        std::cout<<sets_str;
        std::cout.flush();
        emit writeTo(adm->get_id(), sets_str);
    }

    void shuffle(){
        roles.clear();
        size_t citizen_count = players.size()-1; //without admin
        auto check_mes = [this](const std::string &name, const size_t &count){
            if(count > players.size()-1){
                std::stringstream ss;
                ss<<"Error: you set category \""<<name<<"\"to be this large:"<<count
                    <<"but there's only "<<players.size()-1<<" players\n";
                std::cerr<<ss.str();
                emit writeTo(adm->get_id(), ss.str());
            }
        };
        for(int i = 0; i<set.cops_count; i++){
            check_mes("sheriff", set.cops_count);
            roles.emplace_back(std::make_unique<Sheriff>());
            citizen_count--;
        }
        for(int i = 0; i<set.killer_count; i++){
            check_mes("killer", set.killer_count);
            roles.emplace_back(std::make_unique<Killer>());
            citizen_count--;
        }
        for(int i = 0; i<set.medic_count; i++){
            check_mes("medic", set.medic_count);
            roles.emplace_back(std::make_unique<Medic>());
            citizen_count--;
        }
        for(int i = 0; i<set.slut_count; i++){
            check_mes("slut", set.slut_count);
            roles.emplace_back(std::make_unique<Slut>());
            citizen_count--;
        }
        for(int i = 0; i<set.mafia_count; i++){
            check_mes("mafia", set.mafia_count);
            roles.emplace_back(std::make_unique<Mafia>());
            citizen_count--;
        }
        for(int i=0; i<citizen_count; i++){
            roles.emplace_back(std::make_unique<Citizen>());
        }
        std::random_shuffle(roles.begin(), roles.end());
        auto role_it = roles.begin();
        for(int i=0; i<players.size(); i++){
            auto plr = players.at(i);
            auto adm = dynamic_cast<Admin*>(plr);
            if(adm){
                continue;
            }
            auto& role = *role_it;
            role_it++;
            emit writeTo(plr->get_id(), std::to_string(role->get_id()));
            std::cout<<"Room "<<get_id()
                <<": sent role "<<role->get_role_name()
                <<"to "<<plr->get_nick()<<"\n";
        }
    }
public:
    GameRoom(Admin* adm,
        const std::string &pass = "",
        const std::string &set_sluts="sluts",
        const std::string &set_cops="sheriffs",
        const std::string &set_killers="killers",
        const std::string &set_mafias="mafias",
        const std::string &set_medics="medics",
        const std::string &shuffle_cmd="/shuffle")
        :Room(),
        set_sluts(set_sluts),
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

    void processMessage(TgWrapper::msg tg_mes)override{
        std::string mes = tg_mes->text;
        auto plr_it = findPlayer(tg_mes->chat->id);
        if(!plr_it){
            std::cerr<<"no user with id"<<tg_mes->chat->id<<" in room "+get_id()<<"\n";
            return;
        }
        auto plr = *plr_it;
        std::cout<<"GameRoom id "<<get_id()<<":"
            <<"player "<<plr->get_nick()
            <<" wrote this:"<<mes<<"\n";
        auto adm = dynamic_cast<Admin*>(plr);
        if(!adm){
            return;
        }else{
            QString str = QString::fromStdString(mes);
            auto list = str.split(' ');
            check_settings_cmd(list);
            if(list.at(0).toStdString() == shuffle_cmd){
                auto sum = set.cops_count+
                    set.killer_count+
                    set.mafia_count+
                    set.medic_count+
                    set.slut_count;
                if(sum > players.size()-1){
                    auto err_mes = "settings summ not equal to players count, "
                        +std::to_string(sum)+"!="+std::to_string(players.size()-1);
                    std::cerr<<err_mes<<"\n";
                    emit writeTo(adm->get_id(), err_mes);
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
        ss<<"settings::sluts = "<<set.slut_count<<"\n";
        ss<<"settings::mafias = "<<set.mafia_count<<"\n";
        ss<<"settings::killers = "<<set.killer_count<<"\n";
        return ss.str();
    }
public slots:
    void onPlayerDisconnect()override{
        Room::onPlayerDisconnect();
        auto pl = (Player*)sender();
        auto adm_cast = dynamic_cast<Admin*>(pl);
        if(adm_cast){
            std::cout<<"admin disconnected!\n";
            for(auto &cl:players){
                delete cl;
            }
            players.clear();
            emit empty();
        }
    }
};

class WaitingRoom:public Room{
    Q_OBJECT
    const std::string start_cmd, join_cmd;
public:
    WaitingRoom(const std::string start_cmd="/start",
        const std::string join_cmd="/join")
    :Room(),
        start_cmd(start_cmd),
        join_cmd(join_cmd)
    {}
    void processMessage(TgWrapper::msg tg_mes)override{
        auto plr_it = findPlayer(tg_mes->chat->id);
        if(!plr_it){
            std::cerr<<"no user with id"<<tg_mes->chat->id<<" in waiting room \n";
            return;
        }
        auto plr = *plr_it;
        std::string mes = tg_mes->text;
        std::cout<<"Waiting room:"<<plr->get_nick()
            <<" wrote this:"<<mes<<"\n";
        QString str = QString::fromStdString(mes);
        auto list = str.split(' ');
        auto cmd_str = list.at(0).toStdString();
        if(cmd_str == start_cmd){
            std::string pass;
            if(mes == start_cmd){
                pass = "";
            }else{
                pass = list.at(1).toStdString();
            }
            emit roomCreateRequested(plr, pass);
        }else if(cmd_str == join_cmd){
            // /join abcdef pass
            auto id = list.at(1).toStdString();
            auto pass = list.at(2).toStdString();
            emit roomJoinRequested(plr, id, pass);
        }
    }
public slots:
    void onPlayerDisconnect()override{
        Room::onPlayerDisconnect();
        auto pl = dynamic_cast<Player*>(sender());
        players.removeOne(pl);
        delete pl;
    }
signals:
    void roomCreateRequested(Player* plr, std::string pass);
    void roomJoinRequested(Player* plr, std::string id, std::string pass);
};