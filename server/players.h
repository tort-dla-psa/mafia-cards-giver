#pragma once
#include <iostream>

class Player{
protected:
friend class Admin;
    const int id;
    std::string username;
public:
    Player(int id, const std::string &username)
        :id(id),
        username(username)
    {}
    virtual ~Player(){}
    auto get_id()const{
        return this->id;
    }
    auto get_nick()const{
        return this->username;
    }
};

class Admin:public Player{
public:
    Admin(const int id, const std::string &username)
        :Player(id, username)
    {}

    //constructor that moves and secures data from player
    Admin(Player*& plr)
        :Player(plr->id, plr->username)
    {}
};