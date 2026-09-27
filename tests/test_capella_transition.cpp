#include "support/altair_transition_rules.hpp"
#include "nano/consensus_capella_transition.hpp"
#include "nano/consensus_capella_bls_change.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_duty.hpp"
#include "nano/committee.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_proposer.hpp"
#include "blst.h"
#include "../src/consensus_capella_projection.hpp"
#include "../src/consensus_bellatrix_projection.hpp"
using namespace test;using namespace test::phase0_minimal;
namespace {
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){for(size_t i=0;i<width;++i)b.at(at+i)=uint8_t(n>>(8*i));}
template<class A> void copy_to(A& dst,const Bytes& src){check(dst.size()==src.size(),"fixture width");std::copy(src.begin(),src.end(),dst.begin());}
CapellaExecutionPayload payload(const Block& block){const auto& h=block.header;CapellaExecutionPayload p;copy_to(p.parent_hash,h.parent_hash);copy_to(p.fee_recipient,unhex(h.coinbase));copy_to(p.state_root,h.state_root);copy_to(p.receipts_root,h.receipts_root);copy_to(p.logs_bloom,h.logs_bloom);copy_to(p.prev_randao,h.mix_hash);copy_to(p.block_hash,h.hash());auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());copy_to(p.base_fee_per_gas,fee);p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;for(const auto& tx:block.transactions)p.transactions.push_back(tx.raw());return p;}
struct Key {
 blst_scalar secret;Bytes public_key;
 explicit Key(uint64_t index){uint64_t words[4]{index+1,0,0,0};blst_scalar_from_uint64(&secret,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&secret);public_key.resize(48);blst_p1_compress(public_key.data(),&p);}
 Bytes sign(const ConsensusRoot& root)const{static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),32,reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
};
}
static std::string executable;
static bool sync_scenario=false;
static unsigned local_finality=0;
#ifdef NANO_SYNC_FAULT
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
#endif
static const CapellaStoreSnapshotLimits crash_limits{16*1024*1024,64,1000,1000,4*1024*1024,8*1024*1024,64,100000};
static int crash_writer(const fs::path& directory){
 const auto bundle=load(directory/"bundle.json");const auto profile=bundle.at("profile").get<std::string>();
 const auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1024*1024);
 const auto genesis=Block::from_json(bundle.at("genesis"));
 std::array<std::string,2> snapshots;std::array<std::vector<ExecutionCheckpointBlock>,2> suffixes;
 for(size_t i=0;i<2;++i){snapshots[i]=bundle.at("snapshots").at(i).get<std::string>();const auto store=capella_store_from_snapshot(snapshots[i],sha256(Bytes(snapshots[i].begin(),snapshots[i].end())),rules,crash_limits);std::map<Int,ExecutionCheckpointBlock> ordered;
  for(const auto& [id,b]:store.blocks)ordered.emplace(b.execution_block.header.number,ExecutionCheckpointBlock{b.execution_block,b.execution_state,b.receipts});for(const auto& [height,b]:ordered)suffixes[i].push_back(b);
 }
 ChainStore disk(directory/"capella");auto record=disk.fork_checkpoint(rules.transition.chain_id,genesis.header.hash(),crash_limits.envelope_bytes);check(bool(record),"writer checkpoint missing");
 std::cout<<"ready"<<std::endl;
 for(unsigned i=0;i<128;++i){const auto choice=disk.latest()->header.number==1?1:0;const auto token=disk.commit_execution_checkpoint(rules.transition.chain_id,genesis.header.hash(),record->token,disk.latest()->header.hash(),0,genesis.header.hash(),suffixes[choice],snapshots[choice],crash_limits.envelope_bytes,64);record->token=token;std::cout<<token.sequence<<std::endl;}
 return 0;
}
#ifdef NANO_SYNC_FAULT
static int sync_writer(const fs::path& directory){
 const auto bundle=load(directory/"bundle.json");const auto profile=bundle.at("profile").get<std::string>();
 const auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1024*1024);
 const auto genesis=Block::from_json(bundle.at("genesis"));const auto chain=rules.transition.chain_id;
 ChainStore disk(directory/"db");const auto initial=disk.fork_checkpoint(chain,genesis.header.hash(),crash_limits.envelope_bytes);check(bool(initial),"sync checkpoint missing");
 CapellaDurableReactor reactor(disk,chain,genesis.header.hash(),rules,crash_limits,initial->token);
 reactor.tick(bundle.at("time").get<uint64_t>());const auto before=reactor.current();const auto wire=unhex(bundle.at("wire").get<std::string>());
 const auto next=prepare_capella_received_block(*before.state,wire,rules,crash_limits.replay_slots,crash_limits.sync_trials);check(bool(next),"sync candidate rejected");
 save(directory/"expected.json",Json{{"old",capella_store_snapshot(*before.state,rules,crash_limits)},{"new",capella_store_snapshot(*next,rules,crash_limits)},{"sequence",before.token.sequence}});
 fs::path wal;for(const auto& entry:fs::directory_iterator(directory/"db"))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"sync WAL missing");const auto offset=fs::file_size(wal);
 nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{reactor.block(wire);}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures()>0,"Capella sync fault not reached");
 unsigned blocked=0;auto refuse=[&](auto action){try{action();}catch(const std::runtime_error&){++blocked;}};
 refuse([&]{reactor.current();});refuse([&]{reactor.tick(bundle.at("time").get<uint64_t>());});refuse([&]{reactor.block(wire);});refuse([&]{disk.latest();});refuse([&]{disk.fork_checkpoint(chain,genesis.header.hash(),crash_limits.envelope_bytes);});
 check(blocked==5,"uncertain Capella state still accessible");
 std::cout<<Json{{"wal",wal.filename().string()},{"before",offset},{"after",fs::file_size(wal)},{"blocked",blocked}}.dump()<<std::endl;_exit(0);
}
#endif
static void local_finality_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 Temp directory;const auto limits=crash_limits;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;const auto genesis_root=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
 std::array<std::unique_ptr<ChainStore>,2> disks;std::array<std::unique_ptr<CapellaDurableReactor>,2> nodes;
 for(size_t i=0;i<2;++i){disks[i]=std::make_unique<ChainStore>(directory/("node-"+std::to_string(i)));disks[i]->bind_chain(t.chain_id,genesis.header.hash(),"capella-local-loop");disks[i]->commit(genesis,genesis_state,{});nodes[i]=std::make_unique<CapellaDurableReactor>(*disks[i],t.chain_id,genesis.header.hash(),rules,limits,initial);}
 std::map<uint64_t,std::unique_ptr<ChainStore>> key_disks;std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>> journals;
 auto journal=[&](uint64_t index)->BeaconSigningJournal&{if(!journals.contains(index)){key_disks[index]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(index)));journals[index]=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots);}return *journals.at(index);};
 const auto sender_key=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;const auto sender=SignedTransaction::sign(tx,sender_key).sender();
 size_t blocks=0,votes=0,restarts=0,reopened_journals=0;uint64_t previous_finalized=0;const uint64_t end=5*slots+1;
 auto equal=[&]{const auto a=nodes[0]->current(),b=nodes[1]->current();check(a.token==b.token&&capella_store_snapshot(*a.state,rules,limits)==capella_store_snapshot(*b.state,rules,limits)&&nodes[0]->signed_votes()==nodes[1]->signed_votes(),"local loop nodes diverged");const auto& head=capella_fork_head(*a.state,rules);for(size_t i=0;i<2;++i)check(disks[i]->latest()->header.hash()==head.execution_block.header.hash()&&disks[i]->state_at(head.execution_block.header.number)->root()==head.execution_state.root(),"local loop disk/state diverged");};
 for(uint64_t slot=2;slot<=end;++slot){
  const auto now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;for(auto& node:nodes)node->tick(now);
  const auto current=nodes[0]->current();const auto& head=capella_fork_head(*current.state,rules);const auto proposer=prepare_capella_proposer_identity(head.state,head.header,slot,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,t.consensus.attestation.duties,limits.replay_slots,limits.sync_trials);check(bool(proposer),"local loop proposer missing");
  auto provider=[&](const ConsensusRoot& root){return Key(proposer->index).sign(root);};auto& proposer_journal=journal(proposer->index);
  const auto reveal=nodes[0]->sign_randao(proposer->index,slot,[&]{return now;},current.token,proposer_journal,provider);check(bool(reveal),"local loop RANDAO missing");
  tx.nonce=head.execution_state.get(sender).nonce;const std::vector<SignedTransaction> pending{SignedTransaction::sign(tx,sender_key)};
  const auto body=nodes[0]->prepare_local_body(proposer->index,slot,*reveal,head.execution_block.header.coinbase,current.token,pending,16,64);check(bool(body),"local loop body missing");
  const auto wire=nodes[0]->sign_proposal(proposer->index,slot,*body,[&]{return now;},current.token,proposer_journal,provider);check(bool(wire),"local loop signed block missing");
  for(auto& node:nodes)check(node->block(*wire),"local loop block admission failed");++blocks;
  const auto admitted=nodes[0]->current();const auto duties=prepare_capella_attestation_duties(*admitted.state,rules,slot,limits.replay_slots,limits.sync_trials);
  for(const auto& [index,duty]:duties){if(local_finality==50&&index%2)continue;
   const auto vote=nodes[0]->sign_attestation(index,slot,[&]{return now;},nodes[0]->current().token,journal(index),[&](const ConsensusRoot& root){return Key(index).sign(root);});check(bool(vote),"local loop attestation missing");for(auto& node:nodes)check(node->vote(*vote),"local loop gossip vote rejected");++votes;
  }
  const auto finalized=nodes[0]->current().state->timed.view.finalized.epoch;check(finalized>=previous_finalized,"local loop finality regressed");previous_finalized=finalized;
  if(slot%slots==0||slot==end){equal();std::cout<<Json{{"progressSlot",slot},{"finalizedEpoch",finalized},{"signedVotes",votes}}.dump()<<std::endl;}
  if(slot==3*slots){
   for(size_t i=0;i<2;++i){const auto saved=nodes[i]->current().token;nodes[i].reset();disks[i].reset();disks[i]=std::make_unique<ChainStore>(directory/("node-"+std::to_string(i)));nodes[i]=std::make_unique<CapellaDurableReactor>(*disks[i],t.chain_id,genesis.header.hash(),rules,limits,saved);++restarts;}
   reopened_journals=journals.size();for(auto& [index,value]:journals){const auto saved=value->commitment();value.reset();key_disks[index].reset();key_disks[index]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(index)));value=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots,saved);}
   equal();
  }
 }
 const auto result=nodes[0]->current();const auto& head=capella_fork_head(*result.state,rules);check(head.execution_state.get(sender).nonce==Int(std::to_string(blocks+1)),"local loop execution transactions missing");
 if(local_finality==100)check(result.state->timed.view.finalized.epoch>=2&&result.state->timed.view.justified.epoch>=3,"assembled local chain did not finalize");else check(result.state->timed.view.finalized.epoch==0,"half-participation local chain finalized");
 std::cout<<Json{{"scenario","capella-local-assembly-finality"},{"participationPercent",local_finality},{"blocks",blocks},{"votes",votes},{"finalizedEpoch",result.state->timed.view.finalized.epoch},{"justifiedEpoch",result.state->timed.view.justified.epoch},{"independentStores",2},{"storeReopens",restarts},{"signingJournalsReopened",reopened_journals},{"networkTransport",false},{"networkActivated",false}}.dump()<<std::endl;
}
static void scenario(bool migrate,uint64_t skipped=0,bool finality=false,unsigned participation=100){
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",refs=fixtures/"consensus-bellatrix-containers-official",source=fixtures/"consensus-capella-slots-official";
 const auto manifest=load(source/"manifest.json");check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f","initial state revision");
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="slots_1")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(source/"slots_1"/path))).substr(2)==info.at("sha256").get<std::string>(),"initial state digest");
 auto common=altair_transition_rules(fixtures);const auto preset=read(refs/"minimal-bellatrix.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 const auto container_manifest=load(refs/"manifest.json");
 for(const auto& row:container_manifest.at("presets"))check(hex(sha256(binary(refs/row.at("name").get<std::string>()))).substr(2)==row.at("sha256").get<std::string>(),"Bellatrix preset digest");
 common.epoch.rewards.inactivity_penalty_quotient=p("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.minimum_penalty_quotient=p("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.proportional_slashing_multiplier=p("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX");
 const auto clock_file=fixtures/"consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml";
 const auto clock_manifest=load(fixtures/"consensus-altair-fork-head-official/manifest.json");
 check(hex(sha256(binary(clock_file))).substr(2)==clock_manifest.at("referenceFiles").at("minimal-config-v1.6.0-beta.0.yaml").at("sha256").get<std::string>(),"clock config digest");
 const auto seconds=parameter(read(clock_file),"SECONDS_PER_SLOT");

 const auto cp=read(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml");
 const CapellaTransitionRules r{common,{uint32_t(p("MAX_EXTRA_DATA_BYTES")),uint32_t(p("MAX_BYTES_PER_TRANSACTION")),uint32_t(p("MAX_TRANSACTIONS_PER_PAYLOAD"))},4*1024*1024,seconds,{common.deposit.maximum_effective_balance,parameter(cp,"MAX_WITHDRAWALS_PER_PAYLOAD"),parameter(cp,"MAX_VALIDATORS_PER_WITHDRAWALS_SWEEP")},common.deposit.genesis_fork_version,uint32_t(parameter(cp,"MAX_BLS_TO_EXECUTION_CHANGES")),1337,ReplayPolicy::RequireChainId};
 const auto& l=r.consensus.state;const auto& duties=r.consensus.attestation.duties;const uint64_t slots=duties.slots_per_epoch;
 const size_t hist=176+64*l.slots_per_historical_root,randao=hist+96,field=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;
 CapellaSlotState beacon{binary(source/"slots_1/pre.ssz"),std::nullopt};put(beacon.ssz,field+12,48);const auto genesis_time=little(beacon.ssz,0,8);const auto genesis_root=root_at(beacon.ssz,8);
 Key withdrawal_key(1000);auto credentials=sha256(withdrawal_key.public_key);credentials[0]=0;const auto validators_at=little(beacon.ssz,hist+88,4),balances_at=little(beacon.ssz,hist+92,4);
 std::copy(credentials.begin(),credentials.end(),beacon.ssz.begin()+validators_at+48);put(beacon.ssz,balances_at,r.withdrawals.maximum_effective_balance+1000000000);
 SignedCapellaBlsChange change;change.validator_index=0;copy_to(change.from_bls_pubkey,withdrawal_key.public_key);change.to_execution_address[19]=42;
 copy_to(change.signature,withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},r.genesis_version,genesis_root))));
 const auto key=bytes(Int(777),32);const std::string target="0x0000000000000000000000000000000000001234";Transaction tx;tx.type=2;tx.to=target;tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;
 State state;state.get(SignedTransaction::sign(tx,key).sender()).balance=Int("1000000000000000000");state.get(target).code=unhex("602a5f5500");
 Block parent;parent.header.difficulty=0;parent.header.timestamp=Int(std::to_string(genesis_time));parent.header.state_root=state.root();if(!migrate)parent.header.withdrawals_root=withdrawals_root(Json::array());
 const auto initial_header=encode_capella_execution_header(capella_execution_payload_header(payload(parent),r.execution,r.withdrawals.maximum_per_payload),r.execution.extra_data_bytes,r.maximum_state_bytes);
 beacon.ssz.resize(little(beacon.ssz,field,4));beacon.ssz.insert(beacon.ssz.end(),initial_header.begin(),initial_header.end());put(beacon.ssz,field+20,beacon.ssz.size(),4);
 BellatrixSlotState before_fork;ConsensusRoot before_fork_root{};
 CapellaForkSchedule schedule{1,{2,0,0,1},{3,0,0,1}};
 if(migrate){
  auto projection=capella_detail::project(beacon.ssz,l,r.execution,r.maximum_state_bytes,{});
  std::copy(schedule.bellatrix_version.begin(),schedule.bellatrix_version.end(),projection.common.begin()+52);put(projection.common,56,0);
  const auto header=encode_bellatrix_execution_header(static_cast<const BellatrixExecutionPayloadHeader&>(capella_execution_payload_header(payload(parent),r.execution,r.withdrawals.maximum_per_payload)),r.execution.extra_data_bytes,r.maximum_state_bytes);
  before_fork.ssz=bellatrix_detail::restore(projection.common,header,l,r.execution,r.maximum_state_bytes,{});
  before_fork_root=bellatrix_beacon_state_root(before_fork.ssz,l,r.execution,r.maximum_state_bytes);
 }
 const uint64_t first_slot=migrate?slots+skipped:1;
 std::map<Int,Bytes> hashes{{0,parent.header.hash()}};std::function<Bytes(const Int&)> history=[&](const Int& n){return hashes.at(n);};
 const auto disk_genesis=parent;const auto disk_genesis_state=state;
 const CapellaForkBlockRules store_rules{r,seconds*1000,seconds*1000/3,40};
 Temp reactor_database;auto reactor_disk=std::make_unique<ChainStore>(reactor_database/"db");reactor_disk->bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-reactor-test");reactor_disk->commit(disk_genesis,disk_genesis_state,{});std::unique_ptr<CapellaDurableReactor> reactor;
 Temp proposal_location;std::map<uint64_t,std::unique_ptr<ChainStore>> proposal_disks;std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>> proposal_journals;
 auto journal_for=[&](uint64_t index)->BeaconSigningJournal&{if(!proposal_journals.contains(index)){proposal_disks[index]=std::make_unique<ChainStore>(proposal_location/std::to_string(index));proposal_journals[index]=std::make_unique<BeaconSigningJournal>(*proposal_disks[index],r.chain_id,disk_genesis.header.hash(),genesis_root,Key(index).public_key,slots);}return *proposal_journals.at(index);};
 unsigned assembled_bodies=0;unsigned local_proposals=0,local_randao=0;unsigned local_signatures=0;std::vector<Phase0SignedAttestation> deferred_votes;std::map<ConsensusRoot,Bytes> signed_wires;std::optional<CapellaForkStore> store;std::string short_snapshot;Bytes sync_wire;uint64_t sync_time=0;
 const CapellaStoreSnapshotLimits snapshot_limits{16*1024*1024,64,1000,1000,r.maximum_state_bytes,8*1024*1024,64,100000};
 auto roundtrip=[&](const CapellaForkStore& value){
  CapellaStoreSnapshotCache cache;size_t cold=0,warm=0;
  const auto encoded=capella_store_snapshot(value,store_rules,snapshot_limits,[&]{++cold;},&cache);
  check(encoded==capella_store_snapshot(value,store_rules,snapshot_limits,[&]{++warm;},&cache)&&warm<cold,"snapshot cache changed bytes or did not reuse records");
  const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));
  const auto restored=capella_store_from_snapshot(encoded,digest,store_rules,snapshot_limits);
  check(capella_store_snapshot(restored,store_rules,snapshot_limits)==encoded,"Capella snapshot roundtrip differs");
  auto wrong=digest;wrong[0]^=1;bool refused=false;try{capella_store_from_snapshot(encoded,wrong,store_rules,snapshot_limits);}catch(const std::invalid_argument&){refused=true;}check(refused,"snapshot digest ignored");
  auto changed_rules=store_rules;++changed_rules.transition.withdrawals.maximum_validators_per_sweep;refused=false;try{capella_store_from_snapshot(encoded,digest,changed_rules,snapshot_limits);}catch(const std::invalid_argument&){refused=true;}check(refused,"snapshot withdrawal rules ignored");
  auto corrupt=value;corrupt.blocks.begin()->second.execution_state.get(target).balance+=1;refused=false;try{capella_store_snapshot(corrupt,store_rules,snapshot_limits,{},&cache);}catch(const std::invalid_argument&){refused=true;}check(refused,"cache masked mutated execution state");
  return encoded;
 };

 std::vector<Bytes> pending_attestations;uint64_t included_attestations=0;
 unsigned accepted=0,rejected=0,cancelled=0,contexts=0,votes=0,vote_reorgs=0;uint64_t paid=0;
 const uint64_t last_slot=finality?4*slots+1:first_slot+slots;
 for(uint64_t slot=first_slot;slot<=last_slot;++slot){
  const bool crossing=migrate&&slot==first_slot;
  const auto original=beacon;const auto evm_before=state.root();
  const auto advanced=crossing?prepare_capella_fork_slots(before_fork,before_fork_root,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l,schedule,slot,64,100000):prepare_capella_slots(beacon,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l,slot,64,100000);
  const auto epoch=slot/slots;ConsensusRandaoState randomness;randomness.epoch=epoch;randomness.last_reveal_slot=little(advanced.ssz,64,8);
  for(size_t i=0;i<l.epochs_per_historical_vector;++i)randomness.mixes.push_back(root_at(advanced.ssz,randao+32*i));
  const auto validators=little(advanced.ssz,hist+88,4),balances=little(advanced.ssz,hist+92,4);std::vector<Phase0ProposerValidator> registry;
  for(size_t i=0;i<(balances-validators)/121;++i){const auto at=validators+121*i;registry.push_back({i,little(advanced.ssz,at+80,8),little(advanced.ssz,at+97,8),little(advanced.ssz,at+105,8)});}
  check(registry.size()==64,"expected 64 validators");const auto proposer=phase0_proposer_index(registry,randomness,{slots,duties.min_seed_lookahead},slot,duties.maximum_effective_balance,duties.shuffle_rounds);
  Key signing(proposer);check(std::equal(signing.public_key.begin(),signing.public_key.end(),advanced.ssz.begin()+validators+121*proposer),"fixture BLS key mismatch");
  ConsensusBytes4 version{};const size_t version_at=epoch<little(advanced.ssz,56,8)?48:52;std::copy_n(advanced.ssz.begin()+version_at,4,version.begin());
  ConsensusRoot epoch_root{};for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
  const auto reveal=signing.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,genesis_root)));
  const ConsensusBeaconHeader previous{little(advanced.ssz,64,8),little(advanced.ssz,72,8),root_at(advanced.ssz,80),root_at(advanced.ssz,112),root_at(advanced.ssz,144)};
  const auto parent_root=consensus_beacon_header_root(previous);
  Block execution;execution.header.parent_hash=parent.header.hash();execution.header.number=parent.header.number+1;execution.header.timestamp=Int(std::to_string(genesis_time+slot*seconds));execution.header.difficulty=0;execution.header.base_fee=next_base_fee(parent.header);execution.header.mix_hash=Bytes(randomness.mixes[epoch%l.epochs_per_historical_vector].begin(),randomness.mixes[epoch%l.epochs_per_historical_vector].end());
  tx.nonce=Int(std::to_string(slot-first_slot));execution.transactions.push_back(SignedTransaction::sign(tx,key));

  CapellaWithdrawalState withdrawal_state;withdrawal_state.registry.epoch=epoch;withdrawal_state.next_withdrawal_index=little(advanced.ssz,field+4,8);withdrawal_state.next_validator_index=little(advanced.ssz,field+12,8);
  for(size_t i=0;i<registry.size();++i){ConsensusSlashingValidator v;const auto at=validators+121*i;v.balance=little(advanced.ssz,balances+8*i,8);v.effective_balance=little(advanced.ssz,at+80,8);v.withdrawable_epoch=little(advanced.ssz,at+113,8);std::copy_n(advanced.ssz.begin()+at+48,32,v.withdrawal_credentials.begin());withdrawal_state.registry.validators.emplace(i,v);}
  const auto withdrawals=expected_capella_withdrawals(withdrawal_state,r.withdrawals);for(const auto& w:withdrawals)execution.withdrawals.push_back(w.to_json());
  if(slot==first_slot)check(withdrawals.empty(),"credentials paid before change");if(slot==first_slot+(migrate?4:1))check(!withdrawals.empty(),"credentials not paid at expected sweep");paid+=withdrawals.size();
  State expected=state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(expected,execution,false);
  auto ep=payload(execution);ep.withdrawals=withdrawals;const auto encoded=encode_capella_execution_payload(ep,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);
  const size_t sync_bytes=(l.sync_committee_size+7)/8+96,fixed=228+sync_bytes;
  Bytes body(fixed,0);std::copy(reveal.begin(),reveal.end(),body.begin());std::copy_n(advanced.ssz.begin()+hist+4,72,body.begin()+96);body[220+(l.sync_committee_size+7)/8]=0xc0;
  for(size_t at:std::array<size_t,6>{200,204,208,212,216,220+sync_bytes})put(body,at,fixed,4);put(body,224+sync_bytes,fixed+encoded.size(),4);body.insert(body.end(),encoded.begin(),encoded.end());if(slot==first_slot){const auto wire=encode_capella_bls_change(change);body.insert(body.end(),wire.begin(),wire.end());}
  if(finality&&!pending_attestations.empty()){
   Bytes list(4*pending_attestations.size());for(size_t i=0;i<pending_attestations.size();++i){put(list,4*i,list.size(),4);list.insert(list.end(),pending_attestations[i].begin(),pending_attestations[i].end());}
   body.resize(fixed);for(size_t at:std::array<size_t,3>{212,216,220+sync_bytes})put(body,at,fixed+list.size(),4);put(body,224+sync_bytes,fixed+list.size()+encoded.size(),4);body.insert(body.end(),list.begin(),list.end());body.insert(body.end(),encoded.begin(),encoded.end());included_attestations+=pending_attestations.size();
  }
  const auto built=crossing?prepare_capella_fork_unsigned_block(before_fork,before_fork_root,schedule,slot,proposer,parent_root,body,r,parent.header,state,history,64,100000):prepare_capella_unsigned_block(beacon,slot,proposer,parent_root,body,r,parent.header,state,history,64,100000);check(built.has_value(),"Capella unsigned block rejected");
  auto sign_message=[&](Bytes message,uint64_t index=UINT64_MAX){const auto owner=index==UINT64_MAX?proposer:index;Key k(owner);const auto bytes=std::span<const uint8_t>(message);const ConsensusBeaconHeader h{little(message,0,8),little(message,8,8),root_at(message,16),root_at(message,48),capella_beacon_body_root(bytes.subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)};const auto sig=k.sign(consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},version,genesis_root)));Bytes wire(100,0);put(wire,0,100,4);std::copy(sig.begin(),sig.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());return wire;};

  auto receive=[&](const Bytes& b,const std::function<void()>& callback=std::function<void()>{}){
   return crossing?prepare_capella_fork_state_transition(before_fork,before_fork_root,schedule,b,r,parent.header,state,history,64,100000,callback):prepare_capella_state_transition(beacon,b,r,parent.header,state,history,64,100000,callback);
  };
  auto wire=sign_message(built->message_ssz);
  if(reactor){
   const auto now=(genesis_time+slot*seconds)*1000;reactor->tick(now);const auto current=reactor->current();auto& journal=journal_for(proposer);unsigned calls=0;auto provider=[&](const ConsensusRoot& root){++calls;return Key(proposer).sign(root);};
   check(!reactor->sign_randao(UINT64_MAX,slot,[&]{return now;},current.token,journal,provider)&&calls==0,"unassigned RANDAO signer invoked");
   const auto reveal=reactor->sign_randao(proposer,slot,[&]{return now;},current.token,journal,provider);check(reveal&&*reveal==Bytes(body.begin(),body.begin()+96)&&calls==1,"local RANDAO differs");++local_randao;
   if(slot==first_slot+1){
    auto stale=current.token;++stale.sequence;bool rejected=false;try{reactor->sign_proposal(proposer,slot,body,[&]{return now;},stale,journal,provider);}catch(const StaleCapellaDuty&){rejected=true;}check(rejected&&calls==1,"stale proposal reached signer");
    auto invalid=body;invalid[0]^=1;check(!reactor->sign_proposal(proposer,slot,invalid,[&]{return now;},current.token,journal,provider)&&calls==1,"invalid RANDAO body reached signer");
    bool cancelled=false;try{reactor->sign_proposal(proposer,slot,body,[&]{return now;},current.token,journal,provider,[]{throw ExecutionCancelled("proposal stop");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&calls==1,"cancelled proposal reached signer");
   }
   if(!finality){const auto automatic=reactor->prepare_local_body(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64);check(automatic&&*automatic==body,"assembled body differs from independent construction");body=*automatic;++assembled_bodies;}
   const auto local=reactor->sign_proposal(proposer,slot,body,[&]{return now;},current.token,journal,provider);check(local&&*local==wire&&calls==2,"local Capella proposal differs");wire=*local;++local_proposals;
   if(slot==first_slot+1){
    const auto saved=journal.commitment();proposal_journals.erase(proposer);proposal_disks.erase(proposer);proposal_disks[proposer]=std::make_unique<ChainStore>(proposal_location/std::to_string(proposer));proposal_journals[proposer]=std::make_unique<BeaconSigningJournal>(*proposal_disks[proposer],r.chain_id,disk_genesis.header.hash(),genesis_root,Key(proposer).public_key,slots,saved);auto& reopened=*proposal_journals.at(proposer);
    check(reactor->sign_proposal(proposer,slot,body,[&]{return now;},current.token,reopened,provider)==local&&reopened.commitment()==saved&&calls==3,"proposal retry changed reservation");
    auto conflict=body;conflict[168]^=1;bool rejected=false;try{reactor->sign_proposal(proposer,slot,conflict,[&]{return now;},current.token,reopened,provider);}catch(const std::runtime_error&){rejected=true;}check(rejected&&calls==3,"double proposal reached signer");
    Temp expiry;ChainStore disk(expiry/"journal");BeaconSigningJournal expiring(disk,r.chain_id,disk_genesis.header.hash(),genesis_root,Key(proposer).public_key,slots);auto wall=now;bool expired=false;
    try{reactor->sign_proposal(proposer,slot,body,[&]{return wall;},current.token,expiring,[&](const ConsensusRoot& root){wall+=store_rules.slot_duration_ms;return Key(proposer).sign(root);});}catch(const ExpiredCapellaDuty&){expired=true;}check(expired&&expiring.latest_proposal_slot()==slot,"expired proposal lost reservation");
    rejected=false;try{reactor->sign_proposal(proposer,slot,conflict,[&]{return now;},current.token,expiring,provider);}catch(const std::runtime_error&){rejected=true;}check(rejected&&calls==3,"expired proposal allowed conflict");
   }
  }
  if(slot==first_slot+1){sync_wire=wire;sync_time=(genesis_time+slot*seconds)*1000;}size_t calls=0;const auto received=receive(wire,[&]{++calls;});
  check(received&&received->consensus==built->post.consensus&&received->execution.state.root()==expected.root(),"signed Capella transition differs");check(received->execution.receipts[0].success&&received->execution.state.get_storage(target,0)==42,"Shanghai write missing");
  auto deny=[&](const Bytes& b){bool refused=false;try{refused=!receive(b);}catch(const std::invalid_argument&){refused=true;}check(refused&&beacon==original&&state.root()==evm_before,"bad block mutated parent");++rejected;};
  if(crossing){
   auto rejected_context=[&](ConsensusRoot root,CapellaForkSchedule fork,uint64_t budget){bool refused=false;
    try{refused=!prepare_capella_fork_state_transition(before_fork,root,fork,wire,r,parent.header,state,history,budget,100000);}
    catch(const std::invalid_argument&){refused=true;}catch(const std::length_error&){refused=true;}
    check(refused&&state.root()==evm_before&&bellatrix_beacon_state_root(before_fork.ssz,l,r.execution,r.maximum_state_bytes)==before_fork_root,"invalid scheduled admission");++rejected;
   };
   auto wrong=before_fork_root;wrong[0]^=1;rejected_context(wrong,schedule,64);
   auto future=schedule;++future.epoch;rejected_context(before_fork_root,future,64);
   rejected_context(before_fork_root,schedule,slot-1);
   const auto current_version=version;version=schedule.bellatrix_version;auto old_signature=sign_message(built->message_ssz);version=current_version;deny(old_signature);
  }
  auto bad=wire;bad[4]^=1;deny(bad);auto message=built->message_ssz;message[48]^=1;deny(sign_message(message));
  if(slot==first_slot){message=built->message_ssz;message.back()^=1;deny(sign_message(message));}
  if(slot==first_slot+(migrate?4:1)){message=built->message_ssz;const auto duplicate=encode_capella_bls_change(change);message.insert(message.end(),duplicate.begin(),duplicate.end());deny(sign_message(message));}
  for(size_t stop:{size_t(1),calls}){size_t n=0;bool stopped=false;try{receive(wire,[&]{if(++n==stop)throw ExecutionCancelled("stop");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped&&beacon==original&&state.root()==evm_before,"cancelled block changed parents");++cancelled;}
  const ConsensusBeaconHeader admitted{slot,proposer,parent_root,root_at(built->message_ssz,48),capella_beacon_body_root(std::span<const uint8_t>(body),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)};
  const CapellaEpochRules epoch_rules{r.consensus.epoch,r.execution,r.maximum_state_bytes};
  const auto context=capella_fork_block_context(received->consensus,admitted,epoch_rules,l);
  const auto& validator=context.registry.validators.at(0);
  check(context.block_root==consensus_beacon_header_root(admitted)&&validator.balance==little(received->consensus.ssz,little(received->consensus.ssz,hist+92,4),8)&&validator.withdrawal_credentials[0]==1,"Capella registry context lost withdrawal or credentials");
  const auto checkpoints=capella_fork_checkpoints(received->consensus,epoch_rules,l);
  const size_t checkpoint_fields=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
  check(checkpoints.realized.current_justified.epoch==little(received->consensus.ssz,checkpoint_fields+49,8)&&checkpoints.realized.finalized.root==root_at(received->consensus.ssz,checkpoint_fields+97),"Capella checkpoints differ");
  auto wrong_header=admitted;wrong_header.state_root[0]^=1;bool refused=false;try{capella_fork_block_context(received->consensus,wrong_header,epoch_rules,l);}catch(const std::invalid_argument&){refused=true;}check(refused,"unbound Capella context accepted");
  if(slot==first_slot||slot==first_slot+slots){
   const ConsensusCheckpoint target_checkpoint{(slot+slots-1)/slots,consensus_beacon_header_root(admitted)};
   const auto projected=prepare_capella_fork_state_contexts(received->consensus,admitted,target_checkpoint,epoch_rules,l,slots,100000);
   check(little(projected.target_state.ssz,40,8)==target_checkpoint.epoch*slots&&projected.target.registry.epoch==target_checkpoint.epoch,"target context uses wrong epoch");
   const auto selected=prepare_capella_proposer_identity(received->consensus,admitted,slot+1,epoch_rules,l,duties,slots,100000);
   check(selected&&selected->fork_version==version&&selected->genesis_root==genesis_root,"proposer fork context differs");
   check(!prepare_capella_proposer_identity(received->consensus,admitted,slot,epoch_rules,l,duties,slots,100000),"past proposer duty accepted");
   if(slot==first_slot){
    auto slashed=received->consensus;const auto validator_start=little(slashed.ssz,hist+88,4);slashed.ssz.at(validator_start+121*selected->index+88)=1;
    auto slashed_header=admitted;slashed_header.state_root=capella_beacon_state_root(slashed.ssz,l,r.execution,r.maximum_state_bytes);
    check(!prepare_capella_proposer_identity(slashed,slashed_header,slot+1,epoch_rules,l,duties,slots,100000),"slashed proposer rerolled or accepted");
    bool interrupted=false;try{capella_fork_checkpoints(received->consensus,epoch_rules,l,[]{throw ExecutionCancelled("context stop");});}catch(const ExecutionCancelled&){interrupted=true;}check(interrupted,"context cancellation swallowed");
   }

  }
  if(!store){
   store=initialize_capella_fork_store(received->consensus,built->message_ssz,received->execution.block,received->execution.state,received->execution.receipts,{parent.header},store_rules);
   reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,*store);
  }else{
   auto timed=prepare_capella_store_tick(*store,(genesis_time+slot*seconds)*1000,store_rules,64,100000);
   const auto admitted_store=prepare_capella_received_block(timed,wire,store_rules,64,100000);check(admitted_store.has_value(),"Capella Store rejected signed chain");
   check(admitted_store->blocks.size()==store->blocks.size()+1,"Capella Store block missing");
   const auto& head=capella_fork_head(*admitted_store,store_rules);
   check(head.state==received->consensus&&head.execution_state.root()==expected.root(),"Store selected unpaired head");
   if(!head.execution_block.withdrawals.empty()){
    auto corrupted=head;corrupted.execution_block.withdrawals[0]["amount"]="0xffff";bool refused=false;
    try{validate_capella_execution_record(corrupted,store_rules);}catch(const std::invalid_argument&){refused=true;}check(refused,"Store withdrawal body not bound");
   }
   bool cancelled_store=false;try{prepare_capella_received_block(timed,wire,store_rules,64,100000,[]{throw ExecutionCancelled("store stop");});}catch(const ExecutionCancelled&){cancelled_store=true;}check(cancelled_store&&timed.blocks.size()==store->blocks.size(),"Store cancellation leaked publication");
   auto bad_signature=wire;bad_signature[4]^=1;check(!prepare_capella_received_block(timed,bad_signature,store_rules,64,100000),"Store admitted bad signature");
   reactor->tick((genesis_time+slot*seconds)*1000);const auto before_publish=reactor->current();
   bool reactor_cancelled=false;try{reactor->block(wire,[]{throw ExecutionCancelled("reactor stop");});}catch(const ExecutionCancelled&){reactor_cancelled=true;}check(reactor_cancelled&&reactor->current().token==before_publish.token,"cancelled reactor published");
   check(reactor->block(wire),"durable Capella block rejected");check(capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(*admitted_store,store_rules,snapshot_limits),"durable Store differs");
   const auto accepted_token=reactor->current().token;check(reactor->block(wire)&&reactor->current().token==accepted_token,"duplicate block rewrote checkpoint");
   const auto admitted_id=consensus_beacon_header_root(admitted);signed_wires.emplace(admitted_id,wire);
   check(reactor->signed_block(admitted_id)==std::optional<Bytes>(wire),"Capella signed archive differs");
   check(reactor->block(bad_signature)&&reactor->current().token==accepted_token&&reactor->signed_block(admitted_id)==std::optional<Bytes>(wire),"duplicate signature replaced archive");
   store=*admitted_store;
   if(!finality&&slot==first_slot+slots){
    auto alternate_execution=execution;auto alternative_tx=tx;alternative_tx.value=1;alternate_execution.transactions[0]=SignedTransaction::sign(alternative_tx,key);
    State alternate_state=state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(alternate_state,alternate_execution,false);
    auto alternative_payload=payload(alternate_execution);alternative_payload.withdrawals=withdrawals;const auto alternative_encoded=encode_capella_execution_payload(alternative_payload,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);
    auto alternate_body=body;alternate_body.resize(fixed);put(alternate_body,224+sync_bytes,fixed+alternative_encoded.size(),4);alternate_body.insert(alternate_body.end(),alternative_encoded.begin(),alternative_encoded.end());

    const auto alternate=prepare_capella_unsigned_block(beacon,slot,proposer,parent_root,alternate_body,r,parent.header,state,history,64,100000);check(alternate.has_value(),"alternate construction failed");
    const auto branch=prepare_capella_received_block(*store,sign_message(alternate->message_ssz),store_rules,64,100000);check(branch&&branch->blocks.size()==store->blocks.size()+1,"sibling branch lost");
    const auto& branch_head=capella_fork_head(*branch,store_rules);check(branch_head.execution_state.root()==expected.root()||branch_head.execution_state.root()==alternate_state.root(),"sibling head execution mismatch");
    check(reactor->block(sign_message(alternate->message_ssz)),"durable alternate block rejected");for(const auto& [id,b]:branch->blocks)if(!store->blocks.contains(id))signed_wires.emplace(id,sign_message(alternate->message_ssz));reactor->tick((genesis_time+(slot+1)*seconds)*1000);
    auto voting=prepare_capella_store_tick(*branch,(genesis_time+(slot+1)*seconds)*1000,store_rules,64,100000);
    const auto canonical=consensus_beacon_header_root(admitted);
    const ConsensusBeaconHeader alternate_header{slot,proposer,parent_root,root_at(alternate->message_ssz,48),capella_beacon_body_root(alternate_body,r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)};
    const auto alternate_root=consensus_beacon_header_root(alternate_header);
    auto vote_for=[&](size_t position,const ConsensusRoot& head){
     auto target_root=head;while(voting.blocks.at(target_root).header.slot>epoch*slots)target_root=voting.blocks.at(target_root).header.parent_root;
     const auto& target_record=voting.blocks.at(target_root);const ConsensusCheckpoint target_checkpoint{epoch,target_root};
     const auto context=prepare_capella_fork_state_contexts(target_record.state,target_record.header,target_checkpoint,epoch_rules,l,64,100000);
     const auto assignment=phase0_attestation_duties(context.target.registry,context.target.randao,duties,epoch);
     const auto first_committee=partition_committee(assignment.shuffled,(slot%slots)*assignment.committees_per_slot,assignment.committees_per_slot*slots);check(!first_committee.empty(),"empty voting committee");
     const auto committee_index=position/first_committee.size();const auto members=partition_committee(assignment.shuffled,(slot%slots)*assignment.committees_per_slot+committee_index,assignment.committees_per_slot*slots);position%=first_committee.size();check(position<members.size(),"insufficient voting members");
     Phase0SignedAttestation vote{{slot,committee_index,head,{},target_checkpoint},std::vector<bool>(members.size()),{}};vote.aggregation_bits[position]=true;Key voter(members[position]);
     vote.signature=voter.sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},context.target.fork.current_version,context.target.genesis_root)));
     auto invalid=vote;invalid.signature[0]^=1;check(!prepare_capella_store_vote(voting,invalid,store_rules,64,100000),"invalid Capella vote signature accepted");
     const auto next=prepare_capella_store_vote(voting,vote,store_rules,64,100000);check(next.has_value(),"Capella branch vote rejected");voting=*next;check(reactor->vote(vote)&&capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(voting,store_rules,snapshot_limits),"durable Capella vote differs");++votes;return std::pair{members[position],vote};
    };
    const auto first_vote=vote_for(0,canonical);check(capella_fork_head(voting,store_rules).execution_state.root()==expected.root(),"canonical vote head mismatch");
    vote_for(1,alternate_root);vote_for(2,alternate_root);check(capella_fork_head(voting,store_rules).execution_state.root()==alternate_state.root(),"votes did not select alternate EVM state");++vote_reorgs;
    vote_for(3,canonical);vote_for(4,canonical);vote_for(5,canonical);check(capella_fork_head(voting,store_rules).execution_state.root()==expected.root(),"votes did not restore canonical EVM state");++vote_reorgs;roundtrip(voting);
    auto conflicting=first_vote.second;conflicting.data.beacon_block_root=alternate_root;if(slot%slots==0)conflicting.data.target.root=alternate_root;
    conflicting.signature=Key(first_vote.first).sign(consensus_signing_root(consensus_attestation_root(conflicting.data),consensus_domain({1,0,0,0},version,genesis_root)));
    const auto slashed=prepare_capella_store_slashing(voting,{first_vote.second.data,{first_vote.first},first_vote.second.signature},{conflicting.data,{first_vote.first},conflicting.signature},store_rules);
    const IndexedConsensusAttestation proof_a{first_vote.second.data,{first_vote.first},first_vote.second.signature},proof_b{conflicting.data,{first_vote.first},conflicting.signature};
    const auto before_proof=reactor->current().token;auto bad_proof=proof_b;bad_proof.signature[0]^=1;check(!reactor->slashing(proof_a,bad_proof)&&reactor->current().token==before_proof&&reactor->signed_slashings().empty(),"invalid slashing persisted");
    bool proof_cancelled=false;try{reactor->slashing(proof_a,proof_b,[]{throw ExecutionCancelled("proof stop");});}catch(const ExecutionCancelled&){proof_cancelled=true;}check(proof_cancelled&&reactor->current().token==before_proof,"cancelled slashing persisted");
    check(slashed&&slashed->timed.view.equivocating_indices.contains(first_vote.first),"Capella equivocation evidence rejected");roundtrip(*slashed);check(reactor->slashing({first_vote.second.data,{first_vote.first},first_vote.second.signature},{conflicting.data,{first_vote.first},conflicting.signature})&&capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(*slashed,store_rules,snapshot_limits),"durable Capella slashing differs");
    const auto proof_token=reactor->current().token;check(reactor->signed_slashings().size()==1,"missing slashing archive");
    check(reactor->slashing(proof_a,proof_b)&&reactor->slashing(proof_b,proof_a)&&reactor->current().token==proof_token,"duplicate/reversed slashing rewrote checkpoint");
    for(unsigned variant=1;variant<=4;++variant){auto alternative=proof_b;alternative.data.beacon_block_root[0]^=uint8_t(variant);alternative.signature=Key(first_vote.first).sign(consensus_signing_root(consensus_attestation_root(alternative.data),consensus_domain({1,0,0,0},version,genesis_root)));check(reactor->slashing(proof_a,alternative)&&reactor->signed_slashings().size()==1&&reactor->current().token==proof_token,"equivalent offender grew archive");}
    roundtrip(*branch);
    const auto duplicate=prepare_capella_received_block(*branch,wire,store_rules,64,100000);check(duplicate&&duplicate->blocks.size()==branch->blocks.size(),"duplicate changed Store cardinality");
   }
  }
  if(finality){
   pending_attestations.clear();ConsensusRandaoState post_randao;post_randao.epoch=epoch;post_randao.last_reveal_slot=slot;
   for(size_t i=0;i<l.epochs_per_historical_vector;++i)post_randao.mixes.push_back(root_at(received->consensus.ssz,randao+32*i));
   const auto assignment=phase0_attestation_duties(context.registry,post_randao,duties,epoch);
   const auto target_root=slot%slots==0?consensus_beacon_header_root(admitted):root_at(received->consensus.ssz,176+32*((epoch*slots)%l.slots_per_historical_root));
   for(uint64_t committee_index=0;committee_index<assignment.committees_per_slot;++committee_index){
    const auto members=partition_committee(assignment.shuffled,(slot%slots)*assignment.committees_per_slot+committee_index,assignment.committees_per_slot*slots);
    for(size_t position=0;position<members.size()&&position*100<members.size()*participation;++position){Phase0SignedAttestation vote{{slot,committee_index,consensus_beacon_header_root(admitted),checkpoints.realized.current_justified,{epoch,target_root}},std::vector<bool>(members.size()),{}};vote.aggregation_bits[position]=true;
     vote.signature=Key(members[position]).sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},version,genesis_root)));
     pending_attestations.push_back(encode_consensus_attestation_ssz(vote,r.consensus.body.validators_per_committee));
     if(slot==4*slots+1){
      const auto current=reactor->current();const auto now=current.state->timed.clock.time_ms;const auto index=members[position];Key local_key(index);Temp signing_dir;ForkCheckpointToken signed_token;unsigned provider_calls=0;
      const auto duty=prepare_capella_attestation_duty(*current.state,store_rules,index,slot,64,100000);check(duty&&duty->attestation.data==vote.data&&duty->attestation.aggregation_bits==vote.aggregation_bits,"derived Capella duty differs");
      auto excluded=*current.state;excluded.timed.view.equivocating_indices.insert(index);check(!prepare_capella_attestation_duty(excluded,store_rules,index,slot,64,100000),"equivocator received local duty");
      check(!prepare_capella_attestation_duty(*current.state,store_rules,index,slot+1,64,100000),"future local duty accepted");
      auto provider=[&](const ConsensusRoot& root){++provider_calls;return local_key.sign(root);};
      {ChainStore disk(signing_dir/"journal");BeaconSigningJournal journal(disk,r.chain_id,disk_genesis.header.hash(),genesis_root,local_key.public_key,slots);
       auto stale=current.token;++stale.sequence;bool rejected=false;try{reactor->sign_attestation(index,slot,[&]{return now;},stale,journal,provider);}catch(const StaleCapellaDuty&){rejected=true;}check(rejected&&provider_calls==0,"stale duty reached signer");
       check(!reactor->sign_attestation(UINT64_MAX,slot,[&]{return now;},current.token,journal,provider)&&provider_calls==0,"unassigned validator signed");
       const auto signed_vote=reactor->sign_attestation(index,slot,[&]{return now;},current.token,journal,provider);check(signed_vote&&signed_vote->data==vote.data&&signed_vote->signature==vote.signature&&provider_calls==1,"local signed attestation differs");signed_token=journal.commitment();++local_signatures;
      }
      {ChainStore disk(signing_dir/"journal");BeaconSigningJournal journal(disk,r.chain_id,disk_genesis.header.hash(),genesis_root,local_key.public_key,slots,signed_token);
       const auto retry=reactor->sign_attestation(index,slot,[&]{return now;},current.token,journal,provider);check(retry&&retry->signature==vote.signature&&provider_calls==2&&journal.commitment()==signed_token,"reopened signer changed attestation or reservation");
       auto conflict=vote.data;conflict.beacon_block_root[0]^=1;bool rejected=false;try{journal.sign_attestation(conflict,version,provider);}catch(const std::exception&){rejected=true;}check(rejected&&provider_calls==2,"reopened journal allowed double vote");
      }
      if(position==0&&committee_index==0){
       {ChainStore disk(signing_dir/"wrong-domain");BeaconSigningJournal journal(disk,r.chain_id+1,disk_genesis.header.hash(),genesis_root,local_key.public_key,slots);bool rejected=false;try{reactor->sign_attestation(index,slot,[&]{return now;},current.token,journal,provider);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&provider_calls==2,"foreign journal reached signer");}
       {ChainStore disk(signing_dir/"expired");BeaconSigningJournal journal(disk,r.chain_id,disk_genesis.header.hash(),genesis_root,local_key.public_key,slots);auto wall=now;bool expired=false;try{reactor->sign_attestation(index,slot,[&]{return wall;},current.token,journal,[&](const ConsensusRoot& root){wall+=store_rules.slot_duration_ms;return local_key.sign(root);});}catch(const ExpiredCapellaDuty&){expired=true;}check(expired,"expired provider result escaped");auto conflict=vote.data;conflict.beacon_block_root[0]^=1;bool rejected=false;try{journal.sign_attestation(conflict,version,provider);}catch(const std::exception&){rejected=true;}check(rejected&&provider_calls==2,"expired duty lost reservation");}
      }

      const auto before=reactor->current();auto bad=vote;bad.signature[0]^=1;check(!reactor->vote(bad)&&reactor->current().token==before.token,"invalid deferred vote persisted");
      bool stopped=false;try{reactor->vote(vote,[]{throw ExecutionCancelled("vote stop");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped&&reactor->current().token==before.token,"cancelled deferred vote persisted");
      check(reactor->vote(vote)&&capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(*before.state,store_rules,snapshot_limits),"current-slot vote changed fork choice");
      const auto accepted=reactor->current().token;check(accepted!=before.token&&reactor->vote(vote)&&reactor->current().token==accepted,"deferred duplicate rewrote checkpoint");deferred_votes.push_back(vote);
     }
    }
   }
  }
  if(slot==first_slot){short_snapshot=capella_store_snapshot(*store,store_rules,snapshot_limits);if(local_finality){local_finality_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}}
  ++contexts;
  beacon=received->consensus;state=received->execution.state;parent=received->execution.block;hashes[parent.header.number]=parent.header.hash();++accepted;
 }
 if(finality){if(participation==100)check(store->timed.view.finalized.epoch>=2&&store->timed.view.justified.epoch>=3&&included_attestations>=128,"Capella participation did not advance finality");else check(store->timed.view.finalized.epoch==0&&store->timed.view.justified.epoch==0,"insufficient participation finalized a checkpoint");}
 const auto durable_envelope=reactor_disk->fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes)->snapshot;const auto durable_proofs=reactor->signed_slashings();const auto durable_votes=reactor->signed_votes();const auto durable=reactor->current();const auto durable_bytes=capella_store_snapshot(*durable.state,store_rules,snapshot_limits);reactor.reset();reactor_disk.reset();reactor_disk=std::make_unique<ChainStore>(reactor_database/"db");
 reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,durable.token);
 check(capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==durable_bytes&&reactor_disk->latest()->header.hash()==capella_fork_head(*durable.state,store_rules).execution_block.header.hash(),"reactor restart diverged");
 check(reactor->signed_votes()==durable_votes,"signed votes lost on restart");check(reactor->signed_slashings()==durable_proofs,"signed slashings lost on restart");
 if(!deferred_votes.empty()){
  const auto next_time=durable.state->timed.clock.time_ms+store_rules.slot_duration_ms;auto expected=prepare_capella_store_tick(*durable.state,next_time,store_rules,64,100000);
  for(const auto& vote:deferred_votes){auto applied=prepare_capella_store_vote(expected,vote,store_rules,64,100000);check(bool(applied),"deferred expected vote rejected");expected=std::move(*applied);}
  reactor->tick(next_time);check(capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(expected,store_rules,snapshot_limits),"restored deferred votes not applied on next slot");
  reactor->tick(next_time+3*slots*store_rules.slot_duration_ms);check(reactor->signed_votes().empty(),"expired vote archive retained");
 }
 for(const auto& [id,wire]:signed_wires)check(reactor->signed_block(id)==std::optional<Bytes>(wire),"Capella archive lost on restart or reorg");
 check(!reactor->signed_block(durable.state->timed.view.anchor)&&!reactor->signed_block(ConsensusRoot{}),"unsigned anchor or unknown block fabricated");
 bool archive_cancelled=false;try{reactor->signed_block(signed_wires.begin()->first,[]{throw ExecutionCancelled("archive stop");});}catch(const ExecutionCancelled&){archive_cancelled=true;}check(archive_cancelled,"archive read cancellation swallowed");
 {
  const auto initial=reactor->current();const auto& previous=capella_fork_head(*initial.state,store_rules);const auto next_slot=std::max(previous.header.slot+1,(initial.state->timed.clock.time_ms-initial.state->timed.clock.genesis_time_ms)/store_rules.slot_duration_ms);const auto now=initial.state->timed.clock.genesis_time_ms+next_slot*store_rules.slot_duration_ms;
  reactor->tick(now);const auto current=reactor->current();const auto& head=capella_fork_head(*current.state,store_rules);
  const auto identity=prepare_capella_proposer_identity(head.state,head.header,next_slot,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l,duties,64,100000);check(bool(identity),"automatic child proposer missing");auto& journal=journal_for(identity->index);auto signer=[&](const ConsensusRoot& root){return Key(identity->index).sign(root);};
  const auto reveal=reactor->sign_randao(identity->index,next_slot,[&]{return now;},current.token,journal,signer);check(bool(reveal),"automatic child RANDAO missing");
  const auto sender=SignedTransaction::sign(tx,key).sender();auto next_tx=tx;const auto nonce=head.execution_state.get(sender).nonce;next_tx.nonce=nonce;auto future=next_tx;future.nonce+=1;auto invalid=next_tx;invalid.max_fee=0;invalid.priority_fee=0;
  const std::vector<SignedTransaction> pending{SignedTransaction::sign(future,key),SignedTransaction::sign(invalid,key),SignedTransaction::sign(next_tx,key)};
  const auto before=current.token;bool cancelled=false;try{reactor->prepare_local_body(identity->index,next_slot,*reveal,parent.header.coinbase,before,pending,3,64,[]{throw ExecutionCancelled("body stop");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&reactor->current().token==before,"body cancellation changed checkpoint");
  const auto automatic=reactor->prepare_local_body(identity->index,next_slot,*reveal,parent.header.coinbase,before,pending,3,64);check(bool(automatic),"automatic child body missing");
  const auto signed_child=reactor->sign_proposal(identity->index,next_slot,*automatic,[&]{return now;},before,journal,signer);check(signed_child&&reactor->block(*signed_child),"automatic child admission failed");++assembled_bodies;
  const auto after=reactor->current();const auto& child=capella_fork_head(*after.state,store_rules);check(child.execution_block.transactions.size()==2&&child.execution_block.transactions[0].tx.nonce==nonce&&child.execution_block.transactions[1].tx.nonce==nonce+1&&child.execution_state.get(sender).nonce==nonce+2,"automatic nonce selection failed");
  const auto context=capella_fork_block_context(child.state,child.header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l);
  for(const auto& [id,wire]:durable_proofs){const auto proof=decode_consensus_attester_slashing_ssz(wire,r.consensus.body.validators_per_committee);check(bool(proof),"automatic proof decode");for(auto index:proof->first.indices)if(std::binary_search(proof->second.indices.begin(),proof->second.indices.end(),index))check(context.registry.validators.at(index).slashed,"automatic body omitted slashable proof");}
  const auto child_root=child.execution_state.root();reactor.reset();reactor_disk.reset();reactor_disk=std::make_unique<ChainStore>(reactor_database/"db");reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,after.token);check(capella_fork_head(*reactor->current().state,store_rules).execution_state.root()==child_root,"automatic child restart differs");
 }
 const auto snapshot=roundtrip(*store);const auto profile=capella_fork_profile(store_rules).dump();const auto profile_digest=sha256(Bytes(profile.begin(),profile.end()));
 auto reject_profile=[&](const std::string& value,const Bytes& digest,size_t budget){bool refused=false;try{load_capella_fork_profile(value,digest,budget);}catch(const std::exception&){refused=true;}check(refused,"invalid Capella profile accepted");};
 auto wrong_digest=profile_digest;wrong_digest[0]^=1;reject_profile(profile,wrong_digest,1024*1024);reject_profile(profile,profile_digest,profile.size()-1);
 for(const auto& field:{std::string("genesisVersion"),std::string("maximumBlsChanges")}){auto changed=Json::parse(profile);changed[field]=field=="genesisVersion"?"0xffffffff":"0x0";const auto text=changed.dump();reject_profile(text,sha256(Bytes(text.begin(),text.end())),1024*1024);}
 const auto restored_rules=load_capella_fork_profile(profile,profile_digest,1024*1024);check(capella_fork_profile(restored_rules)==capella_fork_profile(store_rules),"Capella profile roundtrip differs");
#ifdef NANO_SYNC_FAULT
 if(sync_scenario){
  for(bool lose:{false,true}){Temp tmp;
   {ChainStore disk(tmp/"db");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-sync-test");disk.commit(disk_genesis,disk_genesis_state,{});auto first=capella_store_from_snapshot(short_snapshot,sha256(Bytes(short_snapshot.begin(),short_snapshot.end())),store_rules,snapshot_limits);CapellaDurableReactor initial(disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,std::move(first));}
   save(tmp/"bundle.json",Json{{"profile",profile},{"genesis",disk_genesis.to_json(true)},{"wire",hex(sync_wire)},{"time",sync_time}});
   const auto result=success(run({executable,"--sync-writer",tmp.path.string()}));const auto wal=tmp/"db"/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"sync WAL suffix missing");
   if(lose){const int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned WAL");const bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate owned WAL");}
   const auto expected=load(tmp/"expected.json");ChainStore disk(tmp/"db");const auto record=disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes);
   check(record&&record->snapshot==expected.at(lose?"old":"new").get<std::string>()&&record->token.sequence==expected.at("sequence").get<uint64_t>()+(lose?0:1),"mixed Capella recovery disposition");
   // The test resolves both known outcomes; production must authenticate its restart token independently.
   CapellaDurableReactor restored(disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,record->token);const auto recovered=restored.current();const auto& head=capella_fork_head(*recovered.state,store_rules);const auto latest=disk.latest();
   check(latest&&latest->header.hash()==head.execution_block.header.hash()&&latest->withdrawals==head.execution_block.withdrawals&&disk.state_at(latest->header.number)->root()==head.execution_state.root()&&disk.count()==latest->header.number+1,"Capella recovery paired state mismatch");
   for(const auto& [id,b]:recovered.state->blocks)for(const auto& tx:b.execution_block.transactions)check(disk.receipt(tx.hash())&&disk.receipt(tx.hash())->block_hash==b.execution_block.header.hash()&&disk.transaction(tx.hash()),"Capella recovered transaction index missing");
   const auto candidate_bytes=expected.at("new").get<std::string>();const auto candidate=capella_store_from_snapshot(candidate_bytes,sha256(Bytes(candidate_bytes.begin(),candidate_bytes.end())),store_rules,snapshot_limits);const auto& next_head=capella_fork_head(candidate,store_rules);
   const auto next_id=consensus_beacon_header_root(next_head.header);check(bool(disk.checkpoint_archive(Bytes(next_id.begin(),next_id.end()),snapshot_limits.envelope_bytes))==!lose,"Capella archive detached from WAL outcome");
   check(bool(disk.block_by_number(next_head.execution_block.header.number))==!lose,"lost Capella block remained indexed");for(const auto& tx:next_head.execution_block.transactions)check(bool(disk.receipt(tx.hash()))==!lose&&bool(disk.transaction(tx.hash()))==!lose,"lost Capella transaction remained indexed");
   check(restored.block(sync_wire),"Capella replay after recovery rejected");const auto completed=restored.current();check(restored.signed_block(next_id)==std::optional<Bytes>(sync_wire),"Capella replay archive differs");check(capella_store_snapshot(*completed.state,store_rules,snapshot_limits)==expected.at("new").get<std::string>(),"Capella recovery replay differs");
   std::cout<<Json{{"scenario","capella-reactor-sync"},{"lostSuffix",lose},{"blockedOperations",result.at("blocked")},{"replay",true},{"physicalPowerLoss",false},{"networkActivated",false}}.dump()<<'\n';
  }
  return;
 }
