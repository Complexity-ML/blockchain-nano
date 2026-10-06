#include "nano/core.hpp"
namespace nano {
Bytes Account::storage_root() const {std::map<Bytes,Bytes> entries;for(const auto& [key,value]:storage)if(value!=0)entries[keccak(bytes(key,32))]=Rlp(value).encode();return trie_root(entries);}
Bytes Account::encode() const {return Rlp(std::vector<Rlp>{Rlp(nonce),Rlp(balance),Rlp(storage_root()),Rlp(keccak(code))}).encode();}
Json Account::to_json() const {Json slots=Json::object();for(const auto& [key,val]:storage)slots[quantity(key)]=quantity(val);return {{"nonce",quantity(nonce)},{"balance",quantity(balance)},{"storageRoot",hex(storage_root())},{"codeHash",hex(keccak(code))},{"code",hex(code)},{"storage",slots}};}
const Account& State::get(const std::string& addr) const {static const Account empty;auto it=accounts_.find(address(addr));return it==accounts_.end()?empty:it->second;}
Account& State::get(const std::string& addr){return accounts_[address(addr)];}
bool State::exists(const std::string& addr)const{return !get(addr).empty();}
void State::erase(const std::string& addr){accounts_.erase(address(addr));}
bool State::transfer(const std::string& from,const std::string& to,const Int& amount){if(amount<0)throw std::invalid_argument("negative transfer");auto dst=address(to);auto src=address(from);if(get(src).balance<amount)return false;Int transferred=amount;get(src).balance-=transferred;get(dst).balance+=transferred;return true;}
void State::set_storage(const std::string& addr,const Int& key,const Int& value){bytes(key,32);bytes(value,32);if(value==0)get(addr).storage.erase(key);else get(addr).storage[key]=value;}
Int State::get_storage(const std::string& addr,const Int& key)const{const auto& storage=get(addr).storage;auto it=storage.find(key);return it==storage.end()?Int(0):it->second;}
Bytes State::root()const{std::map<Bytes,Bytes> entries;for(const auto& [addr,acc]:accounts_)if(!acc.empty())entries[keccak(unhex(addr))]=acc.encode();return trie_root(entries);}
size_t State::snapshot(){snapshots_.push_back(accounts_);return snapshots_.size()-1;}
void State::revert(size_t id){if(id>=snapshots_.size())throw std::out_of_range("invalid snapshot");accounts_=snapshots_[id];snapshots_.resize(id);}
void State::commit(size_t id){if(id>=snapshots_.size())throw std::out_of_range("invalid snapshot");snapshots_.resize(id);}
Json State::to_json()const{Json j=Json::object();for(const auto& [addr,acc]:accounts_)j[addr]=acc.to_json();return j;}
State State::from_json(const Json& j){State state;for(const auto& [addr,data]:j.items()){auto& a=state.get(addr);a.balance=quantity(data.value("balance",Json("0x0")));a.nonce=quantity(data.value("nonce",Json("0x0")));a.code=unhex(data.value("code",std::string("0x")));if(data.contains("storage"))for(const auto& [k,v]:data["storage"].items())state.set_storage(addr,quantity(Json(k)),quantity(v));}return state;}
Json Log::to_json()const{Json t=Json::array();for(const auto& topic:topics)t.push_back(hex(topic));return {{"address",address},{"topics",t},{"data",hex(data)}};}
Bytes logs_bloom(const std::vector<Log>& logs){Bytes bloom(256);auto add=[&](const Bytes& data){auto h=keccak(data);for(size_t i=0;i<6;i+=2){unsigned bit=((unsigned(h[i])<<8)|h[i+1])&2047;bloom[255-bit/8]|=uint8_t(1<<(bit%8));}};for(const auto& log:logs){add(unhex(address(log.address)));for(const auto& topic:log.topics){if(topic.size()!=32)throw std::invalid_argument("topic width");add(topic);}}return bloom;}
}
