#include "timed_reply.hpp"
#include <iostream>
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){try{
 asio::io_context io;unsigned calls=0,cancelled=0;Json result;
 auto normal=std::make_shared<TimedReply>(io,[&](Json value){++calls;result=value;});normal->start(std::chrono::milliseconds(1));normal->on_cancel([&]{++cancelled;});normal->finish(Json{{"ok",true}});normal->finish(nullptr);io.run();
 check(calls==1&&cancelled==0&&result.at("ok")==true&&!normal->active(),"completion cancels timer and ignores duplicate callback");
 io.restart();calls=0;auto timeout=std::make_shared<TimedReply>(io,[&](Json value){++calls;result=value;});timeout->start(std::chrono::milliseconds(1));timeout->on_cancel([&]{++cancelled;});io.run();timeout->finish(Json{{"late",true}});
 check(calls==1&&cancelled==1&&result.is_null(),"timeout cancels stream and late response cannot complete twice");
 check(!timeout->on_cancel([&]{++cancelled;})&&cancelled==2,"stream established after timeout is reset immediately");
 io.restart();calls=0;auto opening=std::make_shared<TimedReply>(io,[&](Json value){++calls;result=value;});opening->start(std::chrono::milliseconds(1));io.run();
 check(calls==1&&result.is_null()&&!opening->active(),"deadline applies even before a stream exists");
 std::cout<<"Exchange deadlines and exactly-once completion passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
