#pragma once
#include "altair_work.hpp"
#include "nano/types.hpp"
#include <map>
#include <mutex>
#include <openssl/rand.h>

// Process-local receipt ledger. No unfinished receipt is evicted. Completed
// receipts require explicit retirement; a watermark forbids replay of retired
// sequence numbers. A new session after restart means UNKNOWN outcome, not rejection.
class AltairReceipts {
 struct Entry {std::string digest;AltairWork::Cancellation cancel=std::make_shared<std::atomic<bool>>(false);std::atomic<int> result{-1};explicit Entry(std::string d):digest(std::move(d)){} };
 struct State {std::mutex mutex;uint64_t watermark=0;std::map<uint64_t,std::shared_ptr<Entry>> entries;};
 const size_t maximum_;
 std::string session_;
 std::shared_ptr<State> state_=std::make_shared<State>();
 void session(const std::string& value)const{if(value!=session_)throw std::invalid_argument("Altair admission session changed; outcome unknown");}
 static nano::Json view(uint64_t sequence,const Entry& e){
  const auto decision=e.result.load();
  nano::Json j{{"sequence",nano::quantity(nano::Int(std::to_string(sequence)))},{"payloadDigest",e.digest},{"cancelRequested",e.cancel->load()},{"state",decision>=0?"completed":"pending"}};
  if(decision>=0)j["decision"]=decision==int(AltairWork::Decision::accepted)?"accepted":decision==int(AltairWork::Decision::rejected)?"rejected":"local_failure";
  return j;
 }
public:
 explicit AltairReceipts(size_t maximum):maximum_(maximum){
  if(!maximum_)throw std::invalid_argument("Altair receipt capacity required");nano::Bytes nonce(32);if(RAND_bytes(nonce.data(),nonce.size())!=1)throw std::runtime_error("Altair session randomness unavailable");session_=nano::hex(nonce);
 }
 const std::string& session()const{return session_;}
 nano::Json submit(const std::string& instance,uint64_t sequence,AltairWireMessage message,const AltairWork::Dispatch& dispatch){
  session(instance);if(!sequence)throw std::invalid_argument("Altair sequence must be positive");
  nano::Bytes content;content.reserve(message.ssz.size()+1);content.push_back(static_cast<uint8_t>(message.kind));content.insert(content.end(),message.ssz.begin(),message.ssz.end());const auto digest=nano::hex(nano::sha256(content));
  std::lock_guard lock(state_->mutex);
  if(auto it=state_->entries.find(sequence);it!=state_->entries.end()){
   if(it->second->digest!=digest)throw std::invalid_argument("Altair sequence reused for different content");return view(sequence,*it->second);
  }
  if(sequence<=state_->watermark)return nano::Json{{"state","retired"}};
  if(state_->entries.size()>=maximum_)return nano::Json{{"state","limited"}};
  auto entry=std::make_shared<Entry>(digest);
  state_->entries.emplace(sequence,entry);
  bool admitted=false;
  try{admitted=dispatch(std::move(message),entry->cancel,[entry](auto result){int empty=-1;entry->result.compare_exchange_strong(empty,int(result));});}
  catch(...){state_->entries.erase(sequence);throw;}
  if(!admitted){state_->entries.erase(sequence);return nano::Json{{"state","limited"}};}
  state_->watermark=sequence;return view(sequence,*entry);
 }
 nano::Json get(const std::string& instance,uint64_t sequence,bool cancel=false,bool retire=false){
  session(instance);if(!sequence)throw std::invalid_argument("Altair sequence must be positive");std::lock_guard lock(state_->mutex);
  const auto it=state_->entries.find(sequence);
  if(it==state_->entries.end()){
   // Fence a not-yet-admitted request: a delayed HTTP Submit must not start it
   // after Cancel has reported that no worker remains for this sequence.
   if(cancel)state_->watermark=std::max(state_->watermark,sequence);
   return nano::Json{{"state",sequence<=state_->watermark?"retired":"unknown"}};
  }
  if(cancel)it->second->cancel->store(true);
  auto result=view(sequence,*it->second);
  if(retire){if(result.at("state")!="completed")throw std::invalid_argument("Cannot retire running Altair job");state_->entries.erase(it);}
  return result;
 }
 nano::Json list(){std::lock_guard lock(state_->mutex);nano::Json entries=nano::Json::array();for(const auto& [seq,e]:state_->entries)entries.push_back(view(seq,*e));return nano::Json{{"session",session_},{"highWater",nano::quantity(nano::Int(std::to_string(state_->watermark)))},{"jobs",entries}};}
};
