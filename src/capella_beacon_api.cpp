#include "nano/beacon_api.hpp"
#include "nano/consensus_beacon_state.hpp"
#include <charconv>
#include "beacon_block_json.hpp"
namespace nano {
namespace {
uint64_t number(const Bytes& b,size_t at){uint64_t n=0;for(unsigned i=0;i<8;++i)n|=uint64_t(b.at(at+i))<<(8*i);return n;}
std::string octets(const Bytes& b,size_t at,size_t count){if(at>b.size()||count>b.size()-at)throw std::invalid_argument("Beacon field outside state");return hex(Bytes(b.begin()+at,b.begin()+at+count));}
}
BeaconReadResponse capella_beacon_read(const CapellaForkStore& store,
 const CapellaForkBlockRules& rules,std::string_view path,const std::function<void()>& checkpoint){
 if(path=="/eth/v1/node/version")return {200,{{"data",{{"version","Nano/0.1.0/c++20"}}}}};
 const bool genesis=path=="/eth/v1/beacon/genesis";
 const bool spec=path=="/eth/v1/config/spec";
 const std::string_view prefix="/eth/v1/beacon/states/";
 std::string_view selector,operation;
 if(path.starts_with(prefix)){
  const auto tail=path.substr(prefix.size());const auto slash=tail.find('/');
  if(slash!=std::string_view::npos){selector=tail.substr(0,slash);operation=tail.substr(slash+1);}
 }
 const bool root=operation=="root",fork=operation=="fork",finality=operation=="finality_checkpoints";
 if(!root&&!fork&&!finality&&!genesis&&!spec)return {404,{{"code",404},{"message","Beacon endpoint not implemented"}}};
 if(checkpoint)checkpoint();
 const CapellaForkBlock* selected=nullptr;
 if(root||fork||finality){
  if(selector=="head")selected=&capella_fork_head(store,rules,checkpoint);
  else{
   ConsensusRoot state_root{};
   try{
    if(!selector.starts_with("0x")||selector.size()!=66)throw std::invalid_argument("Invalid state root");
    const auto raw=unhex(std::string(selector));if(raw.size()!=32)throw std::invalid_argument("Invalid state root");
    std::copy(raw.begin(),raw.end(),state_root.begin());
   }catch(const std::invalid_argument&){return {400,{{"code",400},{"message","Expected head or 32-byte state root"}}};}
   // Only retained, authenticated post-states are served. Never fall back to
   // head when an immutable state was pruned or is unknown.
   for(const auto& [id,block]:store.blocks){if(checkpoint)checkpoint();if(block.header.state_root==state_root){selected=&block;break;}}
   if(!selected)return {404,{{"code",404},{"message","State root not retained"}}};
  }
 }else selected=&capella_fork_head(store,rules,checkpoint);
 const auto& head=*selected;
 const auto& b=head.state.ssz;const auto& l=rules.transition.consensus.state;
 if(capella_beacon_state_root(b,l,rules.transition.execution,rules.transition.maximum_state_bytes,checkpoint)!=head.header.state_root)
  throw std::invalid_argument("Beacon state commitment mismatch");
 const auto& c=rules.transition.consensus;
 const auto genesis_version=hex(Bytes(rules.transition.genesis_version.begin(),rules.transition.genesis_version.end()));
 if(genesis)return {200,{{"data",{{"genesis_time",std::to_string(number(b,0))},{"genesis_validators_root",octets(b,8,32)},{"genesis_fork_version",genesis_version}}}}};
 if(spec){
  // Export only parameters actually pinned by this native Capella profile.
  // Do not infer absent historical fork epochs or claim an Ethereum preset.
  const auto profile=capella_fork_profile(rules).dump();
  Json values={{"CONFIG_NAME","nano-capella-"+hex(sha256(Bytes(profile.begin(),profile.end()))).substr(2)},{"GENESIS_FORK_VERSION",genesis_version}};
  auto n=[&](const char* key,uint64_t value){values[key]=std::to_string(value);};
  n("SECONDS_PER_SLOT",rules.transition.seconds_per_slot);n("SLOT_DURATION_MS",rules.slot_duration_ms);
  n("SLOTS_PER_EPOCH",c.epoch.ffg.slots_per_epoch);n("PROPOSER_SCORE_BOOST",rules.proposer_score_boost);
  n("SLOTS_PER_HISTORICAL_ROOT",l.slots_per_historical_root);n("EPOCHS_PER_HISTORICAL_VECTOR",l.epochs_per_historical_vector);
  n("EPOCHS_PER_SLASHINGS_VECTOR",l.epochs_per_slashings_vector);n("HISTORICAL_ROOTS_LIMIT",l.historical_roots_limit);
  n("VALIDATOR_REGISTRY_LIMIT",l.validator_registry_limit);n("SYNC_COMMITTEE_SIZE",l.sync_committee_size);
  n("EPOCHS_PER_SYNC_COMMITTEE_PERIOD",c.epoch.sync.epochs_per_sync_committee_period);
  n("EPOCHS_PER_ETH1_VOTING_PERIOD",c.epoch.epochs_per_eth1_voting_period);
  n("MIN_SEED_LOOKAHEAD",c.attestation.duties.min_seed_lookahead);n("MAX_SEED_LOOKAHEAD",c.exit.max_seed_lookahead);
  n("TARGET_COMMITTEE_SIZE",c.attestation.duties.target_committee_size);n("MAX_COMMITTEES_PER_SLOT",c.attestation.duties.max_committees_per_slot);
  n("SHUFFLE_ROUND_COUNT",c.attestation.duties.shuffle_rounds);n("MAX_VALIDATORS_PER_COMMITTEE",c.body.validators_per_committee);
  n("EFFECTIVE_BALANCE_INCREMENT",c.deposit.effective_balance_increment);n("MAX_EFFECTIVE_BALANCE",c.deposit.maximum_effective_balance);
  n("EJECTION_BALANCE",c.epoch.registry.ejection_balance);n("MIN_PER_EPOCH_CHURN_LIMIT",c.exit.minimum_churn_limit);
  n("CHURN_LIMIT_QUOTIENT",c.exit.churn_limit_quotient);n("MIN_VALIDATOR_WITHDRAWABILITY_DELAY",c.exit.minimum_withdrawability_delay);
  n("SHARD_COMMITTEE_PERIOD",c.exit.shard_committee_period);n("HYSTERESIS_QUOTIENT",c.epoch.effective.hysteresis_quotient);
  n("HYSTERESIS_DOWNWARD_MULTIPLIER",c.epoch.effective.downward_multiplier);n("HYSTERESIS_UPWARD_MULTIPLIER",c.epoch.effective.upward_multiplier);
  n("BASE_REWARD_FACTOR",c.epoch.rewards.base_reward_factor);n("INACTIVITY_SCORE_BIAS",c.epoch.inactivity.score_bias);
  n("INACTIVITY_SCORE_RECOVERY_RATE",c.epoch.inactivity.score_recovery_rate);
  n("MIN_EPOCHS_TO_INACTIVITY_PENALTY",c.epoch.inactivity.minimum_epochs_to_inactivity_penalty);
  n("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX",c.epoch.rewards.inactivity_penalty_quotient);
  n("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX",c.epoch.slashing.minimum_penalty_quotient);
  n("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX",c.epoch.slashing.proportional_slashing_multiplier);
  n("WHISTLEBLOWER_REWARD_QUOTIENT",c.epoch.slashing.whistleblower_reward_quotient);
  n("TIMELY_SOURCE_WEIGHT",c.attestation.participation_weights[0]);n("TIMELY_TARGET_WEIGHT",c.attestation.participation_weights[1]);
  n("TIMELY_HEAD_WEIGHT",c.attestation.participation_weights[2]);n("PROPOSER_WEIGHT",c.attestation.proposer_weight);
  n("WEIGHT_DENOMINATOR",c.attestation.weight_denominator);n("SYNC_REWARD_WEIGHT",c.sync.sync_reward_weight);
  n("MIN_ATTESTATION_INCLUSION_DELAY",c.attestation.minimum_inclusion_delay);
  n("MAX_PROPOSER_SLASHINGS",c.body.proposer_slashings);n("MAX_ATTESTER_SLASHINGS",c.body.attester_slashings);
  n("MAX_ATTESTATIONS",c.body.attestations);n("MAX_DEPOSITS",c.body.deposits);n("MAX_VOLUNTARY_EXITS",c.body.voluntary_exits);
  n("MAX_EXTRA_DATA_BYTES",rules.transition.execution.extra_data_bytes);n("MAX_BYTES_PER_TRANSACTION",rules.transition.execution.transaction_bytes);
  n("MAX_TRANSACTIONS_PER_PAYLOAD",rules.transition.execution.transactions);
  n("MAX_WITHDRAWALS_PER_PAYLOAD",rules.transition.withdrawals.maximum_per_payload);n("MAX_VALIDATORS_PER_WITHDRAWALS_SWEEP",rules.transition.withdrawals.maximum_validators_per_sweep);n("MAX_BLS_TO_EXECUTION_CHANGES",rules.transition.maximum_bls_changes);
  if(checkpoint)checkpoint();return {200,{{"data",std::move(values)}}};
 }
 Json data;
 if(root)data={{"root",hex(Bytes(head.header.state_root.begin(),head.header.state_root.end()))}};
 if(fork)data={{"previous_version",octets(b,48,4)},{"current_version",octets(b,52,4)},{"epoch",std::to_string(number(b,56))}};
 if(finality){
  // Capella fixed section: participation offsets, justification bits, then
  // previous/current justified and finalized checkpoints. Read the actual head
  // state; the fork-choice Store can contain different pulled-up checkpoints.
  const size_t at=176+64*l.slots_per_historical_root+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+9;
  auto cp=[&](size_t p){return Json{{"epoch",std::to_string(number(b,p))},{"root",octets(b,p+8,32)}};};
  data={{"previous_justified",cp(at)},{"current_justified",cp(at+40)},{"finalized",cp(at+80)}};
 }
 if(checkpoint)checkpoint();
 const auto selected_id=consensus_beacon_header_root(head.header);auto tip=store.timed.view.finalized.root;bool is_finalized=false;
 for(size_t i=0;i<=store.blocks.size();++i){
  if(checkpoint)checkpoint();if(tip==selected_id){is_finalized=true;break;}
  const auto it=store.blocks.find(tip);if(it==store.blocks.end()||tip==store.timed.view.anchor)break;
  tip=it->second.header.parent_root;
 }
 return {200,{{"execution_optimistic",false},{"finalized",is_finalized},{"data",std::move(data)}}};
}
}

