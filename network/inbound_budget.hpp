#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <algorithm>

// Serial access from the daemon's io_context. Rate limits use monotonic time,
// not chain timestamps. A lease follows the complete asynchronous request.
class InboundBudget {
public:
 using Clock=std::chrono::steady_clock;
private:
 struct Bucket {double tokens=16;size_t active=0;Clock::time_point updated,used;};
 struct State {std::map<std::string,Bucket> peers;size_t active=0;double tokens=128;Clock::time_point updated;explicit State(Clock::time_point now):updated(now){}};
 std::shared_ptr<State> state_;
 static void refill(double& tokens,Clock::time_point& updated,Clock::time_point now,double rate,double maximum){
  if(now>updated){tokens=std::min(maximum,tokens+std::chrono::duration<double>(now-updated).count()*rate);updated=now;}
 }
public:
 class Lease {
  friend class InboundBudget;
  std::shared_ptr<State> state_;std::string peer_;bool admitted_=false;
  Lease(std::shared_ptr<State> state,std::string peer):state_(std::move(state)),peer_(std::move(peer)){}
 public:
  Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
  ~Lease(){if(admitted_){--state_->active;--state_->peers.at(peer_).active;}}
 };
 explicit InboundBudget(Clock::time_point now=Clock::now()):state_(std::make_shared<State>(now)){}
 std::shared_ptr<Lease> acquire(const std::string& peer,Clock::time_point now=Clock::now()){
  auto& state=*state_;refill(state.tokens,state.updated,now,64,128);
  if(peer.empty()||peer.size()>128||state.active>=16||state.tokens<1)return {};
  auto found=state.peers.find(peer);
  if(found==state.peers.end()){
   if(state.peers.size()>=256){for(auto it=state.peers.begin();it!=state.peers.end();)if(!it->second.active&&now-it->second.used>=std::chrono::seconds(60))it=state.peers.erase(it);else ++it;}
   if(state.peers.size()>=256)return {};
   found=state.peers.emplace(peer,Bucket{16,0,now,now}).first;
  }
  auto& bucket=found->second;refill(bucket.tokens,bucket.updated,now,8,16);
  if(bucket.active>=4||bucket.tokens<1)return {};
  auto lease=std::shared_ptr<Lease>(new Lease(state_,peer));
  ++bucket.active;++state.active;--bucket.tokens;--state.tokens;bucket.used=now;lease->admitted_=true;
  return lease;
 }
 size_t active()const{return state_->active;}
 size_t tracked_peers()const{return state_->peers.size();}
};
