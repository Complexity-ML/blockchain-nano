#include "consensus_checkpoint_json.hpp"
#include "json.hpp"
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
using Json=nlohmann::json;
using nano::checkpoint_detail::durable_json;
int main(){try{
 std::mt19937_64 random(0x42454c4c41545249);size_t comparisons=0,rejections=0,cancellations=0;
 for(int version=2;version<=5;++version)for(unsigned sample=0;sample<256;++sample){
  Json blocks=Json::array();for(unsigned i=0;i<sample%9;++i)blocks.push_back(Json::array({std::to_string(random()),std::string(121*(1+sample%8),'a')}));
  Json store={{"format","nano-bellatrix-store-v1"},{"clock",Json::array({"0",std::to_string(random())})},{"blocks",blocks},{"unicode","é 🐝"},{"escape","\"\\\n"},{"nested",Json{{"z",UINT64_MAX},{"a",Json::array({nullptr,true,false,-17,1.25})}}}};
  Json document={{"format","nano-bellatrix-durable-v"+std::to_string(version)},{"signedBlocks",blocks}};
  if(version>=3)document["signedVotes"]=blocks;
  if(version>=4)document["signedSlashings"]=blocks;
  if(version>=5)document["signedExits"]=blocks;
  const auto encoded=store.dump();auto legacy=document;legacy["store"]=Json::parse(encoded);const auto expected=legacy.dump();
  if(durable_json(document,encoded,expected.size())!=expected)throw std::runtime_error("canonical byte mismatch");++comparisons;
  if(durable_json(document,encoded,SIZE_MAX)!=expected)throw std::runtime_error("unbounded byte mismatch");++comparisons;
  for(size_t bound:{size_t(0),encoded.size()-1,expected.size()-1}){bool rejected=false;try{durable_json(document,encoded,bound);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("envelope bound accepted");++rejections;}
  bool cancelled=false;try{durable_json(document,encoded,expected.size(),[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error&){cancelled=true;}if(!cancelled)throw std::runtime_error("cancellation swallowed");++cancellations;
 }
 for(const auto& document:{Json::array(),Json::object(),Json{{"format","v3"},{"store",Json::object()}},Json{{"format","v3"},{"zzz",true}}}){bool rejected=false;try{durable_json(document,"{}",SIZE_MAX);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("noncanonical outer keys accepted");++rejections;}
 std::cout<<Json{{"versions",Json::array({2,3,4,5})},{"byteComparisons",comparisons},{"boundRejections",rejections},{"cancellations",cancellations},{"passed",true}}.dump()<<std::endl;
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
