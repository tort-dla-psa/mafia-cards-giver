#pragma once
#include "players.h"
#include "roles.h"
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
    QVector<Player*> clients;
public:
    Room():QObject(){}
    ~Room(){
        for(auto &cl:clients){
            delete cl;
        }
        clients.clear();
    }
    void addPlayer(Player* pl){
        clients.push_back(pl);
        connect(pl, &Player::disconnected,
            this, &Room::onPlayerDisconnect);
    }

    //Note:removes from container in Room
    void removePlayer(Player* plr){
        clients.removeOne(plr);
    }
public slots:
    //abstract method
    virtual void onPlayerMessage(std::string mes)=0;
    virtual void onPlayerDisconnect(){
        auto pl = dynamic_cast<Player*>(sender());
        auto addr = pl->get_addr();
        auto addr_str = addr.toString().toStdString();
        std::cout<<"someone disconnected:"<<addr_str<<"\n";
        clients.removeOne(pl);
    }
signals:
    void empty();
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
            adm->write(ss.str());
        }
        auto sets_str = get_settings();
        std::cout<<sets_str;
        std::cout.flush();
        adm->write(sets_str);
    }

    void shuffle(){
        roles.clear();
        size_t citizen_count = clients.size()-1; //without admin
        auto check_mes = [this](const std::string &name, const size_t &count){
            if(count > clients.size()-1){
                std::stringstream ss;
                ss<<"Error: you set category \""<<name<<"\"to be this large:"<<count
                    <<"but there's only "<<clients.size()-1<<" players\n";
                std::cerr<<ss.str();
                adm->write(ss.str());
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
        for(int i=0; i<clients.size(); i++){
            auto plr = clients.at(i);
            auto adm = dynamic_cast<Admin*>(plr);
            if(adm){
                continue;
            }
            auto& role = *role_it;
            role_it++;
            plr->write(std::to_string(role->get_id()));
            std::cout<<"Room "<<get_id()
                <<": sent role "<<role->get_id()
                <<"to "<<plr->get_addr().toString().toStdString()<<"\n";
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
    void onPlayerMessage(std::string mes)override{
        auto plr = dynamic_cast<Player*>(sender());
        std::cout<<"GameRoom id "<<get_id()<<":"
            <<"player "<<plr->get_addr().toString().toStdString()
            <<" wrote this:"<<mes<<"\n";
        auto adm = dynamic_cast<Admin*>(sender());
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
                if(sum > clients.size()-1){
                    auto err_mes = "settings summ not equal to players count, "
                        +std::to_string(sum)+"!="+std::to_string(clients.size()-1);
                    std::cerr<<err_mes<<"\n";
                    adm->write(err_mes);
                }else{
                    shuffle();
                }
            }
        }
    }
    void onPlayerDisconnect()override{
        Room::onPlayerDisconnect();
        auto pl = (Player*)sender();
        auto adm_cast = dynamic_cast<Admin*>(pl);
        if(adm_cast){
            std::cout<<"admin disconnected!\n";
            for(auto &cl:clients){
                delete cl;
            }
            clients.clear();
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
public slots:
    void onPlayerMessage(std::string mes)override{
        auto plr = (Player*)sender();
        std::cout<<"Waiting room:"<<plr->get_addr().toString().toStdString()
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
    void onPlayerDisconnect()override{
        Room::onPlayerDisconnect();
        auto pl = dynamic_cast<Player*>(sender());
        clients.removeOne(pl);
        delete pl;
    }
signals:
    void roomCreateRequested(Player* plr, std::string pass);
    void roomJoinRequested(Player* plr, std::string id, std::string pass);
};