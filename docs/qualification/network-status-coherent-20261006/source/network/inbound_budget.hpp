#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <algorithm>

// Serial access from the daemon's io_context. Rate limits use monotonic time,
// not chain timestamps. A lease follows the complete asynchronous request.
// Bulk traffic consumes both its sub-budget and the original shared budget.
// Thus it cannot exhaust control capacity or increase any aggregate limit.
class InboundBudget {
public:
 using Clock=std::chrono::steady_clock;
 enum class Traffic {control,bulk};
private:
 struct Bucket {double tokens=16;size_t active=0;Clock::time_point updated,used;double bulk_tokens=8;size_t bulk_active=0;Clock::time_point bulk_updated;double transaction_tokens=64;Clock::time_point transaction_updated;explicit Bucket(Clock::time_point now):updated(now),used(now),bulk_updated(now),transaction_updated(now){};};
 struct State {std::map<std::string,Bucket> peers;size_t active=0;double tokens=128;Clock::time_point updated;double bulk_tokens=64;size_t bulk_active=0;Clock::time_point bulk_updated;double transaction_tokens=128;Clock::time_point transaction_updated;explicit State(Clock::time_point now):updated(now),bulk_updated(now),transaction_updated(now){}};
 std::shared_ptr<State> state_;
 static void refill(double& tokens,Clock::time_point& updated,Clock::time_point now,double rate,double maximum){
  if(now>updated){tokens=std::min(maximum,tokens+std::chrono::duration<double>(now-updated).count()*rate);updated=now;}
 }
public:
 class Lease {
  friend class InboundBudget;
  std::shared_ptr<State> state_;std::string peer_;bool admitted_=false;bool bulk_=false;
  Lease(std::shared_ptr<State> state,std::string peer):state_(std::move(state)),peer_(std::move(peer)){}
 public:
  Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
  ~Lease(){if(admitted_){--state_->active;--state_->peers.at(peer_).active;if(bulk_){--state_->bulk_active;--state_->peers.at(peer_).bulk_active;}}}
 };
 explicit InboundBudget(Clock::time_point now=Clock::now()):state_(std::make_shared<State>(now)){}
 std::shared_ptr<Lease> acquire(const std::string& peer,Clock::time_point now=Clock::now(),Traffic traffic=Traffic::control,const char** rejection=nullptr){
  if(rejection)*rejection=nullptr;auto reject=[&](const char* reason)->std::shared_ptr<Lease>{if(rejection)*rejection=reason;return {};};
  const bool bulk=traffic==Traffic::bulk;
  auto& state=*state_;refill(state.tokens,state.updated,now,64,128);
  if(peer.empty()||peer.size()>128)return reject("invalid-peer");
  if(state.active>=16)return reject("global-concurrency");if(state.tokens<1)return reject("global-rate");
  refill(state.bulk_tokens,state.bulk_updated,now,32,64);
  if(bulk&&state.bulk_active>=8)return reject("global-bulk-concurrency");if(bulk&&state.bulk_tokens<1)return reject("global-bulk-rate");
  auto found=state.peers.find(peer);
  if(found==state.peers.end()){
   if(state.peers.size()>=256){for(auto it=state.peers.begin();it!=state.peers.end();)if(!it->second.active&&now-it->second.used>=std::chrono::seconds(60))it=state.peers.erase(it);else ++it;}
   if(state.peers.size()>=256)return reject("peer-table");
   found=state.peers.emplace(peer,Bucket(now)).first;
  }
  auto& bucket=found->second;refill(bucket.tokens,bucket.updated,now,8,16);
  if(bucket.active>=4)return reject("peer-concurrency");if(bucket.tokens<1)return reject("peer-rate");
  refill(bucket.bulk_tokens,bucket.bulk_updated,now,4,8);
  if(bulk&&bucket.bulk_active>=2)return reject("peer-bulk-concurrency");if(bulk&&bucket.bulk_tokens<1)return reject("peer-bulk-rate");
  auto lease=std::shared_ptr<Lease>(new Lease(state_,peer));
  if(bulk){++bucket.bulk_active;++state.bulk_active;--bucket.bulk_tokens;--state.bulk_tokens;lease->bulk_=true;}
  ++bucket.active;++state.active;--bucket.tokens;--state.tokens;bucket.used=now;lease->admitted_=true;
  return lease;
 }
 // Charge every item before forwarding to the core, including duplicates and
 // malformed items. Both gossip versions share this budget; no work is refunded.
 bool admit_transactions(const std::string& peer,size_t count,Clock::time_point now=Clock::now()){
  if(count==0||count>50)return false;
  auto& state=*state_;auto found=state.peers.find(peer);
  if(found==state.peers.end()||found->second.active==0)return false;
  auto& bucket=found->second;
  refill(state.transaction_tokens,state.transaction_updated,now,64,128);
  refill(bucket.transaction_tokens,bucket.transaction_updated,now,8,64);
  if(state.transaction_tokens<count||bucket.transaction_tokens<count)return false;
  state.transaction_tokens-=count;bucket.transaction_tokens-=count;return true;
 }
 size_t active()const{return state_->active;}
 size_t tracked_peers()const{return state_->peers.size();}
};
