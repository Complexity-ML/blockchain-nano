#include "bft_vote_knowledge.hpp"
#include <iostream>
#include <stdexcept>
using Json=nlohmann::json;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 using Clock=BftVoteKnowledge::Clock;auto now=Clock::time_point{}+std::chrono::seconds(1);BftVoteKnowledge knowledge;
 Json page={{"version",1},{"height","0x1"},{"round","0x0"},{"proposal",{{"hash","proposal-preserved"}}},{"votes",Json::array({"signed-a","signed-b"})},{"previous",nullptr}};
 check(knowledge.filter(page,now)==page,"unknown peer gets complete page");check(knowledge.filter(page,now)==page,"sending alone must not acknowledge votes");
 auto remote=page;remote["votes"]=Json::array({"signed-a"});knowledge.observe(remote);auto delta=knowledge.filter(page,now);check(delta["votes"]==Json::array({"signed-b"})&&delta["proposal"]==page["proposal"],"only exact observed vote omitted, proposal preserved");
 remote["votes"]=Json::array({"forged-a"});knowledge.observe(remote);check(knowledge.filter(page,now)==delta,"different bytes cannot suppress valid vote");
 knowledge.observe(page);check(knowledge.filter(page,now)["votes"].empty(),"peer already sent both exact votes");check(knowledge.filter(page,now+std::chrono::seconds(5))==page,"periodic full resend even if peer loses volatile state");
 auto next=page;next["round"]="0x1";check(knowledge.filter(next,now)==next,"new round must not inherit old acknowledgments");knowledge.observe(next);knowledge.filter(next,now);auto later=next;later["round"]="0x2";knowledge.observe(later);check(knowledge.filter(page,now)==page,"evicted old context safely resends");
 auto old=std::make_shared<BftVoteKnowledge>();old->observe(page);old->filter(page,now);auto fresh=std::make_shared<BftVoteKnowledge>();old->observe(page);check(fresh->filter(page,now)==page,"callbacks retaining old session cannot populate replacement state");
 BftVoteKnowledge ack;auto sent=ack.filter(page,now);auto receipt=ack.receipt(sent);check(!ack.acknowledge(receipt,nullptr)&&ack.filter(page,now)==page,"missing reply cannot acknowledge request");check(!ack.acknowledge(receipt,next),"different round reply cannot acknowledge request");auto reply=page;reply["votes"]=Json::array();check(ack.acknowledge(receipt,reply)&&ack.filter(page,now)["votes"].empty(),"successful same-context response acknowledges request without echoing all votes");check(ack.filter(page,now+std::chrono::seconds(5))==page,"acknowledgments still expire into full refresh");
 BftVoteKnowledge bounded;auto large=page;large["votes"]=Json::array();for(unsigned i=0;i<2000;++i)large["votes"].push_back(std::string(550,'x')+std::to_string(i));for(unsigned round=0;round<20;++round){large["round"]="0x"+std::string(1,"0123456789abcdef"[round%16]);bounded.observe(large);check(bounded.retained()<=4000,"bounded two-context fingerprint storage");}
 BftVoteKnowledge invalid;auto bad=large;bad["votes"].push_back("overflow");invalid.observe(bad);check(invalid.retained()==0,"oversized input not retained");bad=page;bad["votes"]=Json::array({std::string(2049,'x')});invalid.observe(bad);check(invalid.retained()==0,"oversized vote not retained");
 BftVoteKnowledge scale;scale.observe(large);auto full=scale.filter(large,now);auto reduced=scale.filter(large,now);check(reduced["votes"].empty()&&full["votes"].size()==2000,"all known votes suppressed between refreshes");
 std::cout<<Json{{"scenario","bft-vote-knowledge"},{"verified",true},{"fullBytes",full.dump().size()},{"deltaBytes",reduced.dump().size()},{"maxFingerprintsPerPeer",4000},{"fullRefreshSeconds",5},{"consensusValidationBypassed",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
