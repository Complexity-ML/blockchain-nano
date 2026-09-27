#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_message_ssz.hpp"
namespace nano {
namespace {
void archive_require(bool value){if(!value)throw std::invalid_argument("Invalid Capella durable archive");}
uint64_t archive_number(std::span<const uint8_t> bytes,size_t at,size_t width=8){uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(bytes[at+i])<<(8*i);return value;}
template<size_t N>std::array<uint8_t,N> archive_array(std::span<const uint8_t> bytes,size_t at){std::array<uint8_t,N> out{};std::copy_n(bytes.begin()+at,N,out.begin());return out;}
void verify_archived_block(const ConsensusRoot& id,const Bytes& wire,const CapellaForkStore& store,const CapellaForkBlockRules& rules,const std::function<void()>& check){
 const auto& r=rules.transition;
 archive_require(id!=store.timed.view.anchor&&store.blocks.contains(id)&&wire.size()>=404&&wire.size()<=r.consensus.maximum_block_bytes);
 archive_require(archive_number(wire,0,4)==100&&archive_number(wire,180,4)==84);
 const ConsensusBeaconHeader header{archive_number(wire,100),archive_number(wire,108),archive_array<32>(wire,116),archive_array<32>(wire,148),capella_beacon_body_root(std::span(wire).subspan(184),r.consensus.body,r.consensus.state.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,check)};
 const auto& block=store.blocks.at(id);archive_require(header==block.header&&consensus_beacon_header_root(header)==id);
 const auto context=capella_fork_block_context(block.state,header,CapellaEpochRules{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,check);
 const auto proposer=context.registry.validators.find(header.proposer_index);archive_require(proposer!=context.registry.validators.end());
 const auto& state=block.state.ssz;const auto version=archive_array<4>(state,context.registry.epoch<archive_number(state,56)?48:52);
 archive_require(verify_consensus_beacon_header(header,version,archive_array<32>(state,8),proposer->second.public_key,std::span(wire).subspan(4,96)));if(check)check();
}
std::optional<CapellaForkStore> checked_vote(const CapellaForkStore& store,const Phase0SignedAttestation& vote,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 const auto& clock=store.timed.clock;if(!rules.slot_duration_ms||clock.time_ms<clock.genesis_time_ms)throw std::invalid_argument("Capella vote clock");
 const auto slot=(clock.time_ms-clock.genesis_time_ms)/rules.slot_duration_ms;
 if(vote.data.slot!=slot)return prepare_capella_store_vote(store,vote,rules,limits.replay_slots,limits.sync_trials,check);
 if(slot==UINT64_MAX||slot+1>(UINT64_MAX-clock.genesis_time_ms)/rules.slot_duration_ms)throw std::overflow_error("Capella deferred vote clock");
 const auto future=prepare_capella_store_tick(store,clock.genesis_time_ms+(slot+1)*rules.slot_duration_ms,rules,limits.replay_slots,limits.sync_trials,check);
 if(!prepare_capella_store_vote(future,vote,rules,limits.replay_slots,limits.sync_trials,check))return std::nullopt;
 return store;
}
std::string durable_snapshot(const CapellaForkStore& store,const CapellaSignedVotes& votes,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check,CapellaStoreSnapshotCache* cache=nullptr){
 const auto encoded=capella_store_snapshot(store,rules,limits,check,cache);if(votes.empty())return encoded;
 if(votes.size()>limits.messages)throw std::length_error("Capella signed vote capacity");Json list=Json::array();size_t total=0;
 for(const auto& [id,wire]:votes){if(check)check();if(wire.size()>limits.envelope_bytes/2-total)throw std::length_error("Capella signed vote bytes");total+=wire.size();list.push_back(Json::array({hex(id),hex(wire)}));}
 const auto result=Json{{"format","nano-capella-durable-v1"},{"store",Json::parse(encoded)},{"signedVotes",list}}.dump();
 if(result.size()>limits.envelope_bytes)throw std::length_error("Capella durable envelope");return result;
}
std::pair<CapellaForkStore,CapellaSignedVotes> restore_durable(std::string_view encoded,const Bytes& digest,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 if(check)check();archive_require(encoded.size()<=limits.envelope_bytes&&digest.size()==32&&sha256(Bytes(encoded.begin(),encoded.end()))==digest);
 const auto document=Json::parse(encoded);
 if(document.value("format",std::string())!="nano-capella-durable-v1")return {capella_store_from_snapshot(encoded,digest,rules,limits,check),{}};
 archive_require(document.size()==3&&document.at("signedVotes").is_array()&&!document.at("signedVotes").empty()&&document.at("signedVotes").size()<=limits.messages);
 const auto inner=document.at("store").dump();auto store=capella_store_from_snapshot(inner,sha256(Bytes(inner.begin(),inner.end())),rules,limits,check);CapellaSignedVotes votes;
 for(const auto& entry:document.at("signedVotes")){
  if(check)check();archive_require(entry.is_array()&&entry.size()==2&&entry[0].is_string()&&entry[1].is_string());
  const auto key=entry[0].get<std::string>(),payload=entry[1].get<std::string>();archive_require(key.size()==66&&payload.size()>=2&&payload.size()%2==0&&(payload.size()-2)/2<=228+uint64_t(rules.transition.consensus.body.validators_per_committee)/8+1);
  auto id=unhex(key),wire=unhex(payload);archive_require(hex(id)==key&&hex(wire)==payload&&sha256(wire)==id);
  const auto vote=decode_consensus_attestation_ssz(wire,rules.transition.consensus.body.validators_per_committee,check);archive_require(vote&&checked_vote(store,*vote,rules,limits,check).has_value());archive_require(votes.emplace(std::move(id),std::move(wire)).second);
 }
 archive_require(durable_snapshot(store,votes,rules,limits,check)==encoded);return {std::move(store),std::move(votes)};
}

}

void CapellaDurableReactor::require_available()const{if(unavailable_||!state_||!token_)throw std::runtime_error("Capella reactor unavailable; reopen required");}
void CapellaDurableReactor::publish(CapellaForkStore candidate,const std::function<void()>& check,const std::vector<CheckpointArchiveEntry>& archive,std::optional<CapellaSignedVotes> votes){
 if(unavailable_)throw std::runtime_error("Capella reactor unavailable; reopen required");
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 auto next=std::make_shared<const CapellaForkStore>(std::move(candidate));
 const auto encoded=durable_snapshot(*next,votes?*votes:votes_,rules_,limits_,check,&cache_);
 const auto& head=capella_fork_head(*next,rules_,check);
 const auto tip=disk_.latest();if(!tip)throw std::invalid_argument("Capella reactor requires persisted execution anchor");
 if(state_){
  if(tip->header.hash()!=capella_fork_head(*state_,rules_,check).execution_block.header.hash()){unavailable_=true;throw std::runtime_error("Capella execution tip changed outside reactor");}
  const auto& old=state_->timed.view;const auto& view=next->timed.view;
  if(view.anchor!=old.anchor||view.finalized.epoch<old.finalized.epoch||view.justified.epoch<old.justified.epoch||next->timed.clock.genesis_time_ms!=state_->timed.clock.genesis_time_ms||next->timed.clock.time_ms<state_->timed.clock.time_ms)throw std::invalid_argument("Capella checkpoint regression");
  auto at=consensus_beacon_header_root(head.header);const auto final_slot=state_->blocks.at(old.finalized.root).header.slot;
  while(next->blocks.at(at).header.slot>final_slot){if(check)check();at=next->blocks.at(at).header.parent_root;}
  if(at!=old.finalized.root||(view.finalized.epoch==old.finalized.epoch&&view.finalized.root!=old.finalized.root))throw std::invalid_argument("Capella finalized branch changed");
 }
 const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
 if(token_&&digest==token_->digest){if(check)check();try{if(!disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,token_))throw std::runtime_error("missing Capella checkpoint");}catch(...){unavailable_=true;throw;}return;}
 std::vector<ExecutionCheckpointBlock> suffix;auto at=consensus_beacon_header_root(head.header);Int common_height;Bytes common_hash;
 for(;;){
  if(check)check();const auto& record=next->blocks.at(at);const auto& h=record.execution_block.header;const auto existing=disk_.block_by_number(h.number);
  if(existing&&existing->header.hash()==h.hash()){common_height=h.number;common_hash=h.hash();break;}
  if(suffix.size()>=limits_.blocks)throw std::length_error("Capella publication suffix budget");suffix.push_back({record.execution_block,record.execution_state,record.receipts});
  if(at==next->timed.view.anchor){if(h.number<=0)throw std::invalid_argument("Capella execution anchor missing");const auto parent=disk_.block_by_number(h.number-1);if(!parent||parent->header.hash()!=h.parent_hash)throw std::invalid_argument("Capella execution anchor parent mismatch");common_height=h.number-1;common_hash=h.parent_hash;break;}
  at=record.header.parent_root;
 }
 std::reverse(suffix.begin(),suffix.end());if(check)check();
 ForkCheckpointToken committed;
 try{committed=disk_.commit_execution_checkpoint(chain_,genesis_,token_,tip->header.hash(),common_height,common_hash,suffix,encoded,limits_.envelope_bytes,limits_.blocks,archive);}
 catch(...){unavailable_=true;throw;}
 // No allocation or callback after durable success and before publication.
 token_=std::move(committed);state_=std::move(next);if(votes)votes_=std::move(*votes);
}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,CapellaForkStore initial,const std::function<void()>& check):disk_(disk),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits){publish(std::move(initial),check);}
CapellaDurableReactor::CapellaDurableReactor(ChainStore& disk,Int chain,Bytes genesis,CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& check):disk_(disk),chain_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits){
 if(chain_!=rules_.transition.chain_id||genesis_.size()!=32)throw std::invalid_argument("Capella reactor chain/profile mismatch");
 const auto record=disk_.fork_checkpoint(chain_,genesis_,limits_.envelope_bytes,expected);if(!record)throw std::invalid_argument("Capella restart checkpoint missing");
 auto [restored,votes]=restore_durable(record->snapshot,expected.digest,rules_,limits_,check);const auto& head=capella_fork_head(restored,rules_,check);const auto tip=disk_.latest();const auto state=disk_.state_at(head.execution_block.header.number);
 if(!tip||!state||tip->header.hash()!=head.execution_block.header.hash()||state->root()!=head.execution_state.root())throw std::invalid_argument("Capella restart execution mismatch");
 if(check)check();state_=std::make_shared<const CapellaForkStore>(std::move(restored));token_=expected;votes_=std::move(votes);
}
CapellaPublishedStore CapellaDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_};}
CapellaSignedVotes CapellaDurableReactor::signed_votes()const{std::lock_guard lock(mutex_);require_available();return votes_;}
void CapellaDurableReactor::tick(uint64_t time,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_capella_store_tick(*state_,time,rules_,limits_.replay_slots,limits_.sync_trials,check);auto votes=votes_;
 const auto previous=(state_->timed.clock.time_ms-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;
 const auto current=(time-state_->timed.clock.genesis_time_ms)/rules_.slot_duration_ms;const auto epoch=current/rules_.transition.consensus.epoch.history.slots_per_epoch;
 for(auto it=votes.begin();it!=votes.end();){
  if(check)check();const auto vote=decode_consensus_attestation_ssz(it->second,rules_.transition.consensus.body.validators_per_committee,check);if(!vote)throw std::logic_error("Capella stored vote encoding");
  if(current>previous&&vote->data.slot==previous){auto applied=prepare_capella_store_vote(next,*vote,rules_,limits_.replay_slots,limits_.sync_trials,check);if(applied)next=std::move(*applied);}
  if(vote->data.target.epoch<(epoch?epoch-1:0))it=votes.erase(it);else ++it;
 }
 publish(std::move(next),check,{},std::move(votes));
}

bool CapellaDurableReactor::block(std::span<const uint8_t> wire,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_capella_received_block(*state_,wire,rules_,limits_.replay_slots,limits_.sync_trials,check);if(!next)return false;
 std::vector<CheckpointArchiveEntry> archive;
 if(next->blocks.size()!=state_->blocks.size()){
  if(next->blocks.size()!=state_->blocks.size()+1)throw std::logic_error("Capella admission changed multiple blocks");
  for(const auto& [id,record]:next->blocks)if(!state_->blocks.contains(id))archive.push_back({Bytes(id.begin(),id.end()),record.header.slot,Bytes(wire.begin(),wire.end())});
  if(archive.size()!=1)throw std::logic_error("Capella admission missing new block");
 }
 // Duplicate roots are not reauthenticated by on_block: never backfill or
 // replace archive bytes through that fast path.
 publish(std::move(*next),check,archive);return true;
}
std::optional<Bytes> CapellaDurableReactor::signed_block(const ConsensusRoot& id,const std::function<void()>& check)const{
 std::lock_guard lock(mutex_);require_available();if(check)check();
 if(id==state_->timed.view.anchor||!state_->blocks.contains(id))return std::nullopt;
 const auto record=disk_.checkpoint_archive(Bytes(id.begin(),id.end()),limits_.envelope_bytes);if(!record)return std::nullopt;
 archive_require(record->slot==state_->blocks.at(id).header.slot);verify_archived_block(id,record->payload,*state_,rules_,check);return record->payload;
}

bool CapellaDurableReactor::vote(const Phase0SignedAttestation& vote,const std::function<void()>& check){
 std::lock_guard lock(mutex_);require_available();auto wire=encode_consensus_attestation_ssz(vote,rules_.transition.consensus.body.validators_per_committee,check);auto id=sha256(wire);
 if(const auto found=votes_.find(id);found!=votes_.end()&&found->second==wire)return true;
 auto next=checked_vote(*state_,vote,rules_,limits_,check);if(!next)return false;
 if(votes_.size()>=limits_.messages)throw std::length_error("Capella signed vote capacity");auto votes=votes_;votes.emplace(std::move(id),std::move(wire));publish(std::move(*next),check,{},std::move(votes));return true;
}

bool CapellaDurableReactor::slashing(const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,const std::function<void()>& check){std::lock_guard lock(mutex_);require_available();auto next=prepare_capella_store_slashing(*state_,first,second,rules_,check);if(!next)return false;publish(std::move(*next),check);return true;}
}
