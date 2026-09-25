#pragma once
#include "altair_consensus_service.hpp"
#include "altair_receipts.hpp"
#include "nano/rpc.hpp"
#include <future>

// Explicitly injected core endpoint; does not activate any network profile.
// Aggregate worker limits apply independently of the daemon's peer budget.
class AltairRpcService {
 nano::AltairDurableReactor& reactor_;
 const AltairWireProfile profile_;
 const std::string consensus_identity_;
 const size_t worker_jobs_;
 std::function<bool()> clock_healthy_;
 InboundBudget decoder_budget_; // decoder only; peer accounting stays in daemon
 AltairMessageIngress decoder_;
 AltairConsensusService service_;
 const std::chrono::milliseconds timeout_;
 AltairReceipts receipts_;
public:
 AltairRpcService(nano::AltairDurableReactor& reactor,AltairWireProfile profile,
                  size_t jobs,size_t bytes,std::chrono::milliseconds timeout=std::chrono::seconds(10),std::function<void(const std::function<void()>&)> before_message={},std::function<bool()> clock_healthy={},std::string consensus_identity={}):
 reactor_(reactor),profile_(std::move(profile)),consensus_identity_(std::move(consensus_identity)),worker_jobs_(jobs),clock_healthy_(std::move(clock_healthy)),decoder_(profile_,decoder_budget_),service_(reactor,jobs,bytes,std::move(before_message)),timeout_(timeout),receipts_(jobs){
  if(timeout_<=std::chrono::milliseconds::zero())throw std::invalid_argument("invalid Altair RPC timeout");
 }
 nano::Json status(){
  const auto current=reactor_.current();
  return nano::Json{{"sequence",nano::quantity(nano::Int(std::to_string(current.token.sequence)))},{"digest",nano::hex(current.token.digest)},
   {"timeMs",nano::quantity(nano::Int(std::to_string(current.state->timed.clock.time_ms)))},
   {"admissionSession",receipts_.session()},{"chainId",profile_.chain_id},{"genesisHash",profile_.genesis_hash},{"forkDigest",profile_.fork_digest},
   {"workerJobs",worker_jobs_},{"consensusIdentity",consensus_identity_.empty()?nano::Json(nullptr):nano::Json(consensus_identity_)},
   {"wire",{{"frameBytes",profile_.frame_bytes},{"blockBytes",profile_.block_bytes},{"attestationBytes",profile_.attestation_bytes},{"slashingBytes",profile_.slashing_bytes}}},
   {"clockHealthy",clock_healthy_?nano::Json(clock_healthy_()):nano::Json(nullptr)},{"executionBound",false},{"networkProtocolActive",false}};
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
 nano::Json submit(const std::string& session,uint64_t sequence,std::string_view frame){
  auto message=decoder_.decode(frame);if(!message)throw nano::RpcError(-32602,"Invalid Altair envelope");
  return receipts_.submit(session,sequence,std::move(*message),service_.dispatcher());
 }
 nano::Json job(const std::string& session,uint64_t sequence,bool cancel=false,bool retire=false){return receipts_.get(session,sequence,cancel,retire);}
 nano::Json rotate(const std::string& expected){return receipts_.rotate(expected);}
 nano::Json jobs(){return receipts_.list();}
 void stop(){service_.stop();}
 void join(){service_.join();}
};
