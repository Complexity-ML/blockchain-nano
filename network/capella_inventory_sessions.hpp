#pragma once
#include "nano/consensus_capella_reactor.hpp"
#include <algorithm>
#include <chrono>
#include <set>
#include <openssl/rand.h>

// Private transport adapter supplies an authenticated principal, never a payload
// field. No snapshot escapes this registry: quota covers in-flight page reads too.
class CapellaInventorySessions {
 using Clock=std::chrono::steady_clock;
 struct Entry {std::string owner;Clock::time_point created;std::shared_ptr<const nano::CapellaInventorySnapshot> snapshot;std::string generation,request,kind;};
 nano::CapellaDurableReactor& reactor_;
 const size_t maximum_,budget_;const Clock::duration ttl_;
 std::function<Clock::time_point()> now_;
 std::mutex mutex_;std::map<std::string,Entry> entries_;size_t bytes_=0;bool stopped_=false;
 std::string generation_=random_id();std::set<std::pair<std::string,std::string>> retired_;
 static std::string random_id(){nano::Bytes value(32);if(RAND_bytes(value.data(),value.size())!=1)throw std::runtime_error("Capella inventory randomness unavailable");return nano::hex(value);}
 static void identity(const std::string& owner,const std::string& id){if(owner.empty()||owner.size()>256||id.size()!=66||!id.starts_with("0x")||!std::all_of(id.begin()+2,id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))throw std::invalid_argument("Invalid Capella inventory identity");}
 // Retire identifiers without unbounded tombstones. Rotation fences every older
 // acquire, while existing handles and request releases remain valid.
 void retire(const std::string& owner,const std::string& generation,const std::string& request){
  if(request.empty()||generation!=generation_||retired_.contains({owner,request}))return;
  if(retired_.size()>=maximum_){auto next=random_id();if(next==generation_)throw std::runtime_error("Capella generation collision");generation_.swap(next);retired_.clear();return;}
  retired_.emplace(owner,request);
 }
 void reap(Clock::time_point now){for(auto it=entries_.begin();it!=entries_.end();){if(now<it->second.created||now-it->second.created>=ttl_){retire(it->second.owner,it->second.generation,it->second.request);bytes_-=it->second.snapshot->root_bytes();it=entries_.erase(it);}else ++it;}}
 auto find(const std::string& owner,const std::string& id){auto it=entries_.find(id);if(owner.empty()||it==entries_.end()||it->second.owner!=owner)throw std::invalid_argument("Capella inventory handle unavailable");return it;}
public:
 CapellaInventorySessions(nano::CapellaDurableReactor& reactor,size_t maximum,size_t root_budget,Clock::duration ttl,std::function<Clock::time_point()> now=[] {return Clock::now();}):reactor_(reactor),maximum_(maximum),budget_(root_budget),ttl_(ttl),now_(std::move(now)){
  if(!maximum_||!budget_||ttl_<=Clock::duration::zero()||!now_)throw std::invalid_argument("Invalid Capella inventory session limits");
 }
 std::string generation(){std::lock_guard lock(mutex_);if(stopped_)throw std::runtime_error("Capella inventory sessions stopped");return generation_;}
 std::string acquire(const std::string& owner,const std::string& generation,const std::string& request,std::string_view kind,const std::function<void()>& check={}){
  identity(owner,request);identity(owner,generation);
  std::lock_guard lock(mutex_);const auto now=now_();reap(now);
  if(stopped_||generation!=generation_||retired_.contains({owner,request}))throw std::invalid_argument("Capella inventory request retired");
  for(const auto& [id,entry]:entries_)if(entry.owner==owner&&entry.generation==generation&&entry.request==request){if(entry.kind!=kind)throw std::invalid_argument("Capella inventory request conflict");if(check)check();return id;}
  return open_locked(owner,kind,now,check,generation,request);
 }
 // Also fences cancel-before-acquire. An old generation cannot create a new
 // snapshot; its existing snapshot can still be found and released.
 void release(const std::string& owner,const std::string& generation,const std::string& request){
  identity(owner,request);identity(owner,generation);std::lock_guard lock(mutex_);if(stopped_)return;reap(now_());retire(owner,generation,request);
  for(auto it=entries_.begin();it!=entries_.end();++it)if(it->second.owner==owner&&it->second.generation==generation&&it->second.request==request){bytes_-=it->second.snapshot->root_bytes();entries_.erase(it);break;}
 }
 std::string open(const std::string& owner,std::string_view kind,const std::function<void()>& check={}){
  if(owner.empty()||owner.size()>256)throw std::invalid_argument("Invalid Capella inventory principal");
  std::lock_guard lock(mutex_);const auto now=now_();reap(now);return open_locked(owner,kind,now,check,{},{});
 }
private:
 std::string open_locked(const std::string& owner,std::string_view kind,Clock::time_point now,const std::function<void()>& check,const std::string& generation,const std::string& request){
  if(stopped_)throw std::runtime_error("Capella inventory sessions stopped");if(entries_.size()>=maximum_)throw std::length_error("Capella inventory session capacity exhausted");
  auto snapshot=reactor_.capture_inventory(kind,budget_-bytes_,check);const auto id=random_id();
  if(check)check();auto [it,inserted]=entries_.emplace(id,Entry{owner,now,std::move(snapshot),generation,request,std::string(kind)});if(!inserted)throw std::runtime_error("Capella inventory handle collision");bytes_+=it->second.snapshot->root_bytes();return id;
 }
public:
 nano::Json page(const std::string& owner,const std::string& id,const std::optional<nano::ConsensusRoot>& after,size_t count,size_t json_bytes,const std::function<void()>& check={}){
  std::lock_guard lock(mutex_);reap(now_());return find(owner,id)->second.snapshot->page(after,count,json_bytes,check);
 }
 void stop(){std::lock_guard lock(mutex_);stopped_=true;entries_.clear();retired_.clear();bytes_=0;}
 // Closing is idempotent: a lost acknowledgement can be retried without
 // extending leases. Request retirement remains bounded. Foreign handles are protected.
 void close(const std::string& owner,const std::string& id){
  if(owner.empty()||owner.size()>256||id.size()!=66||!id.starts_with("0x")||!std::all_of(id.begin()+2,id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))throw std::invalid_argument("Invalid Capella inventory close");
  std::lock_guard lock(mutex_);reap(now_());auto it=entries_.find(id);if(it==entries_.end())return;
  if(it->second.owner!=owner)throw std::invalid_argument("Capella inventory handle unavailable");
  retire(it->second.owner,it->second.generation,it->second.request);bytes_-=it->second.snapshot->root_bytes();entries_.erase(it);
 }
};
