#include "nano/chain.hpp"
#include <fstream>
#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_capella_canonical_stream.hpp"
#include <algorithm>
#include <limits>
#include "consensus_capella_audit_scratch.hpp"
namespace nano {
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(std::string("Capella canonical audit: ")+message);}
std::function<void()> audit_checkpoint(const std::function<void()>& check){return check?check:std::function<void()>([]{});}
uint64_t audit_index_budget(size_t maximum_blocks){
 require(maximum_blocks&&maximum_blocks<=std::numeric_limits<uint64_t>::max()/canonical_stream_index_record_bytes,"private index byte budget overflow");
 return uint64_t(maximum_blocks)*canonical_stream_index_record_bytes;
}
}
// Exact historical symbol: every call delegates to the same bounded stream.
Json audit_capella_canonical_history(ChainStore& disk,const Int& chain,const Bytes& genesis,
 const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,
 const CapellaForkStore& trusted,const ForkCheckpointToken& expected,
 size_t maximum_blocks,size_t maximum_archive_bytes,const std::function<void()>& checkpoint,const std::optional<ConsensusCheckpoint>& required_finalized){
 const auto check=audit_checkpoint(checkpoint);check();
 require(maximum_blocks&&maximum_archive_bytes&&limits.blocks&&limits.total_state_bytes,"zero replay/window budget");(void)audit_index_budget(maximum_blocks);
 audit_detail::TemporaryCanonicalAuditIndex scratch(disk);check();
 const CapellaCanonicalAuditReplayWindow window{limits.blocks,limits.total_state_bytes,scratch.index()};
 const auto result=audit_capella_canonical_history(disk,chain,genesis,rules,limits,trusted,expected,maximum_blocks,maximum_archive_bytes,window,check,required_finalized);
 check();scratch.retention_reported();return result;
}
Json audit_capella_canonical_history(ChainStore& disk,const Int& chain,const Bytes& genesis,
 const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,
 const CapellaForkStore& trusted,const ForkCheckpointToken& expected,
 size_t maximum_blocks,size_t maximum_archive_bytes,const CapellaCanonicalAuditReplayWindow& window,
 const std::function<void()>& checkpoint,const std::optional<ConsensusCheckpoint>& required_finalized){
 const auto check=audit_checkpoint(checkpoint);check();
 require(maximum_blocks&&maximum_archive_bytes,"zero replay budget");
 require(trusted.blocks.size()==1&&trusted.blocks.contains(trusted.timed.view.anchor),"single trusted anchor required");
 const auto index_bytes=audit_index_budget(maximum_blocks);audit_detail::require_audit_index_outside_database(disk,window.new_private_index);check();
 CapellaDurableReactor restored(disk,chain,genesis,rules,limits,expected,check);
 const auto pruning=disk.state_pruning();size_t pruned_states=0,transactions=0,blocks=0;
 const auto saved=restored.current();const auto target_pin=capella_fork_head(*saved.state,rules,check);const auto& target=*target_pin;
 require(capella_network_anchor(trusted)==capella_network_anchor(*saved.state),"network anchor differs");
 const auto anchor=trusted.timed.view.anchor,head=consensus_beacon_header_root(target.header);
 const auto first_pin=trusted.blocks.at(anchor).lease(check);const auto first_execution_number=first_pin->execution_block.header.number;
 const auto retained_anchor_number=saved.state->blocks.at(saved.state->timed.view.anchor).execution_header().number;
 auto verify_disk=[&](const CapellaForkBlock& record,const std::function<void()>& guard){
  guard();const auto& block=record.execution_block;const auto hash=block.header.hash();
  const auto canonical=disk.block_by_number(block.header.number);guard();const auto indexed=disk.block_by_hash(hash);guard();const auto state=disk.state_at(block.header.number);guard();
  require(canonical&&indexed&&canonical->to_json(true)==block.to_json(true)&&indexed->to_json(true)==block.to_json(true),"canonical block/index mismatch");
  if(state)require(state->root()==record.execution_state.root(),"execution state mismatch");
  else{require(pruning&&block.header.number>0&&block.header.number<pruning->before&&pruning->anchor.number<=retained_anchor_number,"missing state outside authenticated pruning range");require(block.header.state_root==record.execution_state.root(),"pruned execution state differs from signed replay");++pruned_states;}
  if(pruning&&block.header.number==pruning->anchor.number)require(hash==pruning->anchor.hash,"pruning anchor differs from signed replay");
  require(record.receipts.size()==block.transactions.size(),"receipt count mismatch");
  for(size_t i=0;i<block.transactions.size();++i){
   guard();const auto& tx=block.transactions[i];auto wanted=tx.to_json();wanted["blockHash"]=hex(hash);wanted["blockNumber"]=quantity(block.header.number);wanted["transactionIndex"]=quantity(Int(std::to_string(i)));
   const auto stored=disk.transaction(tx.hash());guard();const auto receipt=disk.receipt(tx.hash());guard();
   require(stored&&*stored==wanted,"transaction index mismatch");require(receipt&&receipt->to_json()==record.receipts[i].to_json(),"receipt mismatch");require(transactions<std::numeric_limits<size_t>::max(),"transaction counter overflow");++transactions;
  }
  const auto id=consensus_beacon_header_root(record.header);const auto retained=saved.state->blocks.find(id);
  if(retained!=saved.state->blocks.end()){const auto retained_pin=retained->second.lease(guard);require(retained_pin->state==record.state&&retained_pin->execution_state.root()==record.execution_state.root(),"retained checkpoint state differs from replay");}
  require(blocks<std::numeric_limits<size_t>::max(),"block counter overflow");++blocks;guard();
 };
 const CanonicalStreamLimits bound{window.maximum_live_blocks,window.maximum_live_state_bytes,uint64_t(maximum_blocks),uint64_t(maximum_archive_bytes),index_bytes};
 const CanonicalStreamBoundary boundary{target.header,saved.state->timed.view.finalized,required_finalized.value_or(saved.state->timed.view.finalized)};
 const CanonicalStreamSource source{
  [&](const ConsensusRoot& id,size_t maximum,const std::function<void()>& guard){guard();auto entry=disk.checkpoint_archive(Bytes(id.begin(),id.end()),std::min(maximum,limits.envelope_bytes));guard();return entry;},verify_disk};
 const auto replay=stream_capella_canonical_history(trusted,rules,limits,bound,boundary,source,window.new_private_index,
  [](const CanonicalStreamVisit&,const std::function<void()>& guard){guard();},check);
 require(blocks==replay.verified_signed_blocks+1,"incomplete canonical execution verification");
 require(replay.head&&consensus_beacon_header_root(replay.head->header)==head,"head outside replayed chain");check();
 const auto tip=disk.latest();check();require(tip&&tip->header.hash()==target.execution_block.header.hash(),"tip changed during audit");
 require(bool(disk.fork_checkpoint(chain,genesis,limits.envelope_bytes,expected)),"checkpoint disappeared");check();
 Json result={{"canonicalHistoryVerified",true},{"externalFinalizedAncestorVerified",bool(required_finalized)},
  {"requiredFinalized",required_finalized?Json{{"epoch",quantity(Int(std::to_string(required_finalized->epoch)))},{"root",hex(Bytes(required_finalized->root.begin(),required_finalized->root.end()))}}:Json(nullptr)},
  {"finalized",{{"epoch",quantity(Int(std::to_string(saved.state->timed.view.finalized.epoch)))},{"root",hex(Bytes(saved.state->timed.view.finalized.root.begin(),saved.state->timed.view.finalized.root.end()))}}},{"historicalForkChoiceVerified",false},{"historicalStandaloneVotesReplayed",false},
  {"trustedAnchor",hex(Bytes(anchor.begin(),anchor.end()))},{"firstExecutionBlock",quantity(first_execution_number)},
  {"lastExecutionBlock",quantity(target.execution_block.header.number)},{"blocksIncludingTrustedAnchor",blocks},{"transactions",transactions},
  {"prunedExecutionStatesReplayed",pruned_states},{"allHistoricalStatesMaterialized",pruned_states==0},{"archiveBytes",replay.archive_bytes},{"head",hex(Bytes(head.begin(),head.end()))},{"checkpointSequence",expected.sequence},{"checkpointDigest",hex(expected.digest)},
  {"replayWindow",{{"maximumLiveBlocks",window.maximum_live_blocks},{"maximumLiveStateBytes",window.maximum_live_state_bytes},{"peakLiveBlocks",replay.peak_live_blocks},{"peakLiveStateSszBytes",replay.peak_live_state_ssz_bytes},{"peakLiveStateSszPlusNextReserveBytes",replay.peak_live_state_ssz_plus_next_reserve_bytes},{"prunes",replay.prunes},{"indexBytes",replay.index_bytes},{"maximumIndexBytes",index_bytes},{"privateIndexFile",window.new_private_index.string()},{"privateIndexAutomaticDeletion",false},{"privateIndexOwnershipForDeletionVerified",false},{"verifiedSignedBlocks",replay.verified_signed_blocks},{"finalReplayAnchorRoot",hex(Bytes(replay.replay_anchor.begin(),replay.replay_anchor.end()))},{"finalReplayAnchorSlot",replay.replay_anchor_slot},{"peakDepositAncestorRecords",replay.peak_deposit_ancestor_records},{"largestDepositHistoryCount",replay.largest_deposit_history_count},{"processRssQualified",false},{"auxiliaryDepositMemoryBounded",false}}}};
 check();return result;
}
// Exact historical database-entry symbol; no alternate unlimited replay path.
Json audit_capella_database(const Genesis& genesis,const std::filesystem::path& path,const Json& request){return audit_capella_database(genesis,path,request,{});}
Json audit_capella_database(const Genesis& genesis,const std::filesystem::path& path,const Json& request,const std::function<void()>& checkpoint){
 const auto check=audit_checkpoint(checkpoint);check();auto number=[](const Json& value){return u64(quantity(value));};
 require(genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai,"Beacon Shanghai genesis required");
 require(request.is_object(),"audit request object required");
 for(const auto& [key,_]:request.items())require(key=="profile"||key=="anchor"||key=="limits"||key=="expectedCheckpoint"||key=="maximumBlocks"||key=="maximumArchiveBytes"||key=="requiredFinalized"||key=="replayWindow","unknown audit request field");
 require(request.size()==6+size_t(request.contains("requiredFinalized"))+size_t(request.contains("replayWindow")),"six audit fields and optional requiredFinalized/replayWindow required");
 std::optional<ConsensusCheckpoint> required_finalized;
 if(request.contains("requiredFinalized")){
  const auto& value=request.at("requiredFinalized");require(value.is_object()&&value.size()==2,"invalid required finalized checkpoint");
  const auto text=value.at("root").get<std::string>();require(text.size()==66&&text.starts_with("0x"),"required finalized root must be canonical 32-byte hex");const auto bytes=unhex(text);require(bytes.size()==32&&hex(bytes)==text,"required finalized root must be canonical 32-byte hex");
  ConsensusRoot root;std::copy(bytes.begin(),bytes.end(),root.begin());required_finalized=ConsensusCheckpoint{number(value.at("epoch")),root};
 }
 auto bounded=[&](const Json& reference,size_t maximum){
  check();require(reference.is_object()&&reference.size()==2&&maximum,"invalid file reference");
  const auto file=reference.at("file").get<std::string>();require(std::filesystem::is_regular_file(file)&&std::filesystem::file_size(file)<=maximum,"file size/type");
  std::ifstream in(file,std::ios::binary);require(bool(in),"cannot open input");std::string out;char buffer[8192];
  while(in){check();in.read(buffer,sizeof(buffer));check();const auto count=size_t(in.gcount());require(count<=maximum-out.size(),"input grew beyond budget");out.append(buffer,count);}
  require(!in.bad(),"input read failure");check();return out;
 };
 const auto& l=request.at("limits");require(l.is_object()&&l.size()==8,"invalid snapshot limits");
 const CapellaStoreSnapshotLimits limits{number(l.at("envelopeBytes")),number(l.at("blocks")),number(l.at("messages")),number(l.at("equivocations")),number(l.at("stateBytes")),number(l.at("totalStateBytes")),number(l.at("replaySlots")),number(l.at("syncTrials"))};
 const auto& profile=request.at("profile");const auto rules=load_capella_fork_profile(bounded(profile,limits.envelope_bytes),unhex(profile.at("sha256").get<std::string>()),limits.envelope_bytes);
 require(rules.transition.chain_id==genesis.chain_id&&rules.transition.replay_policy==ReplayPolicy::RequireChainId,"genesis/profile mismatch");
 const auto& anchor=request.at("anchor");const auto initial=capella_store_from_snapshot(bounded(anchor,limits.envelope_bytes),unhex(anchor.at("sha256").get<std::string>()),rules,limits,check);
 const auto& token=request.at("expectedCheckpoint");require(token.is_object()&&token.size()==2,"external checkpoint required");
 const ForkCheckpointToken expected{number(token.at("sequence")),unhex(token.at("digest").get<std::string>())};
 check();ChainStore store(path,ChainStore::OpenMode::ReadOnly);store.bind_chain(genesis.chain_id,genesis.block.header.hash(),"beacon");check();
 const auto maximum_blocks=number(request.at("maximumBlocks")),maximum_archive_bytes=number(request.at("maximumArchiveBytes"));
 if(request.contains("replayWindow")){
  const auto& w=request.at("replayWindow");require(w.is_object()&&w.size()==3&&w.contains("maximumLiveBlocks")&&w.contains("maximumLiveStateBytes")&&w.contains("newPrivateIndex"),"invalid replayWindow fields");
  const CapellaCanonicalAuditReplayWindow window{number(w.at("maximumLiveBlocks")),number(w.at("maximumLiveStateBytes")),w.at("newPrivateIndex").get<std::string>()};
  return audit_capella_canonical_history(store,genesis.chain_id,genesis.block.header.hash(),rules,limits,initial,expected,maximum_blocks,maximum_archive_bytes,window,check,required_finalized);
 }
 return audit_capella_canonical_history(store,genesis.chain_id,genesis.block.header.hash(),rules,limits,initial,expected,maximum_blocks,maximum_archive_bytes,check,required_finalized);
}
}
