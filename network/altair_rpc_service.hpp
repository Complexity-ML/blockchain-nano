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
 const bool network_admission_;
 std::function<bool()> clock_healthy_;
 InboundBudget decoder_budget_; // decoder only; peer accounting stays in daemon
 AltairMessageIngress decoder_;
 AltairConsensusService service_;
 const std::chrono::milliseconds timeout_;
 AltairReceipts receipts_;
 struct CatalogEntry {nano::ConsensusRoot root,parent;uint64_t slot;};
 struct Catalog {std::weak_ptr<const nano::AltairSignedBlockArchive> archive;std::vector<CatalogEntry> entries;std::map<nano::ConsensusRoot,size_t> positions;std::string epoch;};
 std::mutex catalog_mutex_;std::shared_ptr<const Catalog> catalog_;
 std::shared_ptr<const Catalog> catalog(const nano::AltairPublishedStore& current){
  std::lock_guard lock(catalog_mutex_);if(catalog_&&catalog_->archive.lock()==current.signed_blocks)return catalog_;
  auto next=std::make_shared<Catalog>();bool append=bool(catalog_);
  if(append)for(const auto& [id,_]:catalog_->positions)if(!current.signed_blocks->contains(id)){append=false;break;}
  if(append)*next=*catalog_;else{nano::Bytes nonce(32);if(RAND_bytes(nonce.data(),nonce.size())!=1)throw std::runtime_error("Altair inventory randomness unavailable");next->epoch=nano::hex(nonce);}
  std::vector<CatalogEntry> added;for(const auto& [id,_]:*current.signed_blocks)if(!next->positions.contains(id)){const auto& h=current.state->blocks.at(id).header;added.push_back({id,h.parent_root,h.slot});}
  std::sort(added.begin(),added.end(),[](const auto& a,const auto& b){return a.slot!=b.slot?a.slot<b.slot:a.root<b.root;});
  for(const auto& entry:added){next->positions.emplace(entry.root,next->entries.size());next->entries.push_back(entry);}
  next->archive=current.signed_blocks;catalog_=next;return next;
 }
 static nano::ConsensusRoot block_root(const std::string& encoded){
  if(encoded.size()!=66)throw nano::RpcError(-32602,"Invalid Altair block root");const auto bytes=nano::unhex(encoded);
  if(bytes.size()!=32||nano::hex(bytes)!=encoded)throw nano::RpcError(-32602,"Invalid Altair block root");nano::ConsensusRoot result{};std::copy(bytes.begin(),bytes.end(),result.begin());return result;
 }
