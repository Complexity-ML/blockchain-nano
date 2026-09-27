#pragma once
#include "altair_worker.hpp"
#include "nano/consensus_capella_reactor.hpp"

// Nano framing, not Ethereum gossipsub. Registration on libp2p is separate.
inline const std::string capella_protocol="/nano/capella/1.0.0";
// Core-side bounded service. Borrowed reactor and budget must outlive it.
// Network reader must bound fragments to frame_bytes before allocating a frame,
// acquire a peer lease from the shared InboundBudget and retain it until Done.
// Peer ID comes from the authenticated transport, never the JSON payload.
class CapellaConsensusService {
 AltairMessageIngress ingress_; // reuse the checked SSZ envelope decoder
 AltairWorker worker_; // ordered transport-independent bounded queue
 static AltairWireProfile profile(nano::CapellaDurableReactor& reactor,size_t frame,size_t block,size_t vote,size_t slashing){const auto j=reactor.network_identity();return {j.at("chainId").get<std::string>(),j.at("genesisHash").get<std::string>(),j.at("forkDigest").get<std::string>(),frame,block,vote,slashing,j.at("consensusIdentity").get<std::string>()};}
public:
 using Decision=AltairWork::Decision;
 using Cancellation=AltairWork::Cancellation;
 using Done=AltairWork::Done;
 CapellaConsensusService(nano::CapellaDurableReactor& reactor,InboundBudget& budget,
  size_t frame_bytes,size_t block_bytes,size_t vote_bytes,size_t slashing_bytes,
  size_t max_jobs,size_t max_bytes,std::function<void(const std::function<void()>&)> before_message={}):ingress_(profile(reactor,frame_bytes,block_bytes,vote_bytes,slashing_bytes),budget),
  worker_(max_jobs,max_bytes,[&reactor,before_message=std::move(before_message)](const AltairWireMessage& message,const Cancellation& cancelled){
   const auto check=[&]{if(cancelled->load())throw std::runtime_error("Capella message cancelled");};check();if(before_message)before_message(check);check();bool accepted=false;
   switch(message.kind){case AltairWireKind::block:accepted=reactor.block(message.ssz,check);break;case AltairWireKind::attestation:accepted=reactor.vote_ssz(message.ssz,check);break;case AltairWireKind::attester_slashing:accepted=reactor.slashing_ssz(message.ssz,check);break;default:return Decision::rejected;}
   // Commit is definitive: do not turn success into cancellation afterwards.
   return accepted?Decision::accepted:Decision::rejected;
  }){}
 std::optional<AltairWireMessage> decode(std::string_view frame)const{
  auto message=ingress_.decode(frame);if(message&&message->kind==AltairWireKind::voluntary_exit)return std::nullopt;return message;
 }
 // False means no job was admitted and Done will not run. Decode happens before
 // enqueue; invalid identity, frames and kinds never enter the consensus worker.
 bool submit(std::string_view frame,Cancellation cancelled,Done done){
  if(!cancelled||cancelled->load()||!done)return false;
  auto message=decode(frame);return message&&worker_.submit(std::move(*message),std::move(cancelled),std::move(done));
 }
 // Internal dispatcher: callers must first use decode/decode_compact so the
 // complete identity is checked before typed messages enter the worker.
 AltairWork::Dispatch dispatcher(){return worker_.dispatcher();}
 std::optional<AltairWireMessage> decode_compact(const nano::Json& frame)const{
  auto message=ingress_.decode_compact(frame);if(message&&message->kind==AltairWireKind::voluntary_exit)return std::nullopt;return message;
 }
 void stop(){worker_.stop();}
 void join(){worker_.join();}
};