#endif
 Temp database;ForkCheckpointToken token;std::map<Int,ExecutionCheckpointBlock> ordered;
 for(const auto& [id,record]:store->blocks)ordered.emplace(record.execution_block.header.number,ExecutionCheckpointBlock{record.execution_block,record.execution_state,record.receipts});
 std::vector<ExecutionCheckpointBlock> suffix;for(const auto& [number,record]:ordered)suffix.push_back(record);
 if(finality&&participation==100){
  for(unsigned mutation=0;mutation<3;++mutation){Temp tmp;ChainStore disk(tmp/"db");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-vote-restore");disk.commit(disk_genesis,disk_genesis_state,{});auto envelope=Json::parse(durable_envelope);
   if(mutation==1){auto wire=unhex(envelope["signedVotes"][0][1].get<std::string>());wire[132]^=1;envelope["signedVotes"][0]=Json::array({hex(sha256(wire)),hex(wire)});}
   if(mutation==2)envelope["signedVotes"].push_back(envelope["signedVotes"][0]);
   const auto token=disk.commit_execution_checkpoint(r.chain_id,disk_genesis.header.hash(),{},disk_genesis.header.hash(),0,disk_genesis.header.hash(),suffix,envelope.dump(),snapshot_limits.envelope_bytes,64);
   bool refused=false;try{CapellaDurableReactor restored(disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,token);check(restored.signed_votes()==durable_votes,"restored control votes differ");}catch(const std::invalid_argument&){refused=true;}
   check(refused==(mutation!=0),"tampered vote envelope admission wrong");
  }
 }
 if(!finality){
  for(unsigned mutation=0;mutation<4;++mutation){Temp tmp;ChainStore disk(tmp/"db");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-proof-restore");disk.commit(disk_genesis,disk_genesis_state,{});auto envelope=Json::parse(durable_envelope);
   if(mutation==1){const auto wire=unhex(envelope["signedSlashings"][0][1].get<std::string>());auto decoded=decode_consensus_attester_slashing_ssz(wire,r.consensus.body.validators_per_committee);check(bool(decoded),"proof fixture encoding");decoded->first.signature[0]^=1;const auto changed=encode_consensus_attester_slashing_ssz(decoded->first,decoded->second,r.consensus.body.validators_per_committee);envelope["signedSlashings"][0]=Json::array({hex(sha256(changed)),hex(changed)});}
   if(mutation==2)envelope["signedSlashings"].push_back(envelope["signedSlashings"][0]);
   if(mutation==3)envelope["store"]["equivocations"]=Json::array();
   const auto saved=disk.commit_execution_checkpoint(r.chain_id,disk_genesis.header.hash(),{},disk_genesis.header.hash(),0,disk_genesis.header.hash(),suffix,envelope.dump(),snapshot_limits.envelope_bytes,64);
   bool refused=false;try{CapellaDurableReactor restored(disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,saved);check(restored.signed_slashings()==durable_proofs,"proof restore control differs");}catch(const std::invalid_argument&){refused=true;}
   check(refused==(mutation!=0),"tampered slashing envelope admission wrong");
  }
 }
 if(!finality){
  const auto& head=capella_fork_head(*store,store_rules);const auto id=consensus_beacon_header_root(head.header);
  for(unsigned mutation=0;mutation<3;++mutation){Temp corrupt;ChainStore disk(corrupt/"db");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-archive-negative");disk.commit(disk_genesis,disk_genesis_state,{});
   auto wire=signed_wires.at(id);auto slot=head.header.slot;if(mutation==0)wire[4]^=1;else if(mutation==1)wire.back()^=1;else ++slot;
   const auto saved=disk.commit_execution_checkpoint(r.chain_id,disk_genesis.header.hash(),{},disk_genesis.header.hash(),0,disk_genesis.header.hash(),suffix,snapshot,snapshot_limits.envelope_bytes,64,{{Bytes(id.begin(),id.end()),slot,wire}});
   CapellaDurableReactor restored(disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,saved);bool refused=false;try{restored.signed_block(id);}catch(const std::invalid_argument&){refused=true;}check(refused,"corrupt signed archive accepted");
  }
 }
 {ChainStore disk(database/"capella");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-store-test");disk.commit(disk_genesis,disk_genesis_state,{});token=disk.commit_execution_checkpoint(r.chain_id,disk_genesis.header.hash(),{},disk_genesis.header.hash(),0,disk_genesis.header.hash(),suffix,snapshot,snapshot_limits.envelope_bytes,64);}
 {ChainStore disk(database/"capella");const auto record=disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes);check(record&&record->token==token,"Capella checkpoint token changed");const auto restored=capella_store_from_snapshot(record->snapshot,token.digest,restored_rules,snapshot_limits);const auto& head=capella_fork_head(restored,restored_rules);check(disk.latest()->header.hash()==head.execution_block.header.hash()&&disk.state_at(head.execution_block.header.number)->root()==head.execution_state.root(),"reopened Capella consensus and execution diverged");}
 save(database/"bundle.json",Json{{"profile",profile},{"genesis",disk_genesis.to_json(true)},{"snapshots",Json::array({short_snapshot,snapshot})}});
 for(unsigned kill=0;kill<2;++kill){
  Process writer({executable,"--crash-writer",database.path.string()});writer.line([](const auto& line){return line=="ready";});
  if(kill)writer.line([](const auto& line){return !line.empty()&&line[0]>='0'&&line[0]<='9';});
  writer.signal(SIGKILL);const auto result=writer.wait(20);check(result.code==-SIGKILL,"Capella writer not killed");
  uint64_t acknowledged=token.sequence;std::istringstream lines(result.out);std::string line;while(std::getline(lines,line))if(!line.empty()&&line[0]>='0'&&line[0]<='9')acknowledged=std::max(acknowledged,uint64_t(std::stoull(line)));
  ChainStore disk(database/"capella");const auto record=disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes);check(record&&record->token.sequence>=acknowledged,"acknowledged Capella commit lost");
  check(record->snapshot==short_snapshot||record->snapshot==snapshot,"partial Capella snapshot recovered");
  const auto restored=capella_store_from_snapshot(record->snapshot,record->token.digest,restored_rules,snapshot_limits);const auto& head=capella_fork_head(restored,restored_rules);const auto latest=disk.latest();
  check(latest&&latest->header.hash()==head.execution_block.header.hash()&&latest->withdrawals==head.execution_block.withdrawals&&disk.state_at(latest->header.number)->root()==head.execution_state.root()&&disk.count()==latest->header.number+1,"mixed Capella crash recovery");
  for(const auto& [height,b]:ordered){const bool canonical=height<=latest->header.number;check(bool(disk.block_by_number(height))==canonical,"stale block after crash reorg");for(const auto& tx:b.block.transactions)check(bool(disk.receipt(tx.hash()))==canonical&&bool(disk.transaction(tx.hash()))==canonical,"stale transaction after crash reorg");}
  token=record->token;
 }
 std::cout<<Json{{"scenario","capella-signed-transition"},{"migrated",migrate},{"finalityScenario",finality},{"participationPercent",participation},{"finalizedEpoch",store->timed.view.finalized.epoch},{"includedAttestations",included_attestations},{"skippedForkSlots",skipped},{"blocks",accepted},{"contexts",contexts},{"storeBlocks",store->blocks.size()},{"snapshotReopened",true},{"durableReactor",true},{"signedArchives",signed_wires.size()},{"signedVotes",durable_votes.size()},{"signedSlashings",durable_proofs.size()},{"deferredVotes",deferred_votes.size()},{"localAttestations",local_signatures},{"localProposals",local_proposals},{"localRandao",local_randao},{"assembledBodies",assembled_bodies},{"storeSigkillReopens",2},{"storeVotes",votes},{"voteReorgs",vote_reorgs},{"withdrawals",paid},{"rejected",rejected},{"cancellations",cancelled},{"networkActivated",false}}.dump()<<'\n';
}
int main(int argc,char** argv){try{executable=argv[0];if(argc==2&&(std::string(argv[1])=="--local-finality"||std::string(argv[1])=="--local-finality-half")){local_finality=std::string(argv[1])=="--local-finality"?100:50;scenario(false);return 0;}
#ifdef NANO_SYNC_FAULT
if(argc==3&&std::string(argv[1])=="--sync-writer")return sync_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--reactor-sync"){sync_scenario=true;scenario(false);return 0;}
#endif
if(argc==3&&std::string(argv[1])=="--crash-writer")return crash_writer(argv[2]);if(argc==2&&std::string(argv[1])=="--finality-half"){scenario(false,0,true,50);return 0;}if(argc==2&&std::string(argv[1])=="--finality"){scenario(false,0,true);return 0;}scenario(false);scenario(true);scenario(true,3);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
