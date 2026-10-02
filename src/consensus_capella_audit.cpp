#include "nano/chain.hpp"
#include <fstream>
#include "nano/consensus_capella_audit.hpp"
#include <algorithm>
#include "consensus_capella_audit_ancestry.hpp"
namespace nano {
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(std::string("Capella canonical audit: ")+message);}
ConsensusRoot root_at(const Bytes& wire,size_t offset){require(offset<=wire.size()&&wire.size()-offset>=32,"truncated archive parent");ConsensusRoot root;std::copy_n(wire.begin()+offset,32,root.begin());return root;}
}
Json audit_capella_canonical_history(ChainStore& disk,const Int& chain,const Bytes& genesis,
 const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,
 const CapellaForkStore& trusted,const ForkCheckpointToken& expected,
 size_t maximum_blocks,size_t maximum_archive_bytes,const std::function<void()>& check,const std::optional<ConsensusCheckpoint>& required_finalized){
 require(maximum_blocks&&maximum_archive_bytes,"zero replay budget");
 require(trusted.blocks.size()==1&&trusted.blocks.contains(trusted.timed.view.anchor),"single trusted anchor required");
 // Revalidate the caller-authenticated anchor's structural/state bindings.
 const auto encoded=capella_store_snapshot(trusted,rules,limits,check);
 auto replay=capella_store_from_snapshot(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules,limits,check);
 CapellaDurableReactor restored(disk,chain,genesis,rules,limits,expected,check);
 const auto saved=restored.current();const auto& target=capella_fork_head(*saved.state,rules,check);
 require(capella_network_anchor(replay)==capella_network_anchor(*saved.state),"network anchor differs");
 const auto anchor=replay.timed.view.anchor,head=consensus_beacon_header_root(target.header);
 const auto first=replay.blocks.at(anchor);std::vector<ConsensusRoot> reverse;
 auto at=head;uint64_t child_slot=UINT64_MAX;size_t archive_bytes=0;
 while(at!=anchor){
  if(check)check();require(reverse.size()<maximum_blocks,"block budget exceeded");
  const auto entry=disk.checkpoint_archive(Bytes(at.begin(),at.end()),limits.envelope_bytes);
  require(bool(entry),"signed archive missing");require(entry->slot<child_slot&&entry->slot>first.header.slot,"archive slots do not descend to anchor");
  require(entry->payload.size()>=184,"truncated signed archive");
  require(entry->payload.size()<=maximum_archive_bytes-archive_bytes,"archive byte budget exceeded");archive_bytes+=entry->payload.size();
  reverse.push_back(at);child_slot=entry->slot;at=root_at(entry->payload,116);
 }
 std::reverse(reverse.begin(),reverse.end());size_t transactions=0,blocks=0;
 auto verify_disk=[&](const CapellaForkBlock& record){
  if(check)check();const auto& block=record.execution_block;const auto hash=block.header.hash();
  const auto canonical=disk.block_by_number(block.header.number),indexed=disk.block_by_hash(hash);const auto state=disk.state_at(block.header.number);
  require(canonical&&indexed&&canonical->to_json(true)==block.to_json(true)&&indexed->to_json(true)==block.to_json(true),"canonical block/index mismatch");
  require(state&&state->root()==record.execution_state.root(),"execution state mismatch");
  require(record.receipts.size()==block.transactions.size(),"receipt count mismatch");
  for(size_t i=0;i<block.transactions.size();++i){
   if(check)check();const auto& tx=block.transactions[i];auto wanted=tx.to_json();wanted["blockHash"]=hex(hash);wanted["blockNumber"]=quantity(block.header.number);wanted["transactionIndex"]=quantity(Int(std::to_string(i)));
   const auto stored=disk.transaction(tx.hash());const auto receipt=disk.receipt(tx.hash());
   require(stored&&*stored==wanted,"transaction index mismatch");require(receipt&&receipt->to_json()==record.receipts[i].to_json(),"receipt mismatch");++transactions;
  }
  const auto id=consensus_beacon_header_root(record.header);const auto retained=saved.state->blocks.find(id);
  if(retained!=saved.state->blocks.end())require(retained->second.state==record.state&&retained->second.execution_state.root()==record.execution_state.root(),"retained checkpoint state differs from replay");
  ++blocks;
 };
 verify_disk(first);
 for(const auto& id:reverse){
  if(check)check();const auto entry=disk.checkpoint_archive(Bytes(id.begin(),id.end()),limits.envelope_bytes);require(bool(entry),"archive disappeared");
  require(rules.slot_duration_ms&&entry->slot<=(UINT64_MAX-replay.timed.clock.genesis_time_ms)/rules.slot_duration_ms,"archive clock overflow");
  replay=prepare_capella_store_tick(replay,replay.timed.clock.genesis_time_ms+entry->slot*rules.slot_duration_ms,rules,limits.replay_slots,limits.sync_trials,check);
  auto next=prepare_capella_fork_block(replay,entry->payload,rules,limits.replay_slots,limits.sync_trials,check);
  require(next&&next->blocks.contains(id)&&next->blocks.size()==replay.blocks.size()+1,"signed transition rejected");
  require(next->blocks.at(id).header.slot==entry->slot,"archive slot differs from signed header");
  replay=std::move(*next);verify_disk(replay.blocks.at(id));
 }
 require(replay.blocks.contains(head)&&replay.blocks.contains(saved.state->timed.view.finalized.root),"head/finalized checkpoint outside replayed chain");
 if(required_finalized)audit_detail::require_finalized_ancestor(replay,saved.state->timed.view.finalized,*required_finalized,rules.transition.consensus.epoch.ffg.slots_per_epoch,check);
 const auto tip=disk.latest();require(tip&&tip->header.hash()==target.execution_block.header.hash(),"tip changed during audit");
 require(bool(disk.fork_checkpoint(chain,genesis,limits.envelope_bytes,expected)),"checkpoint disappeared");
 return {{"canonicalHistoryVerified",true},{"externalFinalizedAncestorVerified",bool(required_finalized)},
  {"finalized",{{"epoch",quantity(Int(std::to_string(saved.state->timed.view.finalized.epoch)))},{"root",hex(Bytes(saved.state->timed.view.finalized.root.begin(),saved.state->timed.view.finalized.root.end()))}}},{"historicalForkChoiceVerified",false},{"historicalStandaloneVotesReplayed",false},
  {"trustedAnchor",hex(Bytes(anchor.begin(),anchor.end()))},{"firstExecutionBlock",quantity(first.execution_block.header.number)},
  {"lastExecutionBlock",quantity(target.execution_block.header.number)},{"blocksIncludingTrustedAnchor",blocks},{"transactions",transactions},
  {"archiveBytes",archive_bytes},{"head",hex(Bytes(head.begin(),head.end()))},{"checkpointSequence",expected.sequence},{"checkpointDigest",hex(expected.digest)}};
}
}

