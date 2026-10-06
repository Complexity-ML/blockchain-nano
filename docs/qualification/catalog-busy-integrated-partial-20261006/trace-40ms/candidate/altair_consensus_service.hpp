#pragma once
#include "altair_worker.hpp"
#include "nano/consensus_altair_reactor.hpp"

// Core-side durable message service. The reactor (and its ChainStore) must
// outlive this service. No RPC exposure or daemon registration is implied.
// Only locally authenticated clock updates may call reactor.tick(); there is
// deliberately no peer-controlled tick message. The caller authenticates the
// anchor/profile and applies ingress limits before dispatching here.
class AltairConsensusService {
 AltairWorker worker_;
public:
 AltairConsensusService(nano::AltairDurableReactor& reactor,size_t max_jobs,size_t max_bytes,std::function<void(const std::function<void()>&)> before_message={}):
 worker_(max_jobs,max_bytes,[&reactor,before_message=std::move(before_message)](const AltairWireMessage& message,const AltairWork::Cancellation& cancelled){
  const auto checkpoint=[&]{if(cancelled->load())throw std::runtime_error("Altair request cancelled");};
  checkpoint();if(before_message)before_message(checkpoint);bool accepted=false;
  switch(message.kind){
   case AltairWireKind::block:accepted=reactor.block(message.ssz,checkpoint);break;
   case AltairWireKind::attestation:accepted=reactor.vote_ssz(message.ssz,checkpoint);break;
   case AltairWireKind::attester_slashing:accepted=reactor.slashing_ssz(message.ssz,checkpoint);break;
   default:return AltairWork::Decision::rejected;
  }
  // No cancellation check after a successful commit: publication is definitive.
  return accepted?AltairWork::Decision::accepted:AltairWork::Decision::rejected;
 }){}
 AltairWork::Dispatch dispatcher(){return worker_.dispatcher();}
 void stop(){worker_.stop();}
 void join(){worker_.join();}
};
