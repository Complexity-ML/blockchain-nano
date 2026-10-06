#pragma once
#include "nano/consensus_capella_reactor.hpp"
#include <algorithm>
#include <chrono>
#include <set>
#include <openssl/rand.h>

// Private transport adapter supplies an authenticated principal, never a payload
// field. No snapshot escapes this registry: quota covers in-flight page reads too.
class CapellaInventoryCaptureBusy : public std::length_error {
public: CapellaInventoryCaptureBusy():std::length_error("Capella inventory capture busy"){}
};

class CapellaInventorySessions {
 using Clock=std::chrono::steady_clock;
 struct Entry {std::string owner;Clock::time_point created;std::shared_ptr<const nano::CapellaInventorySnapshot> snapshot;std::string generation,request,kind;};
 struct PendingCapture {std::string owner,generation,request;bool cancelled=false;};
 std::optional<PendingCapture> pending_capture_;
 nano::CapellaDurableReactor& reactor_;
 const size_t maximum_,budget_;const Clock::duration ttl_;
 std::function<Clock::time_point()> now_;
 // Serialize captures separately: never retain the control mutex while waiting
 // for the reactor. A capture keeps its full byte reservation until it drains.
 std::mutex capture_mutex_,mutex_;std::map<std::string,Entry> entries_;size_t bytes_=0,reserved_bytes_=0;bool stopped_=false;
 std::string generation_=random_id();std::set<std::pair<std::string,std::string>> retired_;
 static std::string random_id(){nano::Bytes value(32);if(RAND_bytes(value.data(),value.size())!=1)throw std::runtime_error("Capella inventory randomness unavailable");return nano::hex(value);}
 static void identity(const std::string& owner,const std::string& id){if(owner.empty()||owner.size()>256||id.size()!=66||!id.starts_with("0x")||!std::all_of(id.begin()+2,id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))throw std::invalid_argument("Invalid Capella inventory identity");}
 // Retire identifiers without unbounded tombstones. Rotation fences every older
 // acquire. Existing handles and an already-reserved capture remain tracked;
 // cancellation of that capture is retained even across tombstone rotation.
 void retire(const std::string& owner,const std::string& generation,const std::string& request){
  if(!request.empty()&&pending_capture_&&pending_capture_->owner==owner&&pending_capture_->generation==generation&&pending_capture_->request==request)pending_capture_->cancelled=true;
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
  return capture(owner,kind,check,generation,request);
 }
 // Also fences cancel-before-acquire. An old generation cannot create a new
 // snapshot; its existing snapshot can still be found and released.
 void release(const std::string& owner,const std::string& generation,const std::string& request){
  identity(owner,request);identity(owner,generation);std::lock_guard lock(mutex_);if(stopped_)return;reap(now_());retire(owner,generation,request);
  for(auto it=entries_.begin();it!=entries_.end();++it)if(it->second.owner==owner&&it->second.generation==generation&&it->second.request==request){bytes_-=it->second.snapshot->root_bytes();entries_.erase(it);break;}
 }
 std::string open(const std::string& owner,std::string_view kind,const std::function<void()>& check={}){
  if(owner.empty()||owner.size()>256)throw std::invalid_argument("Invalid Capella inventory principal");
  return capture(owner,kind,check,{},{});
 }
private:
 std::string capture(const std::string& owner,std::string_view kind,const std::function<void()>& check,const std::string& generation,const std::string& request){
  std::unique_lock capture_lock(capture_mutex_,std::defer_lock);
  if(!capture_lock.try_lock())throw CapellaInventoryCaptureBusy();
  Clock::time_point created;size_t allowance;std::string id;
  {
   std::lock_guard lock(mutex_);created=now_();reap(created);
   if(stopped_)throw std::runtime_error("Capella inventory sessions stopped");
   if(!request.empty()){
    if(generation!=generation_||retired_.contains({owner,request}))throw std::invalid_argument("Capella inventory request retired");
    for(const auto& [handle,entry]:entries_)if(entry.owner==owner&&entry.generation==generation&&entry.request==request){if(entry.kind!=kind)throw std::invalid_argument("Capella inventory request conflict");if(check)check();return handle;}
   }
   if(entries_.size()>=maximum_)throw std::length_error("Capella inventory session capacity exhausted");
   allowance=budget_-bytes_;id=random_id();if(entries_.contains(id))throw std::runtime_error("Capella inventory handle collision");
   pending_capture_=PendingCapture{owner,generation,request,false};reserved_bytes_=allowance;
  }
  // Exception unwinding destroys the snapshot before relinquishing its byte
  // reservation. stop/release never free a capture still using that reservation.
  try{
   auto snapshot=reactor_.capture_inventory(kind,allowance,check);if(check)check();
   std::lock_guard lock(mutex_);const auto now=now_();reap(now);
   if(stopped_)throw std::runtime_error("Capella inventory sessions stopped");
   if(now<created||now-created>=ttl_)throw std::invalid_argument("Capella inventory capture expired");
   if(!pending_capture_||pending_capture_->cancelled)throw std::invalid_argument("Capella inventory request retired");
   if(snapshot->root_bytes()>reserved_bytes_)throw std::length_error("Capella inventory capture exceeds reservation");
   auto [it,inserted]=entries_.emplace(id,Entry{owner,created,std::move(snapshot),generation,request,std::string(kind)});
   if(!inserted)throw std::runtime_error("Capella inventory handle collision");bytes_+=it->second.snapshot->root_bytes();reserved_bytes_=0;pending_capture_.reset();return id;
  }catch(...){std::lock_guard lock(mutex_);reserved_bytes_=0;pending_capture_.reset();throw;}
 }

public:
 nano::Json page(const std::string& owner,const std::string& id,const std::optional<nano::ConsensusRoot>& after,size_t count,size_t json_bytes,const std::function<void()>& check={}){
  std::lock_guard lock(mutex_);reap(now_());return find(owner,id)->second.snapshot->page(after,count,json_bytes,check);
 }
 nano::Json seek(const std::string& owner,const std::string& id,const nano::ConsensusRoot& boundary,size_t count,size_t json_bytes,const std::function<void()>& check={}){
  std::lock_guard lock(mutex_);reap(now_());return find(owner,id)->second.snapshot->seek(boundary,count,json_bytes,check);
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