public:
 AltairRpcService(nano::AltairDurableReactor& reactor,AltairWireProfile profile,
                  size_t jobs,size_t bytes,std::chrono::milliseconds timeout=std::chrono::seconds(10),std::function<void(const std::function<void()>&)> before_message={},std::function<bool()> clock_healthy={},std::string consensus_identity={},bool network_admission=false):
 reactor_(reactor),profile_(std::move(profile)),consensus_identity_(std::move(consensus_identity)),worker_jobs_(jobs),network_admission_(network_admission),clock_healthy_(std::move(clock_healthy)),decoder_(profile_,decoder_budget_),service_(reactor,jobs,bytes,std::move(before_message)),timeout_(timeout),receipts_(jobs){
  if(network_admission_&&(consensus_identity_.empty()||!clock_healthy_))throw std::invalid_argument("Altair network admission needs identity and local clock");
  if(timeout_<=std::chrono::milliseconds::zero())throw std::invalid_argument("invalid Altair RPC timeout");
 }
 nano::Json status(){
  const auto current=reactor_.current();
  return nano::Json{{"sequence",nano::quantity(nano::Int(std::to_string(current.token.sequence)))},{"digest",nano::hex(current.token.digest)},
   {"timeMs",nano::quantity(nano::Int(std::to_string(current.state->timed.clock.time_ms)))},
   {"admissionSession",receipts_.session()},{"chainId",profile_.chain_id},{"genesisHash",profile_.genesis_hash},{"forkDigest",profile_.fork_digest},
   {"networkAdmissionEnabled",network_admission_},{"workerJobs",worker_jobs_},{"consensusIdentity",consensus_identity_.empty()?nano::Json(nullptr):nano::Json(consensus_identity_)},
   {"wire",{{"frameBytes",profile_.frame_bytes},{"blockBytes",profile_.block_bytes},{"attestationBytes",profile_.attestation_bytes},{"slashingBytes",profile_.slashing_bytes}}},
   {"clockHealthy",clock_healthy_?nano::Json(clock_healthy_()):nano::Json(nullptr)},{"executionBound",false},{"networkProtocolActive",network_admission_?nano::Json(nullptr):nano::Json(false)}};
 }
 nano::Json receive(std::string_view frame,const std::function<void()>& cancelled={}){
  if(network_admission_)throw nano::RpcError(-32601,"Synchronous Altair admission disabled for network mode");
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
 nano::Json submit(const std::string& session,uint64_t sequence,const nano::Json& frame){
  auto message=frame.is_string()?decoder_.decode(frame.get_ref<const std::string&>()):decoder_.decode_compact(frame);if(!message)throw nano::RpcError(-32602,"Invalid Altair envelope");
  return receipts_.submit(session,sequence,std::move(*message),service_.dispatcher());
 }
 nano::Json block(const std::string& root){
  const auto id=block_root(root);const auto current=reactor_.current();const auto found=current.signed_blocks->find(id);
  if(found==current.signed_blocks->end())return nano::Json{{"available",false},{"known",current.state->blocks.contains(id)},{"anchor",id==current.state->timed.view.anchor}};
  const auto& bytes=*found->second;if(bytes.size()>profile_.block_bytes||bytes.size()/3+(bytes.size()%3!=0)>(16*1024*1024-1024)/4)throw nano::RpcError(-32005,"Altair block exceeds IPC export capacity");
  return nano::Json{{"available",true},{"root",root},{"sequence",nano::quantity(nano::Int(std::to_string(current.token.sequence)))},{"digest",nano::hex(current.token.digest)},
   {"envelope",{{"chainId",profile_.chain_id},{"genesisHash",profile_.genesis_hash},{"forkDigest",profile_.fork_digest},{"kind","block"},{"sszBase64",altair_base64(bytes)}}}};
 }
 nano::Json inventory(const std::string& cursor,uint64_t requested){
  if(!requested)throw nano::RpcError(-32602,"Positive Altair page limit required");const auto current=reactor_.current();
  const auto index=catalog(current);size_t offset=0;
  if(cursor!="0x"){const auto found=index->positions.find(block_root(cursor));if(found==index->positions.end())throw nano::RpcError(-32602,"Unknown Altair inventory cursor");offset=found->second+1;}
  const auto budget=std::min(profile_.frame_bytes,size_t(16*1024*1024));if(budget<=512)throw nano::RpcError(-32005,"Altair inventory capacity unavailable");
  const auto maximum=std::min<uint64_t>(requested,(budget-512)/256);if(!maximum)throw nano::RpcError(-32005,"Altair inventory capacity unavailable");
  nano::Json entries=nano::Json::array();std::string last;
  for(size_t n=0;n<maximum&&offset<index->entries.size();++n,++offset){const auto& entry=index->entries[offset];last=nano::hex(nano::Bytes(entry.root.begin(),entry.root.end()));entries.push_back({{"root",last},{"parentRoot",nano::hex(nano::Bytes(entry.parent.begin(),entry.parent.end()))},{"slot",nano::quantity(nano::Int(std::to_string(entry.slot)))}});}
  return nano::Json{{"sequence",nano::quantity(nano::Int(std::to_string(current.token.sequence)))},{"digest",nano::hex(current.token.digest)},{"catalogEpoch",index->epoch},{"blocks",entries},{"next",offset==index->entries.size()?nano::Json(nullptr):nano::Json(last)},{"missingSignedBlocks",current.state->blocks.size()-1-current.signed_blocks->size()}};
 }
 nano::Json job(const std::string& session,uint64_t sequence,bool cancel=false,bool retire=false){return receipts_.get(session,sequence,cancel,retire);}
 nano::Json rotate(const std::string& expected){return receipts_.rotate(expected);}
 nano::Json jobs(){return receipts_.list();}
 void stop(){service_.stop();}
 void join(){service_.join();}
};