namespace nano {
Json audit_capella_database(const Genesis& genesis,const std::filesystem::path& path,const Json& request){
 auto number=[](const Json& value){return u64(quantity(value));};
 require(genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"Beacon Shanghai genesis required");
 require(request.is_object()&&(request.size()==6||(request.size()==7&&request.contains("requiredFinalized"))),"request must contain six audit fields and optional requiredFinalized");
 std::optional<ConsensusCheckpoint> required_finalized;
 if(request.contains("requiredFinalized")){
  const auto& value=request.at("requiredFinalized");require(value.is_object()&&value.size()==2,"invalid required finalized checkpoint");
  const auto text=value.at("root").get<std::string>();require(text.size()==66&&text.starts_with("0x"),"required finalized root must be canonical 32-byte hex");const auto bytes=unhex(text);require(bytes.size()==32&&hex(bytes)==text,"required finalized root must be canonical 32-byte hex");
  ConsensusRoot root;std::copy(bytes.begin(),bytes.end(),root.begin());required_finalized=ConsensusCheckpoint{number(value.at("epoch")),root};
 }
 auto bounded=[](const Json& reference,size_t maximum){
  require(reference.is_object()&&reference.size()==2&&maximum,"invalid file reference");
  const auto file=reference.at("file").get<std::string>();require(std::filesystem::is_regular_file(file)&&std::filesystem::file_size(file)<=maximum,"file size/type");
  std::ifstream in(file,std::ios::binary);require(bool(in),"cannot open input");std::string out;char buffer[8192];
  while(in){in.read(buffer,sizeof(buffer));const auto count=size_t(in.gcount());require(count<=maximum-out.size(),"input grew beyond budget");out.append(buffer,count);}
  require(!in.bad(),"input read failure");return out;
 };
 const auto& l=request.at("limits");require(l.is_object()&&l.size()==8,"invalid snapshot limits");
 const CapellaStoreSnapshotLimits limits{number(l.at("envelopeBytes")),number(l.at("blocks")),number(l.at("messages")),number(l.at("equivocations")),number(l.at("stateBytes")),number(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 const auto& profile=request.at("profile");const auto rules=load_capella_fork_profile(bounded(profile,limits.envelope_bytes),unhex(profile.at("sha256").get<std::string>()),limits.envelope_bytes);
 require(rules.transition.chain_id==genesis.chain_id&&rules.transition.replay_policy==ReplayPolicy::RequireChainId,"genesis/profile mismatch");
 const auto& anchor=request.at("anchor");const auto initial=capella_store_from_snapshot(bounded(anchor,limits.envelope_bytes),unhex(anchor.at("sha256").get<std::string>()),rules,limits);
 const auto& token=request.at("expectedCheckpoint");require(token.is_object()&&token.size()==2,"external checkpoint required");
 const ForkCheckpointToken expected{number(token.at("sequence")),unhex(token.at("digest").get<std::string>())};
 ChainStore store(path,ChainStore::OpenMode::ReadOnly);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");
 return audit_capella_canonical_history(store,genesis.chain_id,genesis.block.header.hash(),rules,limits,initial,expected,number(request.at("maximumBlocks")),number(request.at("maximumArchiveBytes")),{},required_finalized);
}
}
