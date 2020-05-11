#pragma once
#include <QObject>
#include <tgbot/tgbot.h>
#include <memory>

class TgWrapper:public QObject{
    Q_OBJECT
    const std::string key;
    std::unique_ptr<TgBot::Bot> bot;
    TgBot::CurlHttpClient curl;
public:
    using msg = TgBot::Message::Ptr;
    TgWrapper(const std::string &key)
        :QObject(),
        key(key)
    {}
public slots:
    void run(){
        bot = std::make_unique<TgBot::Bot>(key, curl);
        bot->getEvents().onAnyMessage(
            [this](TgBot::Message::Ptr message) {
                if(message->text == "/start"){
                    emit gotStartMessage(message);
                }else{
                    emit gotMessage(message);
                }
        });
        try {
            TgBot::TgLongPoll longPoll(*bot);
            while (true) {
                longPoll.start();
            }
        } catch (TgBot::TgException& e) {
            std::cout<<"ERROR: "<<e.what()<<std::endl;
            emit gotError(e);
        }
    }
    void WriteTo(int id, std::string mes){
        try{
            bot->getApi().sendMessage(id, mes);
        } catch (TgBot::TgException& e) {
            std::cout<<"ERROR: "<<e.what()<<std::endl;
            emit gotError(e);
        }
    }
signals:
    void started();
    void gotStartMessage(msg);
    void gotMessage(msg);
    void gotError(TgBot::TgException);
};