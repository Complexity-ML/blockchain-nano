#include "json.hpp"
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
using Json=nlohmann::json;
void archive_require(bool ok){if(!ok)throw std::invalid_argument("Invalid Bellatrix durable archive");}
std::string candidate(Json document,const std::string& encoded,size_t maximum,const std::function<void()>& cancel={}){
 struct {size_t envelope_bytes;} limits{maximum};
 auto result=document.dump();
 constexpr std::string_view store_key=",\"store\":";
 archive_require(encoded.size()<=limits.envelope_bytes&&result.size()<=limits.envelope_bytes-encoded.size()&&store_key.size()<=limits.envelope_bytes-encoded.size()-result.size());
 result.reserve(result.size()+store_key.size()+encoded.size());
 result.pop_back();result.append(store_key);result.append(encoded);result.push_back('}');
 if(cancel)cancel();return result;
}
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
  if(candidate(document,encoded,expected.size())!=expected)throw std::runtime_error("canonical byte mismatch");++comparisons;
  if(candidate(document,encoded,SIZE_MAX)!=expected)throw std::runtime_error("unbounded byte mismatch");++comparisons;
  for(size_t bound:{size_t(0),encoded.size()-1,expected.size()-1}){bool rejected=false;try{candidate(document,encoded,bound);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("envelope bound accepted");++rejections;}
  bool cancelled=false;try{candidate(document,encoded,expected.size(),[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error&){cancelled=true;}if(!cancelled)throw std::runtime_error("cancellation swallowed");++cancellations;
 }
 std::cout<<Json{{"versions",Json::array({2,3,4,5})},{"byteComparisons",comparisons},{"boundRejections",rejections},{"cancellations",cancellations},{"passed",true}}.dump()<<std::endl;
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
