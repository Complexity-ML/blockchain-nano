#include "json.hpp"
#include <optional>
#include <random>
#include <iostream>
using Json=nlohmann::json;
struct View {std::string anchor;Json justified,finalized;std::optional<std::string> proposer_boost;};
struct Clock {uint64_t genesis_time_ms,time_ms;Json unrealized_justified,unrealized_finalized;};
std::string root(const std::string& value){return value;}
Json cp(const Json& value){return value;}
std::string num(uint64_t value){return std::to_string(value);}
Json candidate(Json ancestors,Json blocks,Json messages,Json equiv,Json timely,Json unrealized,const Json& current_profile,const View& v,const Clock& c){ Json j{{"ancestors",std::move(ancestors)},{"format","nano-bellatrix-store-v1"},{"profile",current_profile},{"anchor",root(v.anchor)},{"justified",cp(v.justified)},{"finalized",cp(v.finalized)},
 {"clock",Json::array({num(c.genesis_time_ms),num(c.time_ms),cp(c.unrealized_justified),cp(c.unrealized_finalized)})},
 {"boost",v.proposer_boost?Json(root(*v.proposer_boost)):Json(nullptr)},{"blocks",std::move(blocks)},{"messages",std::move(messages)},{"equivocations",std::move(equiv)},{"timely",std::move(timely)},{"unrealized",std::move(unrealized)}};
return j;}
Json reference(Json ancestors,Json blocks,Json messages,Json equiv,Json timely,Json unrealized,const Json& current_profile,const View& v,const Clock& c){ Json j{{"ancestors",ancestors},{"format","nano-bellatrix-store-v1"},{"profile",current_profile},{"anchor",root(v.anchor)},{"justified",cp(v.justified)},{"finalized",cp(v.finalized)},
 {"clock",Json::array({num(c.genesis_time_ms),num(c.time_ms),cp(c.unrealized_justified),cp(c.unrealized_finalized)})},
 {"boost",v.proposer_boost?Json(root(*v.proposer_boost)):Json(nullptr)},{"blocks",blocks},{"messages",messages},{"equivocations",equiv},{"timely",timely},{"unrealized",unrealized}};
return j;}

int main(){try{std::mt19937_64 random(0x4d4f564553544f52);size_t comparisons=0;
 for(unsigned n=0;n<512;++n){
  Json rows=Json::array();for(unsigned i=0;i<n%17;++i)rows.push_back(Json::array({std::to_string(random()),std::string((n%32+1)*64,'f'),Json{{"unicode","🐝 é"},{"escaped","\"\\\n"}}}));
  Json profile={{"fork","bellatrix"},{"random",random()}};View v{"0xabc",Json::array({random(),"0xj"}),Json::array({random(),"0xf"}),n%2?std::optional<std::string>{"0xb"}:std::nullopt};Clock c{random(),random(),v.justified,v.finalized};
  auto expected=reference(rows,rows,rows,rows,rows,rows,profile,v,c).dump();auto actual=candidate(rows,rows,rows,rows,rows,rows,profile,v,c).dump();if(expected!=actual)throw std::runtime_error("snapshot move encoding mismatch");++comparisons;
 }
 std::cout<<Json{{"byteComparisons",comparisons},{"passed",true},{"scope","exact final JSON expression; synthetic values"}}.dump()<<std::endl;return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