namespace nano {
BeaconReadResponse CapellaDurableReactor::beacon_read(std::string_view path,const std::function<void()>& check)const{
 std::lock_guard lock(mutex_);require_available();const auto& rules=rules_;const auto& store=*state_;
 const bool full=path.starts_with("/eth/v2/beacon/blocks/");
 const std::string_view prefix=full?"/eth/v2/beacon/blocks/":"/eth/v1/beacon/headers/";
 if(!path.starts_with(prefix))return capella_beacon_read(store,rules,path,check);
 auto error=[](unsigned code,const char* message){return BeaconReadResponse{code,{{"code",code},{"message",message}}};};
 const auto selector=path.substr(prefix.size());
 const auto& head=capella_fork_head(store,rules,check);const auto head_id=consensus_beacon_header_root(head.header);
 // Traversal stops at retained history. Never infer ancestry across missing
 // records or use execution block numbers as Beacon slots.
 auto ancestor=[&](ConsensusRoot tip,const ConsensusRoot& candidate){
  for(size_t i=0;i<=store.blocks.size();++i){
   if(check)check();if(tip==candidate)return true;
   const auto it=store.blocks.find(tip);if(it==store.blocks.end()||tip==store.timed.view.anchor)return false;
   tip=it->second.header.parent_root;
  }
  throw std::invalid_argument("Beacon ancestry cycle");
 };
 ConsensusRoot id{};
 if(selector=="head")id=head_id;
 else if(selector=="finalized")id=store.timed.view.finalized.root;
 else if(selector=="genesis")return error(404,"Genesis signed header not retained");
 else if(selector.starts_with("0x")){
  if(selector.size()!=66)return error(400,"Invalid block root");
  try{const auto raw=unhex(std::string(selector));if(raw.size()!=32)return error(400,"Invalid block root");std::copy(raw.begin(),raw.end(),id.begin());}catch(const std::invalid_argument&){return error(400,"Invalid block root");}
 }else{
  uint64_t slot=0;const auto parsed=std::from_chars(selector.data(),selector.data()+selector.size(),slot);
  if(selector.empty()||parsed.ec!=std::errc{}||parsed.ptr!=selector.data()+selector.size())return error(400,"Invalid block ID");
  auto tip=head_id;bool found=false;
  for(size_t i=0;i<=store.blocks.size();++i){
   if(check)check();const auto it=store.blocks.find(tip);if(it==store.blocks.end())break;
   if(it->second.header.slot==slot){id=tip;found=true;break;}
   if(it->second.header.slot<slot||tip==store.timed.view.anchor)break;
   tip=it->second.header.parent_root;
  }
  if(!found)return error(404,"Block slot not retained on canonical branch");
 }
 const auto block=store.blocks.find(id);if(block==store.blocks.end())return error(404,"Block not retained");
 const auto signed_block=signed_block_locked(id,check);if(!signed_block)return error(404,"Signed block not retained");
 if(full)return {200,{{"version","capella"},{"execution_optimistic",false},{"finalized",ancestor(store.timed.view.finalized.root,id)},{"data",beacon_block_json(*signed_block,rules,check)}},*signed_block,"capella"};
 const auto& h=block->second.header;auto root=[](const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));};
 Json message={{"slot",std::to_string(h.slot)},{"proposer_index",std::to_string(h.proposer_index)},
  {"parent_root",root(h.parent_root)},{"state_root",root(h.state_root)},{"body_root",root(h.body_root)}};
 if(check)check();
 return {200,{{"execution_optimistic",false},{"finalized",ancestor(store.timed.view.finalized.root,id)},
  {"data",{{"root",root(id)},{"canonical",ancestor(head_id,id)},
   {"header",{{"message",std::move(message)},{"signature",octets(*signed_block,4,96)}}}}}}};
}
}
