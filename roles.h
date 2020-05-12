#pragma once
#include <memory>

class Role{
    size_t id;
protected:
    Role(const decltype(id) &id)
        :id(id)
    {}
public:
    virtual ~Role(){}
    auto get_id()const{
        return id;
    }
    virtual std::string get_role_name()=0;
};

class Citizen:public Role{
public:
    Citizen():Role(0){};
    std::string get_role_name()override{ return "citizen"; }
};
class Mafia:public Role{
public:
    Mafia():Role(1){};
    std::string get_role_name()override{ return "mafia"; }
};
class Medic:public Role{
public:
    Medic():Role(2){};
    std::string get_role_name()override{ return "medic"; }
};
class lover:public Role{
public:
    lover():Role(3){};
    std::string get_role_name()override{ return "lover"; }
};
class Killer:public Role{
public:
    Killer():Role(4){};
    std::string get_role_name()override{ return "killer"; }
};
class Cop:public Role{
public:
    Cop():Role(5){};
    std::string get_role_name()override{ return "cop"; }
};
class Converter{
public:
    static std::unique_ptr<Role> get_role(const size_t &id){
        if(id == 0){
            return std::make_unique<Citizen>();
        }else if(id == 1){
            return std::make_unique<Mafia>();
        }else if(id == 2){
            return std::make_unique<Medic>();
        }else if(id == 3){
            return std::make_unique<lover>();
        }else if(id == 4){
            return std::make_unique<Killer>();
        }else if(id == 5){
            return std::make_unique<Cop>();
        }
        throw std::runtime_error("unknown id to convert");
    }
};