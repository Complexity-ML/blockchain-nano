#include "gossip_progress.hpp"
#include <iostream>
#include <stdexcept>
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 GossipProgress p;p.cursor=7;
 Json ack={{"cursor",12},{"processed",2},{"accepted",Json::array({true,false})}};
 check(!p.acknowledge(nullptr,12,2)&&p.cursor==7&&p.failed_cursor==12,"lost reply preserves replay position");
 for(const auto& bad:{Json(),Json::array(),Json{{"cursor",11},{"processed",2},{"accepted",Json::array({true,true})}},Json{{"cursor",12},{"processed",1},{"accepted",Json::array({true,true})}},Json{{"cursor",12},{"processed",2},{"accepted",Json::array({true})}},Json{{"cursor",12},{"processed",2},{"accepted",Json::array({true,1})}}})check(!p.acknowledge(bad,12,2)&&p.cursor==7,"malformed or unrelated acknowledgement cannot advance cursor");
 check(p.acknowledge(ack,12,2)&&p.cursor==12&&!p.failed_cursor,"valid treatment acknowledgement completes retry");
 check(p.acknowledge(ack,12,2)&&p.cursor==12,"duplicate acknowledgement is idempotent");
 check(!p.acknowledge(Json{{"cursor",8},{"processed",0},{"accepted",Json::array()}},8,0)&&p.cursor==12,"stale acknowledgement cannot rewind cursor");
 check(p.acknowledge(Json{{"cursor",0},{"processed",0},{"accepted",Json::array()}},0,0,true)&&p.cursor==0,"explicit inventory restart accepts a new session cursor");
 auto current=std::make_shared<GossipProgress>();current->cursor=99;current->epoch="local";current->after="page";current->snapshot=true;current->busy=true;
 Json before={{"gossip_epoch","0x"+std::string(32,'a')}},after={{"gossip_epoch","0x"+std::string(32,'b')}};
 auto stale=current;observe_gossip_session(current,before,before);check(current==stale,"unchanged remote session preserves progress");
 observe_gossip_session(current,before,after);check(current!=stale&&current->cursor==0&&current->epoch.empty()&&current->after.empty()&&!current->snapshot&&!current->busy,"remote core restart resets complete inventory state");
 check(stale->acknowledge(Json{{"cursor",100},{"processed",0},{"accepted",Json::array()}},100,0)&&current->cursor==0,"late previous-session callback cannot advance replacement state");
 auto fresh=current;observe_gossip_session(current,after,after);check(current==fresh,"repeated status does not restart inventory");
 observe_gossip_session(current,after,Json::object());check(current!=fresh,"loss of session field resets once for legacy peer");fresh=current;observe_gossip_session(current,Json::object(),Json::object());check(current==fresh,"legacy status does not reset continuously");
 GossipProgress retry;auto now=GossipProgress::Clock::time_point{};retry.inventory_due=now+std::chrono::seconds(60);retry.cursor=42;retry.epoch="local";
 check(!retry.refresh_inventory(now+std::chrono::seconds(59))&&retry.cursor==42,"no premature full inventory");
 check(retry.refresh_inventory(now+std::chrono::seconds(60))&&retry.cursor==0&&retry.epoch.empty()&&retry.after.empty()&&retry.snapshot,"periodic inventory revisits processed refusals");
 retry.after="next-page";check(!retry.refresh_inventory(now+std::chrono::seconds(600))&&retry.after=="next-page","long inventory is not restarted by timer");
 retry.snapshot=false;retry.inventory_completed(now+std::chrono::seconds(600));check(!retry.refresh_inventory(now+std::chrono::seconds(659)),"interval starts after final inventory page");
 retry.busy=true;check(!retry.refresh_inventory(now+std::chrono::seconds(660)),"in-flight work preserved");retry.busy=false;retry.failed_cursor=45;check(!retry.refresh_inventory(now+std::chrono::seconds(660)),"failed batch retried before scheduled inventory");retry.failed_cursor.reset();check(retry.refresh_inventory(now+std::chrono::seconds(660)),"inventory resumes when safe");
 std::cout<<"Lost, malformed, stale and duplicate gossip acknowledgements passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
