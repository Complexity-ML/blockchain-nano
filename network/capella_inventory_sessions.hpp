#pragma once
#include "nano/consensus_capella_reactor.hpp"
#include <chrono>
#include <openssl/rand.h>

// Private transport adapter supplies an authenticated principal, never a payload
// field. No snapshot escapes this registry: quota covers in-flight page reads too.
class CapellaInventorySessions {
 using Clock=std::chrono::steady_clock;
 struct Entry {std::string owner;Clock::time_point created;std::shared_ptr<const nano::CapellaInventorySnapshot> snapshot;};
 nano::CapellaDurableReactor& reactor_;
 const size_t maximum_,budget_;const Clock::duration ttl_;
 std::function<Clock::time_point()> now_;
 std::mutex mutex_;std::map<std::string,Entry> entries_;size_t bytes_=0;
 void reap(Clock::time_point now){for(auto it=entries_.begin();it!=entries_.end();){if(now<it->second.created||now-it->second.created>=ttl_){bytes_-=it->second.snapshot->root_bytes();it=entries_.erase(it);}else ++it;}}
 auto find(const std::string& owner,const std::string& id){auto it=entries_.find(id);if(owner.empty()||it==entries_.end()||it->second.owner!=owner)throw std::invalid_argument("Capella inventory handle unavailable");return it;}
public:
 CapellaInventorySessions(nano::CapellaDurableReactor& reactor,size_t maximum,size_t root_budget,Clock::duration ttl,std::function<Clock::time_point()> now=[] {return Clock::now();}):reactor_(reactor),maximum_(maximum),budget_(root_budget),ttl_(ttl),now_(std::move(now)){
  if(!maximum_||!budget_||ttl_<=Clock::duration::zero()||!now_)throw std::invalid_argument("Invalid Capella inventory session limits");
 }
 std::string open(const std::string& owner,std::string_view kind,const std::function<void()>& check={}){
  if(owner.empty()||owner.size()>256)throw std::invalid_argument("Invalid Capella inventory principal");
  std::lock_guard lock(mutex_);const auto now=now_();reap(now);if(entries_.size()>=maximum_)throw std::length_error("Capella inventory session capacity exhausted");
  auto snapshot=reactor_.capture_inventory(kind,budget_-bytes_,check);
  nano::Bytes random(32);if(RAND_bytes(random.data(),random.size())!=1)throw std::runtime_error("Capella inventory randomness unavailable");const auto id=nano::hex(random);
  if(check)check();auto [it,inserted]=entries_.emplace(id,Entry{owner,now,std::move(snapshot)});if(!inserted)throw std::runtime_error("Capella inventory handle collision");bytes_+=it->second.snapshot->root_bytes();return id;
 }
 nano::Json page(const std::string& owner,const std::string& id,const std::optional<nano::ConsensusRoot>& after,size_t count,size_t json_bytes,const std::function<void()>& check={}){
  std::lock_guard lock(mutex_);reap(now_());return find(owner,id)->second.snapshot->page(after,count,json_bytes,check);
 }
 void close(const std::string& owner,const std::string& id){std::lock_guard lock(mutex_);reap(now_());auto it=find(owner,id);bytes_-=it->second.snapshot->root_bytes();entries_.erase(it);}
};
