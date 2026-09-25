#pragma once
#include "altair_consensus_service.hpp"
#include "nano/rpc.hpp"
#include <future>

// Explicitly injected core endpoint; does not activate any network profile.
// Aggregate worker limits apply independently of the daemon's peer budget.
class AltairRpcService {
 InboundBudget decoder_budget_; // decoder only; peer accounting stays in daemon
 AltairMessageIngress decoder_;
 AltairConsensusService service_;
 const std::chrono::milliseconds timeout_;
public:
 AltairRpcService(nano::AltairDurableReactor& reactor,AltairWireProfile profile,
                  size_t jobs,size_t bytes,std::chrono::milliseconds timeout=std::chrono::seconds(10)):
 decoder_(std::move(profile),decoder_budget_),service_(reactor,jobs,bytes),timeout_(timeout){
  if(timeout_<=std::chrono::milliseconds::zero())throw std::invalid_argument("invalid Altair RPC timeout");
 }
 nano::Json receive(std::string_view frame,const std::function<void()>& cancelled={}){
  if(cancelled)cancelled();
  auto message=decoder_.decode(frame);
  if(!message)throw nano::RpcError(-32602,"Invalid Altair envelope");
  auto flag=std::make_shared<std::atomic<bool>>(false);
  struct CancelOnExit {AltairWork::Cancellation flag;~CancelOnExit(){flag->store(true);}} guard{flag};
  auto reply=std::make_shared<std::promise<AltairWork::Decision>>();auto result=reply->get_future();
  if(!service_.dispatcher()(std::move(*message),flag,[reply](auto decision){reply->set_value(decision);}))
   throw nano::RpcError(-32005,"Altair service capacity unavailable");
  const auto deadline=std::chrono::steady_clock::now()+timeout_;
  while(result.wait_for(std::chrono::milliseconds(5))!=std::future_status::ready){
   if(cancelled)cancelled();
   if(std::chrono::steady_clock::now()>=deadline)throw nano::RpcError(-32002,"Altair result unavailable before deadline");
  }
  const auto decision=result.get();
  if(decision==AltairWork::Decision::local_failure)throw nano::RpcError(-32603,"Altair local processing failed");
  return nano::Json{{"accepted",decision==AltairWork::Decision::accepted}};
 }
 void stop(){service_.stop();}
 void join(){service_.join();}
};
