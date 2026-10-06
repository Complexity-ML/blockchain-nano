#include "nano/block_import.hpp"
#include <openssl/rand.h>
#include <set>
namespace nano {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::invalid_argument(message);}
bool canonical_hash(const Json& value){if(!value.is_string())return false;const auto& text=value.get_ref<const std::string&>();if(text.size()!=66)return false;try{return hex(unhex(text))==text&&unhex(text).size()==32;}catch(const std::exception&){return false;}}
}
void BlockImports::erase(std::map<std::string,Assembly>::iterator it){used_-=it->second.charged;assemblies_.erase(it);}
void BlockImports::expire(Clock::time_point now){for(auto it=assemblies_.begin();it!=assemblies_.end();){if(now>=it->second.expires){auto old=it++;erase(old);}else ++it;}}
Json BlockImports::progress(const std::string& id,const Assembly& value)const{return {{"id",id},{"transaction",value.completed.size()},{"offset",quantity(Int(value.current.size()))},{"complete",value.completed.size()==value.block.at("transactions").size()}};}
Json BlockImports::begin(const Json& manifest,Clock::time_point now){return begin_impl(manifest,now,nullptr);}
Json BlockImports::begin_proposal(const Json& envelope,Clock::time_point now){
 auto metadata=envelope;Json manifest={{"version",1},{"block",metadata.at("proposal")}};metadata["proposal"]=nullptr;return begin_impl(manifest,now,std::move(metadata));
}
Json BlockImports::begin_impl(const Json& manifest,Clock::time_point now,Json envelope){
 expire(now);require(manifest.is_object()&&manifest.size()==2&&manifest.at("version").is_number_integer()&&manifest.at("version")==1&&manifest.at("block").is_object(),"invalid block import manifest");
 const auto& block=manifest.at("block");require(block.at("transactionEncoding")=="signed-rlp-hashes-v1"&&block.at("transactions").is_array()&&canonical_hash(block.at("hash")),"invalid block import identity");
 const auto size=manifest.dump().size()+(envelope.is_null()?0:envelope.dump().size());if(size>8*1024*1024||size>limits_.bytes-used_||assemblies_.size()>=limits_.sessions)throw BlockImportLimit("Block import capacity exceeded");
 std::set<std::string> hashes;for(const auto& hash:block.at("transactions")){require(canonical_hash(hash),"invalid import transaction hash");require(hashes.insert(hash.get<std::string>()).second,"duplicate import transaction hash");}
 Bytes random(16);require(RAND_bytes(random.data(),static_cast<int>(random.size()))==1,"block import randomness failed");auto id=hex(random);
 Assembly value;value.block=block;value.envelope=std::move(envelope);value.charged=size;value.expires=now+limits_.lifetime;
 auto [it,inserted]=assemblies_.emplace(id,std::move(value));require(inserted,"block import identifier collision");used_+=size;return progress(id,it->second);
}
Json BlockImports::append(const std::string& id,size_t index,const Json& fragment,Clock::time_point now){
 expire(now);auto it=assemblies_.find(id);require(it!=assemblies_.end(),"unknown or expired block import");auto& value=it->second;
 require(fragment.is_object()&&fragment.size()==7&&fragment.at("version").is_number_integer()&&fragment.at("version")==1,"invalid transaction fragment");
 require(index<value.block.at("transactions").size()&&index<=value.completed.size(),"out-of-order import transaction");
 require(fragment.at("blockHash")==value.block.at("hash")&&fragment.at("transactionHash")==value.block.at("transactions").at(index),"fragment belongs to another block or transaction");
 require(fragment.at("data").is_string()&&fragment.at("data").get_ref<const std::string&>().size()<=2*256*1024+2,"fragment byte limit exceeded");
 auto data=unhex(fragment.at("data").get<std::string>());require(!data.empty()&&data.size()<=256*1024&&hex(data)==fragment.at("data").get<std::string>(),"invalid canonical fragment bytes");
 const auto offset=u64(quantity(fragment.at("offset"))),total=u64(quantity(fragment.at("total")));
 require(total>0&&offset% (256*1024)==0&&offset<total&&data.size()==std::min<uint64_t>(256*1024,total-offset),"invalid fragment range");
 const auto end=offset+data.size();require(fragment.at("next")== (end==total?Json(nullptr):Json(quantity(Int(std::to_string(end))))),"invalid next fragment offset");
 const auto& current=index<value.completed.size()?value.completed[index]:value.current;
 if(index<value.completed.size()||offset<current.size()){
  require(total==(index<value.completed.size()?current.size():value.total)&&offset<=current.size()&&data.size()<=current.size()-offset&&std::equal(data.begin(),data.end(),current.begin()+offset),"conflicting duplicate fragment");return progress(id,value);
 }
 require(offset==value.current.size()&&(!value.total||value.total==total),"fragment gap or changed total");
 if(total>limits_.bytes||data.size()>limits_.bytes-used_)throw BlockImportLimit("Block import byte capacity exceeded");
 value.current.insert(value.current.end(),data.begin(),data.end());value.total=total;value.charged+=data.size();used_+=data.size();
 if(end==total){
  if(hex(keccak(value.current))!=fragment.at("transactionHash").get<std::string>()){erase(it);throw std::invalid_argument("reconstructed transaction hash mismatch; import discarded");}
  value.completed.push_back(std::move(value.current));value.current=Bytes{};value.total=0;
 }
 return progress(id,value);
}
Json BlockImports::finish(const std::string& id,Clock::time_point now){return finish_impl(id,now,false);}
Json BlockImports::finish_proposal(const std::string& id,Clock::time_point now){return finish_impl(id,now,true);}
Json BlockImports::finish_impl(const std::string& id,Clock::time_point now,bool proposal){
 expire(now);auto it=assemblies_.find(id);require(it!=assemblies_.end(),"unknown or expired block import");auto& value=it->second;
 require(proposal==!value.envelope.is_null(),"import purpose mismatch");
 require(value.current.empty()&&value.completed.size()==value.block.at("transactions").size(),"block import is incomplete");
 Json block=value.block;block["transactionEncoding"]="signed-rlp-v1";block["transactions"]=Json::array();for(const auto& raw:value.completed)block["transactions"].push_back(hex(raw));
 Json result=std::move(block);if(proposal){auto envelope=value.envelope;envelope["proposal"]=std::move(result);result=std::move(envelope);}
 erase(it);return result;
}
bool BlockImports::abort(const std::string& id,Clock::time_point now){expire(now);auto it=assemblies_.find(id);if(it==assemblies_.end())return false;erase(it);return true;}
}
