#include <iostream>
#include "server.h"
#include "tgbot/tgbot.h"

int main(int argc, char* argv[]){
    std::string key;
    if(argc != 2){
        std::cerr<<"ERROR:provide api key\n";
        return 1;
    }else{
        key = std::string(argv[1]);
    }
    Server s(key);
    s.run();
    return 0;
}
