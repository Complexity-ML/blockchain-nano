#include "nano/consensus_bls.hpp"
#include "support/altair_transition_rules.hpp"
#include "nano/consensus_capella_transition.hpp"
#include "nano/consensus_capella_bls_change.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_duty.hpp"
#include "nano/rpc.hpp"
#include "nano/http.hpp"
#include "support/persistent_rpc.hpp"
#include "nano/consensus_capella_validator.hpp"
#include "nano/consensus_capella_runtime.hpp"
#include "nano/consensus_capella_sync.hpp"
#include <atomic>
#include "nano/committee.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_proposer.hpp"
#include "blst.h"
#include "../src/consensus_capella_projection.hpp"
#include "../src/beacon_block_json.hpp"
#include "../network/capella_consensus_service.hpp"
#include "../network/capella_inventory_sessions.hpp"
#include "../network/altair_core_recovery.hpp"
#include "../network/capella_catalog_relay.hpp"
#include "../network/capella_peer_core.hpp"
#include "../network/capella_block_delivery.hpp"
#include <future>
#include "support/capella_catalog_concurrency.hpp"
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
static bool owner_mode=false;
static bool owner_reorg=false;
static bool owner_sync=false;
static bool owner_witness=false;
static bool scheduled=false,sync_rotation=false;
static bool configured_runtime=false;
static bool automatic_recovery=false;
static bool capella_http=false;
static std::string cli_binary;static Json cli_genesis;static bool managed_cli=false;static std::string daemon_binary;static bool network_cli=false;static bool multipeer_cli=false;static bool distributed_cli=false;static bool sync_distributed_cli=false,distributed_sync_rotation=false;static bool transport_outage_cli=false;static fs::path network_export;
static bool beacon_reads=false;
static bool exit_lifecycle_sync=false;static bool exit_pending_pruning=false;static bool exit_reindex=false;static bool exit_retirement=false;static bool exit_sync=false;static bool exit_network=false;static bool exit_wire=false;static bool queued_exits=false;static bool local_exits=false;static bool deposit_reorg_sync=false;static bool deposit_reorg_activation=false;static bool deposit_reorg=false;static bool automatic_registration=false;static bool deposit_activation_restart=false;static bool local_sync=false;static bool deposit_invalid_signature=false;static bool deposit_activation=false;static bool deposit_consumption=false;static bool slashing_retirement=false;static bool pruning_slashing=false;static bool pruning_sync=false;static bool pruning_automatic=false;static bool pruning_native=false;static bool pruning_bls=false;static bool pruning_primitive=false;static bool queued_bls_sync=false;static bool queued_bls_reorg=false;static bool queued_bls_changes=false;static bool local_bls_changes=false;static bool wire_mode=false;static bool catalog_concurrency=false;
static size_t beacon_read_checks=0;
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
 const bool bound=bundle.value("executionOwner",false),with_witness=bundle.value("witness",false);std::unique_ptr<ChainStore> witness;if(with_witness)witness=std::make_unique<ChainStore>(directory/"witness");std::unique_ptr<ChainStore> standalone;std::unique_ptr<Blockchain> owner;
 if(bound){Genesis g;g.chain_id=chain;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=State::from_json(bundle.at("alloc"));owner=std::make_unique<Blockchain>(directory/"db",g);}else standalone=std::make_unique<ChainStore>(directory/"db");
 const auto& disk=bound?owner->store():*standalone;const auto initial=disk.fork_checkpoint(chain,genesis.header.hash(),crash_limits.envelope_bytes);check(bool(initial),"sync checkpoint missing");
 auto coordinator=with_witness?std::make_unique<CapellaDurableReactor>(*owner,rules,crash_limits,*witness):bound?std::make_unique<CapellaDurableReactor>(*owner,rules,crash_limits,initial->token):std::make_unique<CapellaDurableReactor>(*standalone,chain,genesis.header.hash(),rules,crash_limits,initial->token);auto& reactor=*coordinator;
 std::unique_ptr<Rpc> rpc;if(bound)rpc=std::make_unique<Rpc>(*owner);
 reactor.tick(bundle.at("time").get<uint64_t>());const auto before=reactor.current();const auto wire=unhex(bundle.at("wire").get<std::string>());
 const auto next=prepare_capella_received_block(*before.state,wire,rules,crash_limits.replay_slots,crash_limits.sync_trials);check(bool(next),"sync candidate rejected");
 save(directory/"expected.json",Json{{"old",capella_store_snapshot(*before.state,rules,crash_limits)},{"new",capella_store_snapshot(*next,rules,crash_limits)},{"sequence",before.token.sequence}});
 const bool fault_witness=bundle.value("faultWitness",false);fs::path wal;for(const auto& entry:fs::directory_iterator(directory/(fault_witness?"witness":"db")))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"sync WAL missing");const auto offset=fs::file_size(wal);
 nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{reactor.block(wire);}catch(const std::runtime_error&){failed=true;}check(failed&&nano_test_sync_failures()>0,"Capella sync fault not reached");
 unsigned blocked=0;auto refuse=[&](auto action){try{action();}catch(const std::runtime_error&){++blocked;}};
 refuse([&]{reactor.current();});refuse([&]{reactor.tick(bundle.at("time").get<uint64_t>());});refuse([&]{reactor.block(wire);});refuse([&]{disk.latest();});refuse([&]{disk.fork_checkpoint(chain,genesis.header.hash(),crash_limits.envelope_bytes);});
 if(bound){refuse([&]{owner->tip();});refuse([&]{owner->state();});refuse([&]{owner->finalized_block();});check(rpc->handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_blockNumber"},{"params",Json::array()}}).contains("error"),"uncertain owner RPC accessible");}
 check(blocked==(bound?(fault_witness?6:8):5),"uncertain Capella state still accessible");
 std::cout<<Json{{"wal",wal.filename().string()},{"before",offset},{"after",fs::file_size(wal)},{"blocked",blocked}}.dump()<<std::endl;_exit(0);
}
#endif
#include "support/capella_bls_sync.hpp"
#include "support/capella_exit_lifecycle_sync.hpp"
#include "support/capella_pruning_sync.hpp"
#include "support/capella_deposit_reorg.hpp"
static void local_finality_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 if(local_sync){CapellaSyncMessage golden{};golden.slot=0x0807060504030201ULL;golden.validator_index=0x1817161514131211ULL;golden.beacon_block_root.fill(0xaa);golden.signature.fill(0xbb);const auto expected=unhex("0102030405060708"+std::string(64,'a')+"1112131415161718"+std::string(192,'b'));check(encode_capella_sync_message(golden)==expected,"sync SSZ field ordering/endian mismatch");const auto decoded=decode_capella_sync_message(expected);check(decoded&&decoded->slot==golden.slot&&decoded->validator_index==golden.validator_index,"sync SSZ golden decode mismatch");}
 Temp directory;auto limits=crash_limits;if(exit_lifecycle_sync)limits.blocks=128;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;const auto genesis_root=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
 Genesis owner_genesis;owner_genesis.chain_id=t.chain_id;owner_genesis.consensus="beacon";owner_genesis.evm_revision=EvmRevision::Shanghai;owner_genesis.block=genesis;owner_genesis.state=genesis_state;
 std::array<std::unique_ptr<ChainStore>,2> lifecycle_witnesses;
 std::array<std::unique_ptr<ChainStore>,2> disks;std::array<std::unique_ptr<Blockchain>,2> owners;std::array<std::unique_ptr<CapellaDurableReactor>,2> nodes;
 for(size_t i=0;i<2;++i){if(owner_mode){owners[i]=std::make_unique<Blockchain>(directory/("node-"+std::to_string(i)),owner_genesis);bool blocked=false;try{owners[i]->tip();}catch(const std::runtime_error&){blocked=true;}check(blocked,"unattached Capella owner readable");if(exit_lifecycle_sync)lifecycle_witnesses[i]=std::make_unique<ChainStore>(directory/("exit-witness-"+std::to_string(i)));nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,initial,std::function<void()>{},lifecycle_witnesses[i].get());continue;}disks[i]=std::make_unique<ChainStore>(directory/("node-"+std::to_string(i)));disks[i]->bind_chain(t.chain_id,genesis.header.hash(),"capella-local-loop");disks[i]->commit(genesis,genesis_state,{});nodes[i]=std::make_unique<CapellaDurableReactor>(*disks[i],t.chain_id,genesis.header.hash(),rules,limits,initial);}
 if(pruning_automatic)for(auto& node:nodes)node->set_automatic_pruning(true);
 if(queued_bls_changes){SignedCapellaBlsChange change;Key withdrawal_key(1001);change.validator_index=1;copy_to(change.from_bls_pubkey,withdrawal_key.public_key);change.to_execution_address[19]=43;copy_to(change.signature,withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,genesis_root))));for(auto& node:nodes)check(node->bls_change_ssz(encode_capella_bls_change(change)),"owner queue admission rejected");}
 if(pruning_bls){SignedCapellaBlsChange change;Key key(1001);change.validator_index=1;copy_to(change.from_bls_pubkey,key.public_key);change.to_execution_address[19]=43;copy_to(change.signature,key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,genesis_root))));check(nodes[1]->bls_change_ssz(encode_capella_bls_change(change)),"pruning BLS admission failed");}
 std::map<uint64_t,std::unique_ptr<ChainStore>> key_disks;std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>> journals;
 auto journal=[&](uint64_t index)->BeaconSigningJournal&{if(!journals.contains(index)){key_disks[index]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(index)));journals[index]=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots);}return *journals.at(index);};
 const auto sender_key=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;const auto sender=SignedTransaction::sign(tx,sender_key).sender();
 size_t blocks=0,votes=0,restarts=0,reopened_journals=0;uint64_t previous_finalized=0;const uint64_t end=(exit_lifecycle_sync?10:pruning_automatic?9:5)*slots+1;size_t automatic_moves=0,maximum_records=0;auto last_anchor=initial.timed.view.anchor;
 auto equal=[&]{const auto a=nodes[0]->current(),b=nodes[1]->current();check((pruning_bls||exit_pending_pruning||a.token==b.token)&&capella_store_snapshot(*a.state,rules,limits)==capella_store_snapshot(*b.state,rules,limits)&&nodes[0]->signed_votes()==nodes[1]->signed_votes(),"local loop nodes diverged");const auto& head=capella_fork_head(*a.state,rules);for(size_t i=0;i<2;++i){const auto& disk=owner_mode?owners[i]->store():*disks[i];check(disk.latest()->header.hash()==head.execution_block.header.hash()&&disk.state_at(head.execution_block.header.number)->root()==head.execution_state.root(),"local loop disk/state diverged");
   if(owner_mode){check(owners[i]->tip().header.hash()==head.execution_block.header.hash()&&owners[i]->state().root()==head.execution_state.root()&&owners[i]->pending_count()==0,"Capella owner cache/pool diverged");const auto finalized=a.state->blocks.at(a.state->timed.view.finalized.root).execution_block;check(owners[i]->finalized_block()->header.hash()==finalized.header.hash(),"Capella finalized cache mismatch");Rpc rpc(*owners[i]);auto call=[&](const char* method,const Json& params){const auto response=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});check(!response.contains("error"),"Capella owner RPC error");return response.at("result");};check(call("eth_getBlockByNumber",Json::array({"latest",false})).at("hash")==hex(head.execution_block.header.hash()),"latest RPC mismatch");check(call("eth_getBlockByNumber",Json::array({"finalized",false})).at("hash")==hex(finalized.header.hash()),"finalized RPC mismatch");check(call("eth_getTransactionCount",Json::array({sender,"latest"}))==quantity(head.execution_state.get(sender).nonce),"nonce RPC mismatch");}
  }};
 auto add_pruning_proof=[&]{const auto current=nodes[0]->current();const auto& justified=current.state->blocks.at(current.state->timed.view.justified.root);const auto context=capella_fork_block_context(justified.state,justified.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);const auto old_root=initial.timed.view.anchor;IndexedConsensusAttestation first{};first.indices={0};first.data.slot=initial.blocks.at(old_root).header.slot;first.data.beacon_block_root=old_root;first.data.source={0,old_root};first.data.target={0,old_root};auto second=first;second.data.beacon_block_root[0]^=1;const auto version=first.data.target.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;for(auto* proof:{&first,&second})proof->signature=Key(0).sign(consensus_signing_root(consensus_attestation_root(proof->data),consensus_domain({1,0,0,0},version,context.genesis_root)));for(auto& node:nodes)check(node->slashing(first,second),"old double-sign proof rejected before pruning");return std::pair{first,second};};
 std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>> retirement_proof;bool saw_pending_proof=false,saw_retired_proof=false;size_t skipped_slashed_proposers=0;uint64_t retirement_epoch=0;
 if(slashing_retirement)retirement_proof=add_pruning_proof();
 std::set<std::string> lifecycle_stages;std::optional<Int> lifecycle_payment_height;
 Bytes retirement_exit;ConsensusRoot exit_origin{};bool saw_retained_exit=false,saw_retired_exit=false;uint64_t exit_retired_at=0;
 auto verify_exit_archive=[&]{if(!exit_retirement||retirement_exit.empty())return;
  for(size_t i=0;i<2;++i){const auto current=nodes[i]->current();const auto& finalized=current.state->blocks.at(current.state->timed.view.finalized.root);
   const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
   const bool retired=context.registry.validators.at(0).exit_epoch!=UINT64_MAX;
   const auto record=owners[i]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,current.token);check(bool(record),"exit retirement checkpoint missing");const auto snapshot=Json::parse(record->snapshot);
   const auto archive=snapshot.value("signedExits",Json::array());check(archive.size()==size_t(!retired),"exit archive retired before finality or retained afterward");
   if(retired){saw_retired_exit=true;exit_retired_at=current.state->timed.view.finalized.epoch;
    check(!nodes[i]->voluntary_exit_ssz(retirement_exit)&&nodes[i]->current().token==current.token,"finalized exit replay regrew archive");
    const auto id=root_at(sha256(retirement_exit),0);check(nodes[i]->export_message("voluntary_exit",id,112).at("available")==false,"retired exit remained exportable");
   }else{check(archive[0][0]==hex(sha256(retirement_exit))&&archive[0][2]==hex(retirement_exit),"retained exit bytes or identity changed");
    check(nodes[i]->voluntary_exit_ssz(retirement_exit)&&nodes[i]->current().token==current.token,"unfinalized exit duplicate rewrote checkpoint");
    if(nodes[i]->pending_exits().empty())saw_retained_exit=true;
   }
  }
 };

#ifdef NANO_SYNC_FAULT
 auto qualify_lifecycle_transition=[&](const Bytes& operation,bool is_tick,uint64_t time){if(!exit_lifecycle_sync||lifecycle_stages.size()==4)return;const auto before=nodes[1]->current();const auto next=is_tick?std::optional<CapellaForkStore>(prepare_capella_store_tick(*before.state,time,rules,limits.replay_slots,limits.sync_trials)):prepare_capella_received_block(*before.state,operation,rules,limits.replay_slots,limits.sync_trials);check(bool(next),"exit lifecycle candidate missing");
   const auto& old_head=capella_fork_head(*before.state,rules);const auto& new_head=capella_fork_head(*next,rules);auto context=[&](const CapellaForkBlock& b){return capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);};std::vector<std::string> stages;
   if(context(old_head).registry.validators.at(0).exit_epoch==UINT64_MAX&&context(new_head).registry.validators.at(0).exit_epoch!=UINT64_MAX)stages.push_back("inclusion");
   const auto& old_final=before.state->blocks.at(before.state->timed.view.finalized.root);const auto& new_final=next->blocks.at(next->timed.view.finalized.root);
   if(context(old_final).registry.validators.at(0).exit_epoch==UINT64_MAX&&context(new_final).registry.validators.at(0).exit_epoch!=UINT64_MAX)stages.push_back("archive-retirement");
   for(const auto& w:new_head.execution_block.withdrawals)if(quantity(w.at("validatorIndex"))==0&&quantity(w.at("amount"))>Int("30000000000")){lifecycle_payment_height=new_head.execution_block.header.number;stages.push_back("payment");}
   if(lifecycle_payment_height&&old_final.execution_block.header.number<*lifecycle_payment_height&&new_final.execution_block.header.number>=*lifecycle_payment_height)stages.push_back("payment-finality");
   for(const auto& stage:stages)if(!lifecycle_stages.contains(stage)){nodes[1].reset();owners[1].reset();lifecycle_witnesses[1].reset();qualify_exit_lifecycle_sync(directory/"node-1",directory/"exit-witness-1",rules,limits,owner_genesis,operation,stage,is_tick,time);lifecycle_witnesses[1]=std::make_unique<ChainStore>(directory/"exit-witness-1");owners[1]=std::make_unique<Blockchain>(directory/"node-1",owner_genesis);nodes[1]=std::make_unique<CapellaDurableReactor>(*owners[1],rules,limits,*lifecycle_witnesses[1]);check(nodes[1]->current().token==before.token,"fault qualification mutated original owner");lifecycle_stages.insert(stage);}
 };
#endif
 for(uint64_t slot=2;slot<=end;++slot){
  Bytes precollected;
  if(local_sync&&slot%4==3){
   const auto before=nodes[0]->current();const auto& parent=capella_fork_head(*before.state,rules);const auto& l=t.consensus.state;const size_t bit_bytes=(l.sync_committee_size+7)/8;
   const auto advanced=prepare_capella_slots(parent.state,{t.consensus.epoch,t.execution,t.maximum_state_bytes},l,slot,limits.replay_slots,limits.sync_trials);
   CapellaSyncMessage message{};message.slot=slot-1;message.beacon_block_root=root_at(advanced.ssz,176+32*(message.slot%l.slots_per_historical_root));
   ConsensusBytes4 version{};std::copy_n(advanced.ssz.begin()+(message.slot/slots<little(advanced.ssz,56,8)?48:52),4,version.begin());
   const auto signing_root=consensus_signing_root(message.beacon_block_root,consensus_domain({7,0,0,0},version,genesis_root));
   const size_t committee=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133;
   precollected.resize(bit_bytes+96);blst_p2 sum{};bool first=true;std::set<uint64_t> locally_signed;
   for(size_t position=0;position<l.sync_committee_size;++position){
    size_t member=0;for(;member<64;++member){const auto key=Key(member).public_key;if(std::equal(key.begin(),key.end(),advanced.ssz.begin()+committee+48*position))break;}check(member<64,"early sync unknown committee member");
    message.validator_index=member;const auto signature=Key(member).sign(signing_root);copy_to(message.signature,signature);
    uint64_t signing_now=before.state->timed.clock.time_ms;auto signing_clock=[&]{return signing_now;};auto provider=[&](const ConsensusRoot& root){check(root==signing_root,"local sync signing domain/root differs");return Key(member).sign(root);};
    if(locally_signed.insert(member).second){
     if(slot==3&&first){
      bool wrong=false;try{nodes[0]->sync_current(member,message.slot,signing_clock,journal((member+1)%64),provider);}catch(const std::invalid_argument&){wrong=true;}check(wrong,"sync accepted wrong signing journal");
      bool cancelled=false;try{nodes[0]->sync_current(member,message.slot,signing_clock,journal(member),provider,[]{throw ExecutionCancelled("sync signing cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled,"sync signer ignored cancellation");
      bool invalid=false;try{nodes[0]->sync_current(member,message.slot,signing_clock,journal(member),[](const ConsensusRoot&){return Bytes(96);});}catch(const std::runtime_error&){invalid=true;}check(invalid,"sync signer accepted invalid provider signature");
      bool expired=false;try{nodes[0]->sync_current(member,message.slot,signing_clock,journal(member),[&](const ConsensusRoot& root){signing_now+=rules.slot_duration_ms;return provider(root);});}catch(const ExpiredCapellaDuty&){expired=true;}check(expired,"sync signer published after deadline");signing_now=before.state->timed.clock.time_ms;
      check(nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"failed sync signing published a message");
     }
     check(nodes[0]->current_sync_members(message.slot).contains(member),"sync duty membership missing");const auto commitment=journal(member).commitment();
     const auto produced=nodes[0]->sync_current(member,message.slot,signing_clock,journal(member),provider);check(produced&&encode_capella_sync_message(*produced)==encode_capella_sync_message(message),"local sync signing differs from independent vector");check(journal(member).commitment()==commitment,"sync fabricated a slashing reservation");
     check(!nodes[0]->current_sync_members(message.slot).contains(member),"completed sync duty remained pending");
    }
    check(!nodes[0]->sync_current(member,message.slot,signing_clock,journal(member),[](const ConsensusRoot&)->Bytes{throw std::runtime_error("duplicate sync signing");}),"sync replay signed again");
    const auto bytes=encode_capella_sync_message(message);ConsensusRoot id{};copy_to(id,sha256(bytes));const auto exported=nodes[0]->export_message("sync_committee_message",id,144);check(exported["available"]==true,"early sync unavailable to relay");check(nodes[1]->sync_message_ssz(unhex(exported["envelope"]["ssz"].get<std::string>())),"early sync rejected by second reactor");
    blst_p2_affine affine;check(blst_p2_uncompress(&affine,signature.data())==BLST_SUCCESS,"early sync signature invalid");blst_p2 point;blst_p2_from_affine(&point,&affine);if(first){sum=point;first=false;}else blst_p2_add_or_double(&sum,&sum,&point);precollected[position/8]|=uint8_t(1u<<(position%8));
   }
   blst_p2_compress(precollected.data()+bit_bytes,&sum);check(nodes[0]->current().token==before.token,"early collection changed durable clock");
  }
  const auto now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;
#ifdef NANO_SYNC_FAULT
  qualify_lifecycle_transition({},true,now);
#endif
  for(auto& node:nodes)node->tick(now);
  if((exit_retirement||exit_pending_pruning||exit_lifecycle_sync)&&slot==slots+1){const auto current=nodes[0]->current();const auto& head=capella_fork_head(*current.state,rules);exit_origin=consensus_beacon_header_root(head.header);
   const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
   SignedConsensusVoluntaryExit operation{slot/slots,0,{}};const auto version=operation.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
   operation.signature=Key(0).sign(consensus_signing_root(consensus_voluntary_exit_root(operation.epoch,0),consensus_domain({4,0,0,0},version,context.genesis_root)));retirement_exit=encode_consensus_voluntary_exit_ssz(operation);
   if(exit_pending_pruning)check(nodes[1]->voluntary_exit_ssz(retirement_exit),"pending exit admission rejected");else for(auto& node:nodes)check(node->voluntary_exit_ssz(retirement_exit),"retirement exit admission rejected");verify_exit_archive();
  }
  const auto current=nodes[0]->current();const auto& head=capella_fork_head(*current.state,rules);const auto proposer=prepare_capella_proposer_identity(head.state,head.header,slot,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,t.consensus.attestation.duties,limits.replay_slots,limits.sync_trials);if(slashing_retirement&&!proposer){++skipped_slashed_proposers;continue;}check(bool(proposer),"local loop proposer missing");
  auto provider=[&](const ConsensusRoot& root){return Key(proposer->index).sign(root);};auto& proposer_journal=journal(proposer->index);
  const auto reveal=nodes[0]->sign_randao(proposer->index,slot,[&]{return now;},current.token,proposer_journal,provider);check(bool(reveal),"local loop RANDAO missing");
  if(local_sync&&slot==2){
   const auto members=nodes[0]->current_sync_members(slot);check(!members.empty(),"missing skipped-slot sync members");const auto member=*members.begin();size_t calls=0;uint64_t wall=now;auto clock=[&]{return wall;};auto signer=[&](const ConsensusRoot& root){++calls;return Key(member).sign(root);};
   check(!nodes[0]->sync_current(member,slot,clock,journal(member),signer)&&calls==0,"sync signed skipped head before one-third slot");
   wall+=rules.slot_duration_ms/3+uint64_t(rules.slot_duration_ms%3!=0);check(bool(nodes[0]->sync_current(member,slot,clock,journal(member),signer))&&calls==1,"sync did not sign skipped head at one-third slot");
  }

  tx.nonce=head.execution_state.get(sender).nonce;const std::vector<SignedTransaction> pending{SignedTransaction::sign(tx,sender_key)};
  if(owner_mode)for(auto& owner:owners)check(owner->submit(pending.front())&&owner->pending_count()==1,"Capella pool submission failed");
  Bytes expected_sync;std::vector<ConsensusRoot> exported_sync;
  if(local_sync){
   const auto& l=t.consensus.state;const size_t bit_bytes=(l.sync_committee_size+7)/8;
   expected_sync.resize(bit_bytes+96);expected_sync[bit_bytes]=0xc0;if(!precollected.empty()){expected_sync=precollected;check(!nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"early sync lost at proposal slot boundary");}
   if(slot%2==0){
    const auto advanced=prepare_capella_slots(head.state,{t.consensus.epoch,t.execution,t.maximum_state_bytes},l,slot,limits.replay_slots,limits.sync_trials);
    ConsensusBytes4 version{};std::copy_n(advanced.ssz.begin()+((slot-1)/slots<little(advanced.ssz,56,8)?48:52),4,version.begin());
    const auto root=consensus_signing_root(root_at(advanced.ssz,176+32*((slot-1)%l.slots_per_historical_root)),consensus_domain({7,0,0,0},version,genesis_root));
    const size_t committee=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133;
    std::map<uint64_t,Bytes> individual_signatures;blst_p2 sum{};bool first=true;std::array<blst_p2,2> partial_sums{};std::array<bool,2> partial_first{true,true};std::array<Bytes,2> partials{Bytes(bit_bytes+96),Bytes(bit_bytes+96)};
    for(size_t position=0;position<l.sync_committee_size;++position){size_t member=0;for(;member<64;++member){const auto key=Key(member).public_key;if(std::equal(key.begin(),key.end(),advanced.ssz.begin()+committee+48*position))break;}check(member<64,"unknown sync fixture key");
     const auto signature=Key(member).sign(root);individual_signatures.emplace(member,signature);blst_p2_affine affine;check(blst_p2_uncompress(&affine,signature.data())==BLST_SUCCESS,"sync signature encoding");blst_p2 point;blst_p2_from_affine(&point,&affine);if(first){sum=point;first=false;}else blst_p2_add_or_double(&sum,&sum,&point);expected_sync[position/8]|=uint8_t(1u<<(position%8));const auto part=position%2;if(partial_first[part]){partial_sums[part]=point;partial_first[part]=false;}else blst_p2_add_or_double(&partial_sums[part],&partial_sums[part],&point);partials[part][position/8]|=uint8_t(1u<<(position%8));
    }
    blst_p2_compress(expected_sync.data()+bit_bytes,&sum);for(size_t part=0;part<2;++part)blst_p2_compress(partials[part].data()+bit_bytes,&partial_sums[part]);
    auto bad=expected_sync;bad.back()^=1;check(!nodes[0]->submit_sync_aggregate(slot,bad,current.token),"corrupt sync aggregate accepted");
    check(!nodes[0]->submit_sync_aggregate(slot+2,expected_sync,current.token),"future sync aggregate accepted");
    auto stale=current.token;++stale.sequence;bool stale_refused=false;try{nodes[0]->submit_sync_aggregate(slot,expected_sync,stale);}catch(const StaleCapellaDuty&){stale_refused=true;}check(stale_refused,"stale sync checkpoint accepted");
    const size_t first_part=(slot/2)%2;
    check(nodes[0]->submit_sync_aggregate(slot,partials[first_part],current.token),"first partial sync rejected");
    check(nodes[0]->submit_sync_aggregate(slot,partials[first_part],current.token),"duplicate partial sync not idempotent");
    if(slot%4==0){
     CapellaSyncMessage message{};message.slot=slot-1;message.beacon_block_root=root_at(advanced.ssz,176+32*((slot-1)%l.slots_per_historical_root));message.validator_index=individual_signatures.begin()->first;copy_to(message.signature,individual_signatures.begin()->second);
     const auto outcomes_before=nodes[0]->sync_admission_stats();
     auto bad_message=message;bad_message.beacon_block_root[0]^=1;check(!nodes[0]->submit_sync_message(bad_message,current.token),"wrong sync message root accepted");
     bad_message=message;bad_message.slot=UINT64_MAX;check(!nodes[0]->submit_sync_message(bad_message,current.token),"overflow sync message slot accepted");
     bad_message=message;bad_message.slot=slot+1;check(!nodes[0]->submit_sync_message(bad_message,current.token),"future sync message accepted");
     bad_message=message;bad_message.validator_index=UINT64_MAX;check(!nodes[0]->submit_sync_message(bad_message,current.token),"unknown sync validator accepted");
     bad_message=message;bad_message.signature[95]^=1;check(!nodes[0]->submit_sync_message(bad_message,current.token),"bad individual sync signature accepted");
     for(size_t outsider=0;outsider<64;++outsider)if(!individual_signatures.contains(outsider)){bad_message=message;bad_message.validator_index=outsider;copy_to(bad_message.signature,Key(outsider).sign(root));check(!nodes[0]->submit_sync_message(bad_message,current.token),"noncommittee validator accepted");break;}
     auto outcomes=nodes[0]->sync_admission_stats();
     for(const auto name:{"root-mismatch","slot-overflow","future-slot","unknown-validator","invalid-signature","not-in-committee"})check(outcomes.at(name).get<uint64_t>()==outcomes_before.at(name).get<uint64_t>()+1,"sync rejection cause misclassified");
     if(slot>1){bad_message=message;bad_message.slot=slot-2;check(!nodes[0]->submit_sync_message(bad_message,current.token),"expired sync accepted");check(nodes[0]->sync_admission_stats().at("expired-slot").get<uint64_t>()==outcomes.at("expired-slot").get<uint64_t>()+1,"expired sync not attributed");}
     const auto before_exceptions=nodes[0]->sync_admission_stats();
     bool message_stale=false;try{nodes[0]->submit_sync_message(message,stale);}catch(const StaleCapellaDuty&){message_stale=true;}check(message_stale,"stale sync message accepted");
     bool message_cancelled=false;try{nodes[0]->submit_sync_message(message,current.token,[]{throw ExecutionCancelled("sync message stop");});}catch(const ExecutionCancelled&){message_cancelled=true;}check(message_cancelled,"sync message cancellation ignored");check(nodes[0]->sync_admission_stats()==before_exceptions,"cancelled or stale duty mislabeled as rejection");
     InboundBudget sync_budget;CapellaConsensusService sync_service(*nodes[0],sync_budget,65536,32768,4096,4096,64,65536);
     const auto session=sync_service.receipts().at("session").get<std::string>();uint64_t sequence=0;
     auto envelope=nodes[0]->network_identity();envelope["kind"]="sync_committee_message";envelope["ssz"]=hex(encode_capella_sync_message(message));
     for(const size_t width:{size_t(0),size_t(143),size_t(145)}){const Bytes malformed(width);check(!decode_capella_sync_message(malformed)&&!nodes[0]->sync_message_ssz(malformed),"malformed sync SSZ accepted");auto frame=envelope;frame["ssz"]=hex(malformed);check(!sync_service.decode(frame.dump()),"malformed sync frame accepted");frame.erase("ssz");frame["sszBase64"]=altair_base64(malformed);check(!sync_service.decode_compact(frame),"malformed compact sync frame accepted");}
     check(nodes[0]->sync_admission_stats().at("invalid-width").get<uint64_t>()==outcomes_before.at("invalid-width").get<uint64_t>()+3,"invalid wire width not attributed");
     auto foreign=envelope;foreign["chainId"]="0xffff";check(!sync_service.decode(foreign.dump()),"foreign sync chain accepted");foreign=envelope;foreign["consensusIdentity"]=hex(Bytes(32,255));check(!sync_service.decode(foreign.dump()),"foreign sync identity accepted");
     const auto empty_catalog=nodes[0]->capture_inventory("sync_committee_message",4096);
     check(empty_catalog->page({},64,16384)["entries"].empty(),"sync catalog not initially empty");
     for(const auto& [member,signature]:individual_signatures){
      message.validator_index=member;copy_to(message.signature,signature);const auto encoded=encode_capella_sync_message(message);const auto decoded=decode_capella_sync_message(encoded);check(decoded&&decoded->slot==message.slot&&decoded->validator_index==member&&decoded->beacon_block_root==message.beacon_block_root&&decoded->signature==message.signature,"sync SSZ roundtrip differs");
      envelope["ssz"]=hex(encoded);auto compact=envelope;compact.erase("ssz");compact["sszBase64"]=altair_base64(encoded);
      const Json request=sequence%2?Json(envelope.dump()):compact;sync_service.submit_receipt(session,++sequence,request);
      Json receipt;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);do{receipt=sync_service.receipt(session,sequence);if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
      check(receipt.value("decision",std::string())=="accepted","sync worker receipt rejected");sync_service.receipt(session,sequence,false,true);
      check(nodes[0]->sync_message_ssz(encoded),"individual sync SSZ replay rejected");
     }
     sync_service.stop();sync_service.join();
     check(empty_catalog->page({},64,16384)["entries"].empty(),"pinned sync catalog changed");
     const auto catalog=nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384);
     check(catalog["entries"].size()==individual_signatures.size(),"sync replay duplicated or lost exported members");
     bool bounded=false;try{nodes[0]->capture_inventory("sync_committee_message",0);}catch(const std::length_error&){bounded=true;}check(bounded,"sync root budget ignored");
     bool legacy=false;try{nodes[0]->inventory("sync_committee_message",{},64,16384);}catch(const std::invalid_argument&){legacy=true;}check(legacy,"ephemeral inventory accepted durable cursor API");
     for(const auto& entry:catalog["entries"]){ConsensusRoot id{};copy_to(id,unhex(entry.get<std::string>()));exported_sync.push_back(id);const auto out=nodes[0]->export_message("sync_committee_message",id,144);check(out["available"]==true,"sync export missing");const auto bytes=unhex(out["envelope"]["ssz"].get<std::string>());check(hex(sha256(bytes))==entry.get<std::string>(),"sync export hash differs");check(nodes[1]->sync_message_ssz(bytes),"second reactor rejected exported sync");bool small=false;try{nodes[0]->export_message("sync_committee_message",id,143);}catch(const std::length_error&){small=true;}check(small,"sync export byte bound ignored");}

     if(slot==4){
      // With no block yet at slot 4, both slot-3 and slot-4 signatures can
      // reference the same parent. Collecting slot 4 must not erase slot 3.
      auto next_message=message;next_message.slot=slot;check(nodes[0]->submit_sync_message(next_message,current.token),"next-slot sync context rejected");
      const auto both=nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384);
      check(both["entries"].size()==individual_signatures.size()+1,"two-slot sync contexts not retained independently");
     }
     check(nodes[0]->current().token==current.token,"individual sync collection mutated checkpoint");
    }else check(nodes[0]->submit_sync_aggregate(slot,partials[1-first_part],current.token),"disjoint partial sync rejected");
    check(!nodes[0]->submit_sync_aggregate(slot,partials[first_part],current.token),"overlapping partial double counted");
    check(nodes[0]->submit_sync_aggregate(slot,expected_sync,current.token)&&nodes[0]->submit_sync_aggregate(slot,expected_sync,current.token),"valid sync aggregate rejected");
    Bytes empty(bit_bytes+96);empty[bit_bytes]=0xc0;check(!nodes[0]->submit_sync_aggregate(slot,empty,current.token),"empty sync replaced full participation");
    bool cancelled=false;try{nodes[0]->submit_sync_aggregate(slot,expected_sync,current.token,[]{throw ExecutionCancelled("sync stop");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&nodes[0]->current().token==current.token,"sync submission changed durable state");
   }
  }
  const auto body=owner_mode?nodes[0]->prepare_local_body_from_pool(proposer->index,slot,*reveal,head.execution_block.header.coinbase,current.token,16,64):nodes[0]->prepare_local_body(proposer->index,slot,*reveal,head.execution_block.header.coinbase,current.token,pending,16,64);check(bool(body),"local loop body missing");if(local_sync)check(std::equal(expected_sync.begin(),expected_sync.end(),body->begin()+220),"selected sync aggregate differs or stale aggregate leaked");
  const auto wire=(queued_bls_changes||local_sync)?nodes[0]->propose_current(proposer->index,slot,head.execution_block.header.coinbase,16,64,[&]{return now;},proposer_journal,provider):nodes[0]->sign_proposal(proposer->index,slot,*body,[&]{return now;},current.token,proposer_journal,provider);check(bool(wire),"local loop signed block missing");
#ifdef NANO_SYNC_FAULT
  qualify_lifecycle_transition(*wire,false,0);
#endif
  for(auto& node:nodes)check(node->block(*wire),"local loop block admission failed");++blocks;
  if(local_sync){CapellaSyncMessage obsolete;obsolete.slot=slot-1;const auto stats=nodes[0]->sync_admission_stats();const auto published=nodes[0]->current();check(!nodes[0]->submit_sync_message(obsolete,published.token),"old-parent sync accepted after proposal");check(nodes[0]->sync_admission_stats().at("head-at-or-after-inclusion").get<uint64_t>()==stats.at("head-at-or-after-inclusion").get<uint64_t>()+1&&nodes[0]->current().token==published.token,"obsolete sync changed checkpoint or wrong cause");}
  if(local_sync){check(nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"changed head retained sync catalog");for(const auto& id:exported_sync)check(nodes[0]->export_message("sync_committee_message",id,144)["available"]==false,"changed head exported stale sync");}
  const auto admitted=nodes[0]->current();const auto duties=prepare_capella_attestation_duties(*admitted.state,rules,slot,limits.replay_slots,limits.sync_trials);
  for(const auto& [index,duty]:duties){if(slashing_retirement&&index==0)continue;if(local_finality==50&&index%2)continue;
   const auto vote=nodes[0]->sign_attestation(index,slot,[&]{return now;},nodes[0]->current().token,journal(index),[&](const ConsensusRoot& root){return Key(index).sign(root);});check(bool(vote),"local loop attestation missing");
   for(auto& node:nodes)check(node->vote(*vote),"local loop gossip vote rejected");++votes;
  }
  verify_exit_archive();
  if(slashing_retirement){for(auto& node:nodes){const auto state=node->current();const auto& finalized=state.state->blocks.at(state.state->timed.view.finalized.root);const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);const bool retired=context.registry.validators.at(0).slashed;check(node->signed_slashings().size()==size_t(!retired),"slashing proof retired before finality or retained afterward");if(retired){saw_retired_proof=true;retirement_epoch=state.state->timed.view.finalized.epoch;const auto token=state.token;check(node->slashing(retirement_proof->second,retirement_proof->first)&&node->current().token==token&&node->signed_slashings().empty(),"finalized proof replay regrew archive");}else saw_pending_proof=true;}}
  if(pruning_automatic){const auto current=nodes[0]->current();maximum_records=std::max(maximum_records,current.state->blocks.size());if(current.state->timed.view.anchor!=last_anchor){++automatic_moves;last_anchor=current.state->timed.view.anchor;const auto id=hex(Bytes(last_anchor.begin(),last_anchor.end()));for(auto& node:nodes){const auto header=node->beacon_read("/eth/v1/beacon/headers/"+id),full=node->beacon_read("/eth/v2/beacon/blocks/"+id);check(header.status==200&&full.status==200&&header.body.at("finalized")==true&&full.body.at("finalized")==true,"pruned signed anchor unavailable to Beacon API");}}check(nodes[0]->network_identity()==nodes[1]->network_identity(),"automatic pruning changed network identity");}
  const auto finalized=nodes[0]->current().state->timed.view.finalized.epoch;check(finalized>=previous_finalized,"local loop finality regressed");previous_finalized=finalized;
  if(slot%slots==0||slot==end){equal();std::cout<<Json{{"progressSlot",slot},{"finalizedEpoch",finalized},{"signedVotes",votes}}.dump()<<std::endl;}
  if(slot==3*slots||(exit_retirement&&slot==slots+1)){
   for(size_t i=0;i<2;++i){const auto saved=nodes[i]->current().token;nodes[i].reset();if(owner_mode){bool blocked=false;try{owners[i]->tip();}catch(const std::runtime_error&){blocked=true;}check(blocked,"detached Capella owner remained readable");owners[i].reset();owners[i]=std::make_unique<Blockchain>(directory/("node-"+std::to_string(i)),owner_genesis);if(exit_lifecycle_sync)nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,*lifecycle_witnesses[i]);else nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,saved);}else{disks[i].reset();disks[i]=std::make_unique<ChainStore>(directory/("node-"+std::to_string(i)));nodes[i]=std::make_unique<CapellaDurableReactor>(*disks[i],t.chain_id,genesis.header.hash(),rules,limits,saved);}if(pruning_automatic)nodes[i]->set_automatic_pruning(true);++restarts;}
   reopened_journals=journals.size();for(auto& [index,value]:journals){const auto saved=value->commitment();value.reset();key_disks[index].reset();key_disks[index]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(index)));value=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots,saved);}
   equal();verify_exit_archive();
  }
 }
 CapellaSignedVotes pruning_proofs;
 if(pruning_slashing){add_pruning_proof();pruning_proofs=nodes[0]->signed_slashings();check(pruning_proofs.size()==1&&nodes[1]->signed_slashings()==pruning_proofs,"pruning slashing archive differs");}
 const auto result=nodes[0]->current();const auto& head=capella_fork_head(*result.state,rules);check(head.execution_state.get(sender).nonce==Int(std::to_string(blocks+1)),"local loop execution transactions missing");
 if(local_finality==100)check(result.state->timed.view.finalized.epoch>=2&&result.state->timed.view.justified.epoch>=3,"assembled local chain did not finalize");else check(result.state->timed.view.finalized.epoch==0,"half-participation local chain finalized");
#ifdef NANO_SYNC_FAULT
 if(pruning_sync)qualify_pruning_sync(*result.state,nodes[0]->signed_votes(),rules,owner_genesis);
#endif
 if(pruning_automatic){check(automatic_moves>=3&&maximum_records<=5*slots&&result.state->blocks.size()<5*slots,"automatic pruning did not bound retained block states");for(size_t i=0;i<2;++i){const auto token=nodes[i]->current().token;nodes[i].reset();owners[i].reset();owners[i]=std::make_unique<Blockchain>(directory/("node-"+std::to_string(i)),owner_genesis);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,token);nodes[i]->set_automatic_pruning(true);}equal();std::cout<<Json{{"scenario","capella-automatic-pruning"},{"anchorMoves",automatic_moves},{"maximumRetainedRecords",maximum_records},{"finalRetainedRecords",result.state->blocks.size()},{"depositAncestorPins",result.state->deposit_ancestors.size()},{"nativeProfile",pruning_native},{"finalizedEpoch",result.state->timed.view.finalized.epoch},{"reopenedBothOwners",true}}.dump()<<std::endl;}
 if(slashing_retirement){check(saw_pending_proof&&saw_retired_proof&&nodes[0]->signed_slashings().empty()&&nodes[1]->signed_slashings().empty(),"slashing archive lifecycle incomplete");std::cout<<Json{{"scenario","capella-slashing-retirement"},{"retainedUntilFinalized",true},{"retiredAfterFinality",true},{"replayDidNotRegrowArchive",true},{"finalizedEpoch",retirement_epoch},{"skippedSlashedProposers",skipped_slashed_proposers},{"reopenedWithoutProof",true}}.dump()<<std::endl;}
 if(pruning_primitive){
  const auto old=*result.state;const auto anchor=old.timed.view.finalized.root;const auto pruned=prepare_capella_pruned_store(old,anchor,rules);check(pruned.blocks.size()<old.blocks.size()&&capella_network_anchor(pruned)==capella_network_anchor(old),"pruning failed to reduce state or changed identity");check(pruned.timed.view.finalized==old.timed.view.finalized&&pruned.timed.view.justified==old.timed.view.justified,"pruning changed checkpoints");
  const auto encoded=capella_store_snapshot(pruned,rules,limits),original=capella_store_snapshot(old,rules,limits);check(encoded.size()<original.size(),"pruned snapshot did not shrink");const auto restored=capella_store_from_snapshot(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules,limits);check(capella_store_snapshot(restored,rules,limits)==encoded&&capella_network_anchor(restored)==capella_network_anchor(old),"pruned snapshot restart differs");
  if(pruning_native){check(Json::parse(encoded).at("format")=="nano-capella-store-v3"&&!restored.deposit_ancestors.empty(),"native pruning did not persist v3 ancestry");const auto pins=Json::parse(encoded).at("depositAncestors");for(unsigned attack=0;attack<3;++attack){auto corrupt=Json::parse(encoded);if(attack==0)corrupt["depositAncestors"][0][3]=hex(Bytes(32,255));if(attack==1)corrupt["depositAncestors"].erase(corrupt["depositAncestors"].end()-1);if(attack==2){corrupt.erase("depositAncestors");corrupt["format"]="nano-capella-store-v2";}const auto wire=corrupt.dump();bool refused=false;try{capella_store_from_snapshot(wire,sha256(Bytes(wire.begin(),wire.end())),rules,limits);}catch(const std::invalid_argument&){refused=true;}check(refused,"corrupt native ancestor checkpoint accepted");}}
  if(pruning_slashing)check(!pruned.blocks.contains(initial.timed.view.anchor)&&pruned.timed.view.equivocating_indices.contains(0),"slashing pruning did not remove old proof block or lost offender");
  const auto head_id=consensus_beacon_header_root(head.header);for(Int height=0;height<head.execution_block.header.number;++height)check(capella_execution_history(pruned,head_id,height)==capella_execution_history(old,head_id,height),"pruning changed BLOCKHASH history");
  for(const auto& [index,message]:old.timed.view.latest_messages)check(pruned.timed.view.latest_messages.at(index).epoch==message.epoch,"pruning lost vote replay watermark");
  auto dormant=old;dormant.timed.view.latest_messages[0]={0,old.timed.view.anchor};const auto rebased=prepare_capella_pruned_store(dormant,anchor,rules);check(rebased.timed.view.latest_messages.at(0).epoch==0&&rebased.timed.view.latest_messages.at(0).root==anchor,"pruning failed to retain old vote watermark at new anchor");check(capella_store_snapshot(prepare_capella_pruned_store(pruned,anchor,rules),rules,limits)==encoded,"repeat pruning not idempotent");
  auto unsupported=rules;unsupported.native_deposit_votes=true;unsupported.deposit_contract_code_hash=Bytes(32,1);auto missing_history=old;missing_history.blocks.at(old.timed.view.anchor).deposit_history.reset();bool deposit_refused=false;try{prepare_capella_pruned_store(missing_history,anchor,unsupported);}catch(const std::invalid_argument&){deposit_refused=true;}check(deposit_refused,"unqualified native deposit history pruned");

  bool refused=false;try{prepare_capella_pruned_store(old,head_id,rules);}catch(const std::invalid_argument&){refused=true;}check(refused,"unfinalized pruning anchor accepted");bool cancelled=false;try{prepare_capella_pruned_store(old,anchor,rules,[]{throw ExecutionCancelled("prune stopped");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&capella_store_snapshot(old,rules,limits)==original,"cancelled pruning changed input");
  Temp pruned_directory;{ChainStore disk(pruned_directory/"db");disk.bind_chain(t.chain_id,genesis.header.hash(),"capella-pruned-test");disk.commit(genesis,genesis_state,{});std::map<Int,const CapellaForkBlock*> records;for(const auto& [id,record]:old.blocks)records.emplace(record.execution_block.header.number,&record);for(const auto& [height,record]:records)disk.commit(record->execution_block,record->execution_state,record->receipts);auto reactor=std::make_unique<CapellaDurableReactor>(disk,t.chain_id,genesis.header.hash(),rules,limits,pruned);check(reactor->network_identity()==nodes[0]->network_identity(),"pruned reactor network identity changed");const auto token=reactor->current().token;reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(disk,t.chain_id,genesis.header.hash(),rules,limits,token);check(reactor->network_identity()==nodes[0]->network_identity(),"pruned restart identity changed");}

  Bytes pending_wire;ConsensusRoot pending_origin{};if(pruning_bls){const auto pending=nodes[1]->pending_bls_changes();check(pending.size()==1,"pending BLS lost before pruning");pending_wire=encode_capella_bls_change(pending.front());const auto record=owners[1]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,nodes[1]->current().token);const auto data=Json::parse(record->snapshot);const auto root=unhex(data.at("blsChanges")[0][1].get<std::string>());std::copy(root.begin(),root.end(),pending_origin.begin());check(!pruned.blocks.contains(pending_origin),"BLS origin was not removed by pruning");}
  if(exit_pending_pruning){const auto pending=nodes[1]->pending_exits();check(pending.size()==1&&encode_consensus_voluntary_exit_ssz(pending.front())==retirement_exit,"never-included exit lost before pruning");check(!pruned.blocks.contains(exit_origin),"pending exit origin retained by pruning");const auto& final=old.blocks.at(anchor);const auto context=capella_fork_block_context(final.state,final.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.at(0).exit_epoch==UINT64_MAX,"pending exit was unexpectedly included");}
  const auto before_prune=nodes[1]->current().token;bool live_cancelled=false;try{nodes[1]->prune_finalized([]{throw ExecutionCancelled("durable prune stopped");});}catch(const ExecutionCancelled&){live_cancelled=true;}check(live_cancelled&&nodes[1]->current().token==before_prune,"cancelled durable prune changed checkpoint");
  const auto identity=nodes[1]->network_identity();check(nodes[1]->prune_finalized(),"durable pruning did not run");const auto pruned_token=nodes[1]->current().token;check(pruned_token.sequence==before_prune.sequence+1&&nodes[1]->network_identity()==identity&&capella_store_snapshot(*nodes[1]->current().state,rules,limits)==encoded,"durable pruning differs from candidate");check(!nodes[1]->prune_finalized()&&nodes[1]->current().token==pruned_token,"duplicate pruning wrote checkpoint");
  nodes[1].reset();owners[1].reset();owners[1]=std::make_unique<Blockchain>(directory/"node-1",owner_genesis);nodes[1]=std::make_unique<CapellaDurableReactor>(*owners[1],rules,limits,pruned_token);check(nodes[1]->network_identity()==identity&&nodes[1]->signed_votes()==nodes[0]->signed_votes(),"durable pruning restart lost identity or signed votes");
  if(exit_pending_pruning){const auto pending=nodes[1]->pending_exits();check(pending.size()==1&&encode_consensus_voluntary_exit_ssz(pending.front())==retirement_exit,"prune/reopen lost pending exit bytes");const auto record=owners[1]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,pruned_token);const auto archive=Json::parse(record->snapshot).at("signedExits");check(archive.size()==1&&archive[0][1]==hex(Bytes(anchor.begin(),anchor.end()))&&archive[0][2]==hex(retirement_exit),"pending exit did not reanchor at finalized state");check(nodes[1]->voluntary_exit_ssz(retirement_exit)&&nodes[1]->current().token==pruned_token,"pending exit replay mutated checkpoint");}
  if(pruning_slashing){check(nodes[1]->signed_slashings()==pruning_proofs&&nodes[1]->current().state->timed.view.equivocating_indices.contains(0),"slashing evidence lost after pruning restart");const auto proof=decode_consensus_attester_slashing_ssz(pruning_proofs.begin()->second,t.consensus.body.validators_per_committee);check(proof&&nodes[1]->slashing(proof->second,proof->first)&&nodes[1]->current().token==pruned_token,"reversed old proof changed checkpoint after pruning");}
  if(pruning_bls){const auto pending=nodes[1]->pending_bls_changes();check(pending.size()==1&&encode_capella_bls_change(pending.front())==pending_wire,"pruning/restart lost signed BLS operation");const auto record=owners[1]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,pruned_token);const auto data=Json::parse(record->snapshot);check(data.at("blsChanges")[0][1]==hex(Bytes(anchor.begin(),anchor.end())),"BLS origin not re-anchored at finalized state");check(nodes[1]->bls_change_ssz(pending_wire)&&nodes[1]->current().token==pruned_token,"replayed BLS request duplicated checkpoint");}
  if(pruning_native){Phase0Deposit deposit{};Key depositor((deposit_activation||deposit_invalid_signature)?64:2);copy_to(deposit.message.public_key,depositor.public_key);deposit.message.withdrawal_credentials[0]=1;deposit.message.withdrawal_credentials[31]=44;deposit.message.amount_gwei=(deposit_activation||deposit_invalid_signature)?t.consensus.deposit.maximum_effective_balance:1000000000;copy_to(deposit.signature,depositor.sign(consensus_signing_root(consensus_deposit_root(deposit.message),consensus_domain({3,0,0,0},t.genesis_version,{}))));if(deposit_invalid_signature){check(verify_consensus_deposit_signature(deposit.message,deposit.signature,t.genesis_version),"valid deposit control failed");copy_to(deposit.signature,Key(65).sign(consensus_signing_root(consensus_deposit_root(deposit.message),consensus_domain({3,0,0,0},t.genesis_version,{}))));check(!verify_consensus_deposit_signature(deposit.message,deposit.signature,t.genesis_version),"wrong-key deposit control accepted");}const std::string method="deposit(bytes,bytes,bytes,bytes32)";auto data=keccak(Bytes(method.begin(),method.end()));data.resize(4);Bytes args(416);auto word=[&](size_t at,size_t n){const auto encoded=bytes(Int(n),32);std::copy(encoded.begin(),encoded.end(),args.begin()+at);};word(0,128);word(32,224);word(64,288);const auto root=consensus_deposit_data_root(deposit);std::copy(root.begin(),root.end(),args.begin()+96);word(128,48);std::copy(deposit.message.public_key.begin(),deposit.message.public_key.end(),args.begin()+160);word(224,32);std::copy(deposit.message.withdrawal_credentials.begin(),deposit.message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(deposit.signature.begin(),deposit.signature.end(),args.begin()+320);data.insert(data.end(),args.begin(),args.end());auto request=tx;request.nonce=head.execution_state.get(sender).nonce;request.to=ethereum_deposit_contract;request.value=Int(std::to_string(deposit.message.amount_gwei))*1000000000;request.gas=1000000;request.data=std::move(data);check(owners[0]->submit(SignedTransaction::sign(request,sender_key)),"post-pruning deposit submission failed");}
  const auto next_slot=end+1,now=old.timed.clock.genesis_time_ms+next_slot*rules.slot_duration_ms;auto& proposer_node=nodes[(pruning_bls||exit_pending_pruning)?1:0];proposer_node->tick(now);const auto selected=proposer_node->current_proposer(next_slot);check(bool(selected),"next proposer missing after pruning");const auto wire=proposer_node->propose_current(*selected,next_slot,head.execution_block.header.coinbase,16,64,[&]{return now;},journal(*selected),[&](const ConsensusRoot& root){return Key(*selected).sign(root);});check(bool(wire),"next proposal missing");
  const auto before=prepare_capella_store_tick(old,now,rules,limits.replay_slots,limits.sync_trials),after=prepare_capella_store_tick(restored,now,rules,limits.replay_slots,limits.sync_trials);const auto reference=prepare_capella_received_block(before,*wire,rules,limits.replay_slots,limits.sync_trials),candidate=prepare_capella_received_block(after,*wire,rules,limits.replay_slots,limits.sync_trials);check(reference&&candidate,"pruned store rejected next valid block");const auto& a=capella_fork_head(*reference,rules);const auto& b=capella_fork_head(*candidate,rules);check(a.state==b.state&&a.execution_state.root()==b.execution_state.root()&&a.execution_block.header.hash()==b.execution_block.header.hash(),"pruned continuation differs");
  nodes[1]->tick(now);check(nodes[1]->block(*wire),"durably pruned owner rejected next block");check(owners[1]->tip().header.hash()==a.execution_block.header.hash()&&owners[1]->state().root()==a.execution_state.root(),"durably pruned owner continuation differs");
  if(pruning_slashing){const auto context=capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.at(0).slashed,"preserved proof was not applied by next block");check(nodes[1]->signed_slashings()==pruning_proofs,"unfinalized slashing evidence retired after inclusion");}
  if(pruning_native){check(a.deposit_history&&b.deposit_history&&a.deposit_history->count()==1&&a.deposit_history->snapshot()==b.deposit_history->snapshot(),"new deposit after native pruning differs");const auto token=nodes[1]->current().token;nodes[1].reset();owners[1].reset();owners[1]=std::make_unique<Blockchain>(directory/"node-1",owner_genesis);nodes[1]=std::make_unique<CapellaDurableReactor>(*owners[1],rules,limits,token);check(capella_fork_head(*nodes[1]->current().state,rules).deposit_history->snapshot()==a.deposit_history->snapshot(),"new deposit history lost after native restart");}
  if(deposit_consumption){
   check(nodes[0]->block(*wire),"reference did not admit recorded deposit block");for(auto& node:nodes)node->set_automatic_pruning(true);
   const auto period=t.consensus.state.eth1_votes_limit;check(period&&period<=128,"deposit consumption fixture period unbounded");const auto next_period=(next_slot/period+1)*period;const auto stop=next_period+period/2+(deposit_activation?(t.consensus.exit.max_seed_lookahead+6)*slots:slots)+1;size_t consumed=0,new_votes=0;uint64_t consumed_slot=0,activation_epoch=UINT64_MAX;bool observed_pending=false,observed_active=false;const size_t eth1=176+64*t.consensus.state.slots_per_historical_root;const auto fee=head.execution_block.header.coinbase;
   bool pending_restarted=false,active_restarted=false,conflict_refused=false;
   uint64_t registration_clock=nodes[0]->current().state->timed.clock.time_ms;
   std::unique_ptr<CapellaLocalClock> registration_time;std::unique_ptr<CapellaValidatorLoop> registration_loop;
   size_t registration_signatures=0;
   if(automatic_registration){
    registration_time=std::make_unique<CapellaLocalClock>(*nodes[0],2*rules.slot_duration_ms,[&]{return registration_clock;});
    auto shared_journal=std::shared_ptr<BeaconSigningJournal>(&journal(64),[](auto*){});
    std::vector<CapellaValidatorKey> local{{0,shared_journal,[&](const ConsensusRoot& root){++registration_signatures;return Key(64).sign(root);},fee,Key(64).public_key}};
    registration_loop=std::make_unique<CapellaValidatorLoop>(*nodes[0],*registration_time,rules.slot_duration_ms,slots,16,64,std::move(local),[&]{return registration_clock;});
    registration_loop->poll();check(registration_loop->status().at("awaitingRegistration")==1&&registration_signatures==0,"unknown key did not wait without signing");
   }

   auto reopen_new_validator=[&](size_t node_index,uint64_t slot,const char* phase){
    const auto previous=nodes[node_index]->current();const auto& h=capella_fork_head(*previous.state,rules);
    const auto journal_token=journal(64).commitment();const auto signed_target=journal(64).latest_attestation_target();
    nodes[node_index].reset();owners[node_index].reset();
    owners[node_index]=std::make_unique<Blockchain>(directory/("node-"+std::to_string(node_index)),owner_genesis);
    nodes[node_index]=std::make_unique<CapellaDurableReactor>(*owners[node_index],rules,limits,previous.token);
    nodes[node_index]->set_automatic_pruning(true);
    journals[64].reset();key_disks[64].reset();key_disks[64]=std::make_unique<ChainStore>(directory/"key-64");
    journals[64]=std::make_unique<BeaconSigningJournal>(*key_disks[64],t.chain_id,genesis.header.hash(),genesis_root,Key(64).public_key,slots,journal_token);
    const auto restored=nodes[node_index]->current();const auto& restored_head=capella_fork_head(*restored.state,rules);
    check(restored.token==previous.token&&restored_head.state==h.state&&owners[node_index]->state().root()==h.execution_state.root(),"activation restart changed Beacon or EVM state");
    check(journal(64).commitment()==journal_token&&journal(64).latest_attestation_target()==signed_target,"activation restart lost signing reservation");
    std::cout<<Json{{"activationRestartPhase",phase},{"slot",slot},{"owner",node_index},{"signedTarget",signed_target?Json(*signed_target):Json(nullptr)}}.dump()<<std::endl;
   };
   for(uint64_t slot=next_slot+1;slot<=stop;++slot){const auto clock=old.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;for(auto& node:nodes)node->tick(clock);const auto prior=nodes[0]->current();const auto& parent=capella_fork_head(*prior.state,rules);const auto prior_index=little(parent.state.ssz,eth1+80,8);const auto advanced=prepare_capella_slots(parent.state,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,slot,limits.replay_slots,limits.sync_trials);const auto balances=little(advanced.ssz,eth1+92,4);const auto balance_before=little(advanced.ssz,balances+16,8);
    const auto proposer=nodes[0]->current_proposer(slot);check(bool(proposer),"deposit cycle proposer missing");const auto next=nodes[0]->propose_current(*proposer,slot,fee,16,64,[&]{return clock;},journal(*proposer),[&](const ConsensusRoot& root){return Key(*proposer).sign(root);});check(bool(next),"deposit cycle proposal missing");for(auto& node:nodes)check(node->block(*next),"deposit cycle block rejected");const auto left=nodes[0]->current(),right=nodes[1]->current();const auto& left_head=capella_fork_head(*left.state,rules);const auto& right_head=capella_fork_head(*right.state,rules);check(left_head.state==right_head.state&&left_head.execution_block.header.hash()==right_head.execution_block.header.hash()&&owners[0]->state().root()==owners[1]->state().root(),"deposit cycle nodes diverged");const auto index=little(left_head.state.ssz,eth1+80,8);check(index<=1&&index>=prior_index,"deposit cursor regressed or advanced twice");
    if(index>prior_index&&!deposit_activation&&!deposit_invalid_signature){
     ++consumed;consumed_slot=slot;
     const auto context=capella_fork_block_context(left_head.state,left_head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
     // Isolate the top-up from withdrawals and empty-sync penalties. A proposer
     // also earns attestation inclusion rewards, so this fixture requires another key.
     check(*proposer!=2&&context.registry.validators.size()==64,"top-up accounting fixture changed");
     const auto& sync=t.consensus.sync;const auto& state_limits=t.consensus.state;
     const auto validator_start=little(advanced.ssz,eth1+88,4);
     uint64_t active_balance=0;
     for(size_t i=0;i<64;++i){const auto at=validator_start+121*i;if(little(advanced.ssz,at+97,8)<=slot/slots&&slot/slots<little(advanced.ssz,at+105,8))active_balance+=little(advanced.ssz,at+80,8);}
     active_balance=std::max(active_balance,sync.effective_balance_increment);
     Int square;const Int total(std::to_string(active_balance));mpz_sqrt(square.get_mpz_t(),total.get_mpz_t());
     const auto reward_per_increment=sync.effective_balance_increment*sync.base_reward_factor/u64(square);
     const auto penalty=reward_per_increment*(active_balance/sync.effective_balance_increment)*sync.sync_reward_weight/sync.weight_denominator/slots/state_limits.sync_committee_size;
     const auto committee=eth1+96+32*state_limits.epochs_per_historical_vector+8*state_limits.epochs_per_slashings_vector+133;
     const auto sync_start=184+220;const auto bit_bytes=(state_limits.sync_committee_size+7)/8;
     check(std::all_of(next->begin()+sync_start,next->begin()+sync_start+bit_bytes,[](uint8_t b){return b==0;}),"top-up accounting requires empty sync aggregate");
     uint64_t withdrawn=0,memberships=0;
     for(const auto& withdrawal:left_head.execution_block.withdrawals)if(u64(quantity(withdrawal.at("validatorIndex")))==2)withdrawn+=u64(quantity(withdrawal.at("amount")));
     check(withdrawn<=balance_before,"withdrawal exceeds pre-block balance");
     auto expected=balance_before-withdrawn+1000000000;
     const auto key=Key(2).public_key;
     for(size_t i=0;i<state_limits.sync_committee_size;++i)if(std::equal(key.begin(),key.end(),advanced.ssz.begin()+committee+48*i)){++memberships;expected-=std::min(expected,penalty);}
     const auto balance_after=context.registry.validators.at(2).balance;
     std::cout<<Json{{"consumedSlot",slot},{"balanceBefore",balance_before},{"balanceAfter",balance_after},{"withdrawnGwei",withdrawn},{"syncMemberships",memberships},{"syncPenaltyPerPositionGwei",penalty},{"expectedBalance",expected}}.dump()<<std::endl;
     check(balance_after==expected,"consumed deposit balance differs from exact accounting");
    }
    if(deposit_activation){
     const auto context=capella_fork_block_context(left_head.state,left_head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
     check(context.registry.validators.size()==(index?65:64),"deposit admission registry count mismatch");
     if(index){const auto& added=context.registry.validators.at(64);const auto key=Key(64).public_key;
      check(std::equal(key.begin(),key.end(),added.public_key.begin())&&added.withdrawal_credentials[0]==1&&added.withdrawal_credentials[31]==44,"new validator identity mismatch");
      if(index>prior_index){++consumed;consumed_slot=slot;check(added.balance==t.consensus.deposit.maximum_effective_balance&&added.effective_balance==added.balance&&added.activation_epoch==UINT64_MAX,"new deposit not pending at full stake");}
      if(added.activation_epoch!=UINT64_MAX){if(activation_epoch==UINT64_MAX)activation_epoch=added.activation_epoch;check(activation_epoch==added.activation_epoch,"activation epoch changed after scheduling");}
      const bool active=added.activation_epoch<=slot/slots;observed_pending|=!active;observed_active|=active;
      if(deposit_activation_restart&&!active&&activation_epoch!=UINT64_MAX&&!pending_restarted){reopen_new_validator(0,slot,"scheduled-pending");pending_restarted=true;}
      if(!active){for(auto& node:nodes){const auto token=node->current().token;bool signer_called=false;check(!node->sign_attestation(64,slot,[&]{return clock;},token,journal(64),[&](const ConsensusRoot& root){signer_called=true;return Key(64).sign(root);}),"pending validator signed an attestation");check(!signer_called&&node->current().token==token,"pending vote reached signer or changed checkpoint");}}
     }
    }
    if(deposit_invalid_signature){
     if(index>prior_index){++consumed;consumed_slot=slot;}
     const auto context=capella_fork_block_context(left_head.state,left_head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
     check(context.registry.validators.size()==64&&!context.registry.validators.contains(64),"wrong-key deposit admitted validator");
     for(auto& node:nodes){const auto token=node->current().token;bool signer_called=false;
      check(!node->sign_attestation(64,slot,[&]{return clock;},token,journal(64),[&](const ConsensusRoot& root){signer_called=true;return Key(64).sign(root);}),"unregistered key received signing duty");
      check(!signer_called&&node->current().token==token,"unregistered key reached signer or changed checkpoint");
     }
    }
    const auto replay_token=nodes[1]->current().token;check(nodes[1]->block(*next)&&nodes[1]->current().token==replay_token,"deposit block replay changed checkpoint");const auto duties=prepare_capella_attestation_duties(*left.state,rules,slot,limits.replay_slots,limits.sync_trials);for(const auto& [index,duty]:duties){if(automatic_registration&&index==64)continue;const auto vote=nodes[0]->sign_attestation(index,slot,[&]{return clock;},nodes[0]->current().token,journal(index),[&](const ConsensusRoot& root){return Key(index).sign(root);});check(bool(vote),"deposit cycle vote missing");for(auto& node:nodes)check(node->vote(*vote),"deposit cycle vote rejected");if(deposit_activation&&index==64){check(slot/slots>=activation_epoch,"new validator voted before activation");++new_votes;
      if(deposit_activation_restart&&!active_restarted){
       check(pending_restarted,"first new vote preceded pending restart");reopen_new_validator(1,slot,"first-vote");active_restarted=true;
       auto contradictory=vote->data;contradictory.beacon_block_root[0]^=1;
       const auto context=capella_fork_block_context(left_head.state,left_head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
       const auto version=contradictory.target.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
       const auto reserved=journal(64).commitment();bool signer_called=false;
       try{journal(64).sign_attestation(contradictory,version,[&](const ConsensusRoot& root){signer_called=true;return Key(64).sign(root);});}catch(const std::runtime_error& e){if(std::string(e.what())!="Conflicting or old beacon attestation refused")throw;conflict_refused=true;}
       check(conflict_refused&&!signer_called&&journal(64).commitment()==reserved,"reopened new validator signed conflicting vote or mutated journal");
       std::cout<<Json{{"activationConflictRejected",true},{"slot",slot},{"signerCalled",signer_called}}.dump()<<std::endl;
      }
     }}
    if(registration_loop){
     const auto archived=nodes[0]->signed_votes();registration_clock=clock;registration_loop->poll();
     const auto status=registration_loop->status();check(status.at("healthy")==true,"automatic registration loop failed");
     check(status.at("awaitingRegistration")==size_t(index==0),"public key resolution does not match deposit admission");
     for(const auto& [id,bytes]:nodes[0]->signed_votes())if(!archived.contains(id))check(nodes[1]->vote_ssz(bytes),"automatic newcomer vote rejected by peer");
     new_votes=status.at("attestationsAdmitted").get<size_t>();
     if(slot/slots<activation_epoch)check(new_votes==0&&registration_signatures==0,"automatic registration signed before activation");
    }
    if(slot%slots==0||slot==stop)std::cout<<Json{{"depositProgressSlot",slot},{"depositIndex",index},{"finalizedEpoch",left.state->timed.view.finalized.epoch}}.dump()<<std::endl;
   }
   if(registration_loop){const auto receipt=registration_loop->restart_commitments();check(receipt.size()==1&&receipt[0].at("index").is_null()&&receipt[0].at("publicKey")==hex(Key(64).public_key)&&new_votes>0,"automatic registration receipt or duties missing");std::cout<<Json{{"scenario","capella-automatic-registration"},{"newValidatorVotes",new_votes},{"signatures",registration_signatures},{"publicKeyBound",true}}.dump()<<std::endl;registration_loop.reset();registration_time.reset();}
   check(consumed==1&&consumed_slot>=next_period&&consumed_slot<=next_period+period/2,"deposit not consumed at native majority");for(size_t i=0;i<2;++i){const auto token=nodes[i]->current().token;nodes[i].reset();owners[i].reset();owners[i]=std::make_unique<Blockchain>(directory/("node-"+std::to_string(i)),owner_genesis);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,token);check(little(capella_fork_head(*nodes[i]->current().state,rules).state.ssz,eth1+80,8)==1,"consumed cursor lost on restart");}
   if(deposit_activation){check(observed_pending&&observed_active&&new_votes>0,"new validator never completed activation and voting");for(auto& node:nodes){const auto current=node->current();const auto& h=capella_fork_head(*current.state,rules);const auto context=capella_fork_block_context(h.state,h.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.size()==65&&context.registry.validators.at(64).activation_epoch==activation_epoch,"activation lost after restart");}}
   if(deposit_invalid_signature)for(auto& node:nodes){const auto current=node->current();const auto& h=capella_fork_head(*current.state,rules);const auto context=capella_fork_block_context(h.state,h.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.size()==64,"invalid deposit created validator after restart");}
   if(deposit_activation_restart){check(pending_restarted&&active_restarted&&conflict_refused&&new_votes>=2,"activation restart did not resume safe later duties");std::cout<<Json{{"scenario","capella-deposit-activation-restart"},{"pendingRestart",pending_restarted},{"firstVoteRestart",active_restarted},{"conflictingVoteRejectedBeforeSigner",conflict_refused},{"newValidatorVotes",new_votes},{"finalizedEpoch",nodes[0]->current().state->timed.view.finalized.epoch}}.dump()<<std::endl;}
   std::cout<<Json{{"scenario",deposit_invalid_signature?"capella-native-invalid-deposit-signature":deposit_activation?"capella-native-deposit-activation":"capella-native-deposit-consumption"},{"activationEpoch",activation_epoch},{"newValidatorVotes",new_votes},{"votingPeriodSlots",period},{"consumedAtSlot",consumed_slot},{"continuedThroughSlot",stop},{"consumptionTransitions",consumed},{"existingValidatorTopup",!deposit_activation&&!deposit_invalid_signature},{"invalidSignatureConsumedWithoutAdmission",deposit_invalid_signature},{"blockReplayIdempotent",true},{"bothOwnersReopened",true}}.dump()<<std::endl;
  }
  if(exit_pending_pruning){const auto context=capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.at(0).exit_epoch!=UINT64_MAX&&nodes[1]->pending_exits().empty(),"reanchored exit not automatically included");const auto record=owners[1]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,nodes[1]->current().token);check(Json::parse(record->snapshot).at("signedExits").size()==1,"unfinalized reanchored exit prematurely retired");std::cout<<Json{{"scenario","capella-pending-exit-pruning"},{"neverIncludedBeforePruning",true},{"admissionOriginRemoved",true},{"reanchoredAtFinality",true},{"exactBytesAfterReopen",true},{"duplicateIdempotent",true},{"automaticallyIncludedAfterReopen",true},{"unfinalizedArchiveRetained",true},{"networkTransport",false}}.dump()<<std::endl;}
  if(pruning_bls){const auto context=capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);const auto credentials=context.registry.validators.at(1).withdrawal_credentials;check(credentials[0]==1&&credentials[31]==43&&nodes[1]->pending_bls_changes().empty(),"re-anchored BLS request not applied by next block");const auto record=owners[1]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,nodes[1]->current().token);check(Json::parse(record->snapshot).at("blsChanges").size()==1,"unfinalized BLS archive prematurely retired");}
  std::cout<<Json{{"scenario","capella-pruning-primitive"},{"beforeBlocks",old.blocks.size()},{"afterBlocks",pruned.blocks.size()},{"beforeSnapshotBytes",original.size()},{"afterSnapshotBytes",encoded.size()},{"networkAnchorPreserved",true},{"historyPreserved",true},{"nextBlockEqual",true},{"automaticPruning",false},{"durablePruningRestart",true},{"pendingBlsReanchoredAndIncluded",pruning_bls},{"nativeCheckpointV3",pruning_native},{"oldSlashingPreservedAndApplied",pruning_slashing}}.dump()<<std::endl;
 }
 if(queued_bls_changes){check(head.execution_state.get("0x000000000000000000000000000000000000002b").balance>0,"owner queued withdrawal unpaid");for(size_t i=0;i<2;++i){const auto& disk=owner_mode?owners[i]->store():*disks[i];const auto record=disk.fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,nodes[i]->current().token);check(record&&!Json::parse(record->snapshot).contains("blsChanges"),"finalized BLS queue not retired");}}
 if(exit_lifecycle_sync){check(lifecycle_stages==std::set<std::string>{"inclusion","archive-retirement","payment","payment-finality"},"exit lifecycle fault boundary missing");std::cout<<Json{{"scenario","capella-exit-lifecycle-complete"},{"boundaries",lifecycle_stages},{"walCases",16}}.dump()<<std::endl;}
 if(exit_retirement){verify_exit_archive();check(saw_retained_exit&&saw_retired_exit&&!result.state->blocks.contains(exit_origin),"exit retention/retirement/pruning coverage incomplete");
  std::cout<<Json{{"scenario","capella-exit-archive-retirement"},{"retainedAfterUnfinalizedInclusion",saw_retained_exit},{"retiredAtFinality",saw_retired_exit},{"observedFinalizedEpoch",exit_retired_at},{"admissionOriginPruned",true},{"replayDidNotRegrowArchive",true},{"bothOwnersReopened",true},{"networkTransport",false}}.dump()<<std::endl;
 }
 if(owner_mode){Rpc rpc(*owners[0]);nodes[0]->invalidate();check(rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_blockNumber"},{"params",Json::array()}}).contains("error"),"invalidated owner still served block number");}
 std::cout<<Json{{"scenario","capella-local-assembly-finality"},{"localSyncAggregates",local_sync},{"disjointSyncMerging",local_sync},{"individualSyncMessages",local_sync},{"syncWireWorker",local_sync},{"syncCatalogExport",local_sync},{"syncPrecollection",local_sync},{"localSyncSigning",local_sync},{"executionOwner",owner_mode},{"participationPercent",local_finality},{"blocks",blocks},{"votes",votes},{"finalizedEpoch",result.state->timed.view.finalized.epoch},{"justifiedEpoch",result.state->timed.view.justified.epoch},{"independentStores",2},{"storeReopens",restarts},{"signingJournalsReopened",reopened_journals},{"networkTransport",false},{"networkActivated",false}}.dump()<<std::endl;
}
static void scheduler_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 Temp directory;const auto limits=crash_limits;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;const auto validators=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
 Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
 ChainStore witness(directory/"witness");auto owner=std::make_unique<Blockchain>(directory/"owner",g);auto reactor=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,initial,std::function<void()>{},&witness);
 if(sync_rotation)reactor->set_automatic_pruning(true);
 uint64_t now=initial.timed.clock.time_ms;auto wall=[&]{return now;};auto clock=std::make_unique<CapellaLocalClock>(*reactor,2*rules.slot_duration_ms,wall);
 std::map<uint64_t,std::unique_ptr<ChainStore>> disks;std::map<uint64_t,std::shared_ptr<BeaconSigningJournal>> journals;
 uint64_t signatures=0;std::vector<CapellaValidatorKey> keys;
 auto load_keys=[&](const std::map<uint64_t,ForkCheckpointToken>& saved){for(uint64_t i=0;i<64;++i){disks[i]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(i)));journals[i]=std::make_shared<BeaconSigningJournal>(*disks[i],t.chain_id,genesis.header.hash(),validators,Key(i).public_key,slots,saved.contains(i)?std::optional<ForkCheckpointToken>(saved.at(i)):std::nullopt);keys.push_back({i,journals[i],[&,i](const ConsensusRoot& root){++signatures;return Key(i).sign(root);},genesis.header.coinbase,automatic_registration?Key(i).public_key:Bytes{}});}};load_keys({});
 bool wrong_timing=false;try{CapellaValidatorLoop invalid(*reactor,*clock,rules.slot_duration_ms,slots+1,16,64,keys,wall);}catch(const std::invalid_argument&){wrong_timing=true;}check(wrong_timing,"scheduler accepted wrong epoch duration");
 auto loop=std::make_unique<CapellaValidatorLoop>(*reactor,*clock,rules.slot_duration_ms,slots,16,64,keys,wall);
 if(automatic_registration){
  const auto before=reactor->current().token,journal_before=journals.at(0)->commitment();const auto calls=signatures;
  unsigned stale=0,misconfigured=0;
  const auto& configured=keys.front();
  auto attempt=[&](unsigned kind,const Bytes& registration){
   if(kind==0)reactor->propose_current(1,2,genesis.header.coinbase,16,64,wall,*configured.journal,configured.signer,{},registration);
   if(kind==1)reactor->attest_current(1,2,wall,*configured.journal,configured.signer,{},registration);
   if(kind==2)reactor->sync_current(1,2,wall,*configured.journal,configured.signer,{},registration);
  };
  for(unsigned kind=0;kind<3;++kind){
   try{attempt(kind,Key(0).public_key);}catch(const StaleCapellaDuty&){++stale;}
   try{attempt(kind,Key(1).public_key);}catch(const StaleCapellaDuty&){throw std::runtime_error("wrong journal identity treated as retryable");}catch(const std::invalid_argument&){++misconfigured;}
  }
  check(stale==3&&misconfigured==3&&signatures==calls&&reactor->current().token==before&&journals.at(0)->commitment()==journal_before,"stale automatic index signed or mutated durable state");
  std::cout<<Json{{"scenario","capella-automatic-index-signing-guard"},{"staleIndicesRejected",stale},{"journalMismatchesRejected",misconfigured},{"providerCalls",signatures-calls},{"durableStateUnchanged",true}}.dump()<<std::endl;
 }
 const auto key=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;const auto sender=SignedTransaction::sign(tx,key).sender();
 uint64_t proposals=0,votes=0,sync_messages=0;
 const auto period=t.consensus.epoch.sync.epochs_per_sync_committee_period;check(period&&period<=16,"rotation fixture period must remain bounded");const auto boundary=period*slots;
 const uint64_t stop_slot=sync_rotation?(3*period+3)*slots+1:5*slots+1;
 Bytes previous_committee,previous_next;unsigned rotations=0,committee_changes=0,verified_aggregates=0;size_t maximum_records=0;
 for(uint64_t slot=2;slot<=stop_slot;++slot){
  now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;tx.nonce=owner->state().get(sender).nonce;check(owner->submit(SignedTransaction::sign(tx,key)),"scheduler mempool submit");
  loop->poll();const auto current=reactor->current();const auto& head=capella_fork_head(*current.state,rules);check(head.header.slot==slot&&head.execution_block.transactions.size()==1&&owner->pending_count()==0,"scheduler failed to produce mempool block");
  if(sync_rotation){
   const auto& l=t.consensus.state;const size_t at=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133;
   const size_t width=48*(l.sync_committee_size+1);const auto& state=head.state.ssz;
   Bytes committee(state.begin()+at,state.begin()+at+width),next(state.begin()+at+width,state.begin()+at+2*width);
   if(!previous_committee.empty()&&slot%boundary==0){check(committee==previous_next,"sync committee did not rotate to prior next committee");++rotations;committee_changes+=committee!=previous_committee;std::cout<<Json{{"event","scheduled-sync-rotation"},{"slot",slot},{"currentCommitteeHash",hex(sha256(committee))},{"nextCommitteeHash",hex(sha256(next))}}.dump()<<std::endl;}
   previous_committee=committee;previous_next=next;maximum_records=std::max(maximum_records,current.state->blocks.size());
   if(slot>=3){const auto block=reactor->beacon_read("/eth/v2/beacon/blocks/head").body.at("data").at("message");const auto aggregate=block.at("body").at("sync_aggregate");const auto bits=unhex(aggregate.at("sync_committee_bits").get<std::string>());std::vector<Bytes> members;
    for(size_t i=0;i<l.sync_committee_size;++i){check(bits.at(i/8)&(1u<<(i%8)),"rotation lost sync participation");members.emplace_back(committee.begin()+48*i,committee.begin()+48*(i+1));}
    ConsensusBytes4 version{};std::copy_n(state.begin()+((slot-1)/slots<little(state,56,8)?48:52),4,version.begin());const auto signing=consensus_signing_root(root_at(unhex(block.at("parent_root").get<std::string>()),0),consensus_domain({7,0,0,0},version,root_at(state,8)));
    check(verify_consensus_bls_fast_aggregate(members,signing,unhex(aggregate.at("sync_committee_signature").get<std::string>())),"rotated sync aggregate fails independent BLS check");++verified_aggregates;
   }
  }
  check(!reactor->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"scheduler did not publish sync messages");
  const auto count=signatures;const auto token=current.token;++now;loop->poll();check(signatures==count&&reactor->current().token==token,"same-phase scheduler repeated signing or publication");
  now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms+rules.attestation_due_ms;loop->poll();check(loop->status().at("healthy")==true,"scheduler unhealthy");
  if(slot==(sync_rotation?2*boundary-1:3*slots)){
   proposals+=loop->status().at("proposalsAdmitted").get<uint64_t>();votes+=loop->status().at("attestationsAdmitted").get<uint64_t>();sync_messages+=loop->status().at("syncMessagesAdmitted").get<uint64_t>();loop->stop();clock->stop();loop.reset();clock.reset();reactor.reset();owner.reset();
   std::map<uint64_t,ForkCheckpointToken> saved;for(const auto& [i,journal]:journals)saved.emplace(i,journal->commitment());keys.clear();journals.clear();disks.clear();load_keys(saved);
   owner=std::make_unique<Blockchain>(directory/"owner",g);reactor=std::make_unique<CapellaDurableReactor>(*owner,rules,limits,witness);clock=std::make_unique<CapellaLocalClock>(*reactor,2*rules.slot_duration_ms,wall);loop=std::make_unique<CapellaValidatorLoop>(*reactor,*clock,rules.slot_duration_ms,slots,16,64,keys,wall);if(sync_rotation){reactor->set_automatic_pruning(true);loop->poll();check(!reactor->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"restart before rotation lost sync duties");}
  }
 }
 const auto current=reactor->current();check(current.state->timed.view.finalized.epoch>=3&&owner->state().get(sender).nonce==Int(std::to_string(stop_slot)),"scheduled chain did not finalize all transactions");
 const auto finalized_header=reactor->beacon_read("/eth/v1/beacon/headers/finalized");const auto finalized_root=current.state->timed.view.finalized.root;check(finalized_header.status==200&&finalized_header.body.at("finalized")==true&&finalized_header.body.at("data").at("root")==hex(Bytes(finalized_root.begin(),finalized_root.end())),"scheduled finalized Beacon header mismatch");
 proposals+=loop->status().at("proposalsAdmitted").get<uint64_t>();votes+=loop->status().at("attestationsAdmitted").get<uint64_t>();sync_messages+=loop->status().at("syncMessagesAdmitted").get<uint64_t>();check(proposals==stop_slot-1&&votes>0&&sync_messages>0,"scheduler duty counters");
 if(sync_rotation)check(rotations==3&&committee_changes>=2&&verified_aggregates==stop_slot-2&&maximum_records<=5*slots&&current.state->timed.view.finalized.epoch>=3*period,"rotation/finality/pruning qualification incomplete");
 bool cancelled=false;try{loop->poll([]{throw ExecutionCancelled("scheduler cancellation");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&loop->status().at("healthy")==true&&clock->healthy(),"cancellation poisoned scheduler");
 // Exercise the real worker lifecycle with a stable clock and already completed duties.
 const auto prior=signatures;const auto polls_before=loop->status().at("timing").at("polls").get<uint64_t>();loop->start(std::chrono::milliseconds(1));std::this_thread::sleep_for(std::chrono::milliseconds(20));loop->stop();check(signatures==prior&&loop->status().at("healthy")==true&&loop->status().at("timing").at("polls").get<uint64_t>()>polls_before,"worker failed to poll or duplicated completed duty");
 now-=1;bool rejected=false;try{clock->poll();}catch(const std::runtime_error&){rejected=true;}check(rejected&&!clock->healthy(),"backward clock accepted");bool closed=false;try{owner->tip();}catch(const std::runtime_error&){closed=true;}check(closed,"clock failure left execution readable");bool beacon_closed=false;try{reactor->beacon_read("/eth/v1/beacon/headers/head");}catch(const std::runtime_error&){beacon_closed=true;}check(beacon_closed,"clock failure left Beacon readable");
 std::cout<<Json{{"scenario",sync_rotation?"capella-scheduled-sync-rotation":"capella-scheduled-finality"},{"rotations",rotations},{"committeeChanges",committee_changes},{"restartSlot",sync_rotation?2*boundary-1:3*slots},{"verifiedSyncAggregates",verified_aggregates},{"maximumRetainedRecords",maximum_records},{"syncMessages",sync_messages},{"blocks",proposals},{"votes",votes},{"finalizedEpoch",current.state->timed.view.finalized.epoch},{"witnessRestart",true},{"signingJournalsReopened",64},{"duplicatePollNoSignature",true},{"backwardsClockClosedOwner",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void runtime_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 Temp directory;const auto limits=crash_limits;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;auto owner=std::make_unique<Blockchain>(directory/"owner",g);
 const auto profile=capella_fork_profile(rules).dump(),snapshot=capella_store_snapshot(initial,rules,limits);{std::ofstream f(directory/"profile.json");f<<profile;}{std::ofstream f(directory/"initial.json");f<<snapshot;}
 Json config{{"profile",{{"file",(directory/"profile.json").string()},{"sha256",hex(sha256(Bytes(profile.begin(),profile.end())))},{"maximumBytes","0x100000"}}},{"limits",{{"envelopeBytes",quantity(Int(std::to_string(limits.envelope_bytes)))},{"blocks",quantity(Int(std::to_string(limits.blocks)))},{"messages",quantity(Int(std::to_string(limits.messages)))},{"equivocations",quantity(Int(std::to_string(limits.equivocations)))},{"stateBytes",quantity(Int(std::to_string(limits.state_bytes)))},{"totalStateBytes",quantity(Int(std::to_string(limits.total_state_bytes)))},{"replaySlots",quantity(Int(std::to_string(limits.replay_slots)))},{"syncTrials",quantity(Int(std::to_string(limits.sync_trials)))}}},{"initial",{{"file",(directory/"initial.json").string()},{"sha256",hex(sha256(Bytes(snapshot.begin(),snapshot.end())))}}},{"restart",nullptr},{"recoveryWitness",(directory/"witness").string()},{"clock",{{"intervalMs","0x3e8"},{"maximumForwardStepMs",quantity(Int(std::to_string(2*rules.slot_duration_ms)))}}}};
 if(!cli_binary.empty()){
  owner.reset();config["clock"]["maximumForwardStepMs"]="0x1d4c0";config["catalogs"]={{"handles","0x2"},{"rootBytes","0x10000"},{"ttlMs","0xea60"}};
  {std::ofstream f(directory/"genesis.json");f<<cli_genesis;}{std::ofstream f(directory/"serve.json");f<<Json{{"capella",config}};}
  if(managed_cli){
   const std::string key_env="NANO_TEST_MANAGED_CAPELLA_KEY";
   config["validator"]={{"pollIntervalMs","0x3e8"},{"maximumTransactionTrials","0x10"},{"maximumVoteTrials","0x40"},{"keys",Json::array({Json{{"index","0x0"},{"feeRecipient",genesis.header.coinbase},{"keyEnv",key_env},{"journal",(directory/"managed-key").string()},{"restart",nullptr},{"recoveryWitness",(directory/"managed-key-witness").string()}}})}};
   const uint64_t block_budget=t.consensus.maximum_block_bytes,vote_budget=228+(uint64_t(t.consensus.body.validators_per_committee)+8)/8,slash_budget=464+16*uint64_t(t.consensus.body.validators_per_committee),largest=std::max({block_budget,vote_budget,slash_budget});
   config["admission"]={{"frameBytes",quantity(Int(std::to_string(1024+2*largest)))},{"blockBytes",quantity(Int(std::to_string(block_budget)))},{"attestationBytes",quantity(Int(std::to_string(vote_budget)))},{"slashingBytes",quantity(Int(std::to_string(slash_budget)))},{"jobs","0x2"},{"payloadBytes",quantity(Int(std::to_string(2*largest)))}};
   Json node{{"capella",config},{"rpc_port",0},{"listen_addr","/ip4/127.0.0.1/tcp/0"},{"p2p_binary",daemon_binary.empty()?fs::absolute(executable).string():daemon_binary}};
   if(network_cli){
    node["capella"]["network"]={{"enabled",true},{"pageEntries","0x8"},{"maximumEntries","0x400"},{"dependencyBlocks","0x100"},{"dependencyBytes","0x1000000"},{"acknowledgedRoots","0x400"}};
    node["capella"]["clock"]["intervalMs"]="0x64";node["capella"]["validator"]["pollIntervalMs"]="0x64";node["capella"]["validator"]["keys"]=Json::array();
    Env env{{"NANO_TEST_NETWORK_TRACE","1"}};
    for(uint64_t i=0;i<64;++i){const auto name="NANO_TEST_CAPELLA_NETWORK_KEY_"+std::to_string(i);env[name]=hex(bytes(Int(std::to_string(i+1)),32));node["capella"]["validator"]["keys"].push_back({{"index",quantity(Int(std::to_string(i)))},{"feeRecipient",genesis.header.coinbase},{"keyEnv",name},{"journal",(directory/("network-key-"+std::to_string(i))).string()},{"restart",nullptr},{"recoveryWitness",(directory/("network-key-witness-"+std::to_string(i))).string()}});}
    if(!network_export.empty()){
     check(!fs::exists(network_export),"network fixture destination exists");fs::create_directories(network_export);
     fs::copy_file(directory/"profile.json",network_export/"profile.json");fs::copy_file(directory/"initial.json",network_export/"initial.json");save(network_export/"genesis.json",cli_genesis);
     const auto all=node["capella"]["validator"]["keys"];
     for(size_t n=0;n<2;++n){auto out=node;auto& c=out["capella"];c["limits"]["blocks"]="0x100";c["limits"]["messages"]="0x800";c["limits"]["totalStateBytes"]="0x4000000";c["limits"]["envelopeBytes"]="0x8000000";c["profile"]["file"]="/config/profile.json";c["initial"]["file"]="/config/initial.json";c["recoveryWitness"]="/var/lib/nano/witness";c["validator"]["keys"]=Json::array();Json variables={{"NANO_TEST_NETWORK_TRACE","1"}};
      for(size_t i=n*32;i<(n+1)*32;++i){auto entry=all[i];entry["journal"]="/var/lib/nano/key-"+std::to_string(i);entry["recoveryWitness"]="/var/lib/nano/key-witness-"+std::to_string(i);c["validator"]["keys"].push_back(entry);const auto name=entry.at("keyEnv").get<std::string>();variables[name]=*env.at(name);}
      out["rpc_host"]="127.0.0.1";out["rpc_port"]=8545;out["p2p_binary"]="/usr/local/bin/nano-p2p";out["listen_addr"]="/ip4/0.0.0.0/tcp/30303";out["p2p_identity"]="/var/lib/nano/peer.key";out["bootnodes"]=Json::array();
      save(network_export/("node-"+std::to_string(n)+".json"),out);save(network_export/("env-"+std::to_string(n)+".json"),variables);
     }
     Json meta{{"syntheticValidators",64},{"nodes",2},{"chainId",1337},{"fork","Capella/Shanghai"},{"secondsPerSlot",t.seconds_per_slot},{"slotsPerEpoch",slots}};
     if(exit_network){const auto& head=capella_fork_head(initial,rules);const auto& seed=head.state.ssz;ConsensusBytes4 version{};std::copy_n(seed.begin()+(little(seed,56,8)>0?48:52),4,version.begin());
      SignedConsensusVoluntaryExit operation{0,0,{}};operation.signature=Key(0).sign(consensus_signing_root(consensus_voluntary_exit_root(0,0),consensus_domain({4,0,0,0},version,root_at(seed,8))));
      save(network_export/"exit.json",Json{{"kind","voluntary_exit"},{"ssz",hex(encode_consensus_voluntary_exit_ssz(operation))},{"validatorIndex",0},{"epoch",0},{"recipient","0x000000000000000000000000000000000000002a"}});
      const auto root=consensus_beacon_header_root(head.header);meta["initialHead"]=hex(Bytes(root.begin(),root.end()));meta["exitTest"]=true;meta["testWithdrawalDelayEpochs"]=t.consensus.exit.minimum_withdrawability_delay;
     }
     save(network_export/"fixture.json",meta);
     std::cout<<Json{{"event","capella-network-fixture-exported"},{"path",network_export.string()},{"syntheticValidators",64}}.dump()<<std::endl;return;
    }
    if(distributed_cli){
     const auto all_keys=node["capella"]["validator"]["keys"];auto left=node,right=node;left["capella"]["validator"]["keys"]=Json::array();right["capella"]["validator"]["keys"]=Json::array();Env left_env{{"NANO_TEST_NETWORK_TRACE","1"}},right_env=left_env;
     for(size_t i=0;i<all_keys.size();++i){auto& cfg=i<32?left:right;auto& secrets=i<32?left_env:right_env;cfg["capella"]["validator"]["keys"].push_back(all_keys[i]);const auto name=all_keys[i].at("keyEnv").get<std::string>();secrets[name]=env.at(name);}
     right["capella"]["recoveryWitness"]=(directory/"right-witness").string();save(directory/"left.json",left);
     Process a({cli_binary,"run",(directory/"genesis.json").string(),(directory/"left-db").string(),(directory/"left.json").string()},left_env);const auto ap=a.port();const auto address=a.event("listen").at("address");check(a.event("altair-ipc-ready").at("networkProtocolActive")==true,"left protocol inactive");right["bootnodes"]=Json::array({address});save(directory/"right.json",right);
     Process b({cli_binary,"run",(directory/"genesis.json").string(),(directory/"right-db").string(),(directory/"right.json").string()},right_env);const auto bp=b.port();check(b.event("altair-ipc-ready").at("networkProtocolActive")==true,"right protocol inactive");
     auto beacon=[](uint16_t port,const std::string& path){namespace http=boost::beast::http;asio::io_context io;boost::beast::tcp_stream stream(io);boost::beast::flat_buffer buffer;http::request<http::empty_body> request(http::verb::get,path,11);request.set(http::field::host,"127.0.0.1");request.set(http::field::accept,"application/json");http::response_parser<http::string_body> parser;parser.body_limit(1024*1024);boost::beast::error_code failure;stream.expires_after(std::chrono::seconds(5));stream.async_connect({asio::ip::make_address("127.0.0.1"),port},[&](auto error){if(error){failure=error;return;}http::async_write(stream,request,[&](auto error,size_t){if(error){failure=error;return;}http::async_read(stream,buffer,parser,[&](auto error,size_t){failure=error;});});});io.run();check(!failure,"Beacon HTTP transport failed");auto response=parser.release();if(response.result()!=http::status::ok)throw std::runtime_error("Beacon HTTP response "+std::to_string(response.result_int())+" for "+path);return Json::parse(response.body());};
     struct SuspendedTransport {
      pid_t left=-1,right=-1;bool left_stopped=false,right_stopped=false;
      void resume()noexcept{if(left_stopped){::kill(left,SIGCONT);left_stopped=false;}if(right_stopped){::kill(right,SIGCONT);right_stopped=false;}}
      ~SuspendedTransport(){resume();}
      void pause(){check(left>0&&right>0&&left!=right,"invalid child P2P identities");check(::kill(left,SIGSTOP)==0,"stop left transport");left_stopped=true;check(::kill(right,SIGSTOP)==0,"stop right transport");right_stopped=true;}
     } transport;
     transport.left=a.event("p2p-process").at("pid").get<pid_t>();transport.right=b.event("p2p-process").at("pid").get<pid_t>();
     const auto sync_period=t.consensus.epoch.sync.epochs_per_sync_committee_period;check(sync_period&&sync_period<=16,"distributed rotation fixture period unbounded");
     bool complete=false;uint64_t previous=UINT64_MAX,minimum_finalized=distributed_sync_rotation?2*sync_period+1:2;unsigned stable=0;uint64_t sync_after_slot=0;
     std::optional<Bytes> rotation_state;std::string qualified_finalized_root;
     std::set<std::string> sync_checked,remote_sync_blocks;std::map<std::string,Json> observed_sync_blocks;std::map<std::string,unsigned> sync_observers;
     auto qualify_sync=[&](const std::string& finalized_root){
      const auto& l=t.consensus.state;const auto& seed=rotation_state?*rotation_state:initial.blocks.at(initial.timed.view.anchor).state.ssz;
      const size_t committee=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133;
      auto root=finalized_root;
      for(size_t depth=0;depth<8&&root!=hex(Bytes(initial.timed.view.anchor.begin(),initial.timed.view.anchor.end()));++depth){
       const auto found=observed_sync_blocks.find(root);if(found==observed_sync_blocks.end())break;
       const auto& message=found->second.at("message");const auto id=root;root=message.at("parent_root").get<std::string>();if(sync_observers[id]!=3||!sync_checked.insert(id).second)continue;
       const auto slot=std::stoull(message.at("slot").get<std::string>()),proposer=std::stoull(message.at("proposer_index").get<std::string>());
       const auto period=t.consensus.epoch.sync.epochs_per_sync_committee_period;
       if(transport_outage_cli&&slot<=sync_after_slot)continue;
       if(slot/slots/period!=little(seed,40,8)/slots/period)continue; // Committee must come from an authenticated state in the same period.
       const auto& aggregate=message.at("body").at("sync_aggregate");const auto bits=unhex(aggregate.at("sync_committee_bits").get<std::string>()),signature=unhex(aggregate.at("sync_committee_signature").get<std::string>());check(bits.size()==(l.sync_committee_size+7)/8,"sync bitvector width");
       std::vector<Bytes> keys;size_t remote=0;
       for(size_t position=0;position<l.sync_committee_size;++position)if(bits[position/8]&(1u<<(position%8))){Bytes key(seed.begin()+committee+48*position,seed.begin()+committee+48*(position+1));size_t member=0;for(;member<64&&Key(member).public_key!=key;++member){}check(member<64,"unknown distributed sync fixture key");remote+=(member<32)!=(proposer<32);keys.push_back(std::move(key));}
       if(!keys.empty()){ConsensusBytes4 version{};std::copy_n(seed.begin()+((slot-1)/slots<little(seed,56,8)?48:52),4,version.begin());ConsensusRoot parent{};copy_to(parent,unhex(root));const auto signing=consensus_signing_root(parent,consensus_domain({7,0,0,0},version,root_at(seed,8)));check(verify_consensus_bls_fast_aggregate(keys,signing,signature),"distributed sync signature invalid");}
       if(remote)remote_sync_blocks.insert(id);
       std::cout<<Json{{"event","distributed-sync-finalized"},{"root",id},{"slot",slot},{"proposer",proposer},{"participants",keys.size()},{"remotePositions",remote}}.dump()<<std::endl;
      }
     };

     auto head=[&](uint16_t port){return beacon(port,"/eth/v1/beacon/headers/head").at("data");};
     auto slot_of=[](const Json& value){return std::stoull(value.at("header").at("message").at("slot").get<std::string>());};

     try{
      if(transport_outage_cli){
       // Suspend only the child daemons: validator duties, clocks and stores remain live.
       // This qualifies a transport-process outage, not packet loss with running daemons.
       const uint64_t rotation_boundary=2*sync_period*slots;
       const uint64_t outage_start=distributed_sync_rotation?rotation_boundary-slots:slots+2;
       until([&]{a.pump(0);b.pump(0);const auto ha=head(ap),hb=head(bp);return slot_of(ha)>=outage_start&&slot_of(hb)>=outage_start&&ha.at("root")==hb.at("root");},"distributed peers reached outage start",(distributed_sync_rotation?outage_start+2*slots:2*slots)*t.seconds_per_slot);
       transport.pause();const auto before_a=head(ap),before_b=head(bp);const auto cutoff=std::max(slot_of(before_a),slot_of(before_b))/slots;
       if(distributed_sync_rotation)check(slot_of(before_a)<rotation_boundary&&slot_of(before_b)<rotation_boundary,"outage started after target rotation");
       std::cout<<Json{{"event","capella-transport-outage-start"},{"leftHead",before_a},{"rightHead",before_b},{"maximumFinalizedEpochDuringOutage",cutoff},{"durationSeconds",96}}.dump()<<std::endl;
       const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(96);
       while(std::chrono::steady_clock::now()<end){a.pump(0);b.pump(0);check(a.running()&&b.running(),"validator exited during transport outage");
        for(auto port:{ap,bp}){const auto cp=beacon(port,"/eth/v1/beacon/states/head/finality_checkpoints").at("data").at("finalized");check(std::stoull(cp.at("epoch").get<std::string>())<=cutoff,"isolated half-stake advanced finality beyond cutoff");}
        std::this_thread::sleep_for(std::chrono::seconds(1));
       }
       const auto after_a=head(ap),after_b=head(bp);check(slot_of(after_a)>slot_of(before_a)&&slot_of(after_b)>slot_of(before_b),"isolated validators did not continue proposing");check(after_a.at("root")!=after_b.at("root"),"outage did not produce distinct branches");
       if(distributed_sync_rotation)check(slot_of(after_a)>=rotation_boundary&&slot_of(after_b)>=rotation_boundary,"isolated branches did not cross target rotation");
       sync_after_slot=std::max(slot_of(after_a),slot_of(after_b))+1;
       minimum_finalized=std::max(slot_of(after_a),slot_of(after_b))/slots+(distributed_sync_rotation?1:0);transport.resume();
       std::cout<<Json{{"event","capella-transport-resumed"},{"leftHead",after_a},{"rightHead",after_b},{"minimumRecoveredFinalizedEpoch",minimum_finalized},{"syncInclusionMustExceedSlot",sync_after_slot}}.dump()<<std::endl;
      }
      const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(distributed_sync_rotation?(2*sync_period+4)*slots*t.seconds_per_slot+120:transport_outage_cli?240:210);while(std::chrono::steady_clock::now()<deadline){a.pump(0);b.pump(0);check(a.running()&&b.running(),"distributed validator process exited");const auto fa=beacon(ap,"/eth/v1/beacon/states/head/finality_checkpoints").at("data").at("finalized"),fb=beacon(bp,"/eth/v1/beacon/states/head/finality_checkpoints").at("data").at("finalized");const auto ea=std::stoull(fa.at("epoch").get<std::string>()),eb=std::stoull(fb.at("epoch").get<std::string>());if(ea&&ea==eb)check(fa.at("root")==fb.at("root"),"conflicting finalized roots");
      if(sync_distributed_cli)for(auto port:{ap,bp}){const auto h=beacon(port,"/eth/v1/beacon/headers/head").at("data");const auto root=h.at("root").get<std::string>();const unsigned observer=port==ap?1:2;if(!(sync_observers[root]&observer)){const auto block=beacon(port,"/eth/v2/beacon/blocks/"+root).at("data");const auto& m=block.at("message");const auto& header=h.at("header").at("message");for(const auto field:{"slot","proposer_index","parent_root","state_root"})check(m.at(field)==header.at(field),"observed sync block/header mismatch");if(observed_sync_blocks.contains(root))check(observed_sync_blocks.at(root)==block,"peers disagree on signed sync block");else observed_sync_blocks.emplace(root,block);sync_observers[root]|=observer;}}
      if(std::min(ea,eb)!=previous){previous=std::min(ea,eb);std::cout<<Json{{"event","distributed-finality-observation"},{"left",fa},{"right",fb}}.dump()<<std::endl;}
      if(ea>=minimum_finalized&&fa==fb){const auto ha=beacon(ap,"/eth/v1/beacon/headers/finalized"),hb=beacon(bp,"/eth/v1/beacon/headers/finalized");if(ha.at("finalized")==true&&hb.at("finalized")==true&&ha.at("data").at("root")==fa.at("root")&&hb.at("data").at("root")==fa.at("root")){qualified_finalized_root=fa.at("root").get<std::string>();if(sync_distributed_cli&&!distributed_sync_rotation)qualify_sync(qualified_finalized_root);if(++stable>=3&&(distributed_sync_rotation||!sync_distributed_cli||remote_sync_blocks.size()>=3)){complete=true;break;}}else stable=0;}else stable=0;
      std::this_thread::sleep_for(std::chrono::seconds(1));}check(complete,"distributed Capella finality did not reach required common epoch");
     }catch(...){transport.resume();const auto ar=a.stop(),br=b.stop();std::cerr<<ar.out<<ar.err<<br.out<<br.err;throw;}
     const auto ar=a.stop(),br=b.stop();check(ar.code==0&&br.code==0,"distributed nodes did not stop cleanly");std::cout<<ar.out<<br.out;
     if(distributed_sync_rotation){
      ConsensusRoot finalized{};copy_to(finalized,unhex(qualified_finalized_root));
      for(const auto& [db,witness_path]:std::array<std::pair<std::string,std::string>,2>{{{"left-db","witness"},{"right-db","right-witness"}}}){
       Blockchain reopened(directory/db,g);ChainStore recovered_witness(directory/witness_path);CapellaDurableReactor recovered(reopened,rules,limits,recovered_witness);const auto durable=recovered.current();
       const auto found=durable.state->blocks.find(finalized);check(found!=durable.state->blocks.end(),"finalized rotation state missing after reopen");const auto& block=found->second;
       check(capella_beacon_state_root(block.state.ssz,t.consensus.state,t.execution,t.maximum_state_bytes)==block.header.state_root,"rotated state commitment mismatch");
       if(rotation_state)check(*rotation_state==block.state.ssz,"peers disagree on finalized rotated state");else rotation_state=block.state.ssz;
      }
      const auto& l=t.consensus.state;const size_t at=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+133,width=48*(l.sync_committee_size+1);const auto& seed=initial.blocks.at(initial.timed.view.anchor).state.ssz;
      Bytes committee(rotation_state->begin()+at,rotation_state->begin()+at+width),original(seed.begin()+at,seed.begin()+at+width);check(committee!=original,"distributed run never changed sync committee");
      check(little(*rotation_state,40,8)/slots/sync_period>=2,"rotation state predates composition change");qualify_sync(qualified_finalized_root);check(remote_sync_blocks.size()>=3,"rotated finalized blocks lack verified remote sync");
      std::cout<<Json{{"event","distributed-sync-rotation-verified"},{"finalizedRoot",qualified_finalized_root},{"stateSlot",little(*rotation_state,40,8)},{"committeeHash",hex(sha256(committee))},{"initialCommitteeHash",hex(sha256(original))},{"bothStoresReopened",true}}.dump()<<std::endl;
     }
     std::cout<<Json{{"scenario",transport_outage_cli?"capella-transport-outage-recovery":"capella-distributed-finality"},{"validatorsPerNode",32},{"nodes",2},{"minimumFinalizedEpoch",minimum_finalized},{"stableObservations",stable},{"syncQualification",sync_distributed_cli},{"syncRotation",distributed_sync_rotation},{"syncInclusionMustExceedSlot",sync_after_slot},{"finalizedBlocksWithRemoteSync",remote_sync_blocks.size()},{"realLibp2p",true}}.dump()<<std::endl;return;
    }
    const auto source_config=directory/"source.json";save(source_config,node);Process source_node({cli_binary,"run",(directory/"genesis.json").string(),(directory/"source-db").string(),source_config.string()},env);const auto source_port=source_node.port();const auto source_address=source_node.event("listen").at("address").get<std::string>();check(source_node.event("altair-ipc-ready").at("networkProtocolActive")==true,"source network inactive");
    until([&]{source_node.pump(0);return quantity(rpc(source_port,"eth_blockNumber"))>=2;},"source produced before follower",40);
    if(multipeer_cli){
     constexpr size_t followers=5;
     std::vector<std::unique_ptr<Process>> peers;std::vector<uint16_t> ports;std::vector<std::string> identities;
     struct PausedDaemon {pid_t pid=-1;bool paused=false;void resume()noexcept{if(paused){::kill(pid,SIGCONT);paused=false;}}~PausedDaemon(){resume();}} slow;
     auto stop_all=[&]{slow.resume();for(auto& peer:peers){const auto result=peer->stop();std::cout<<result.out<<result.err;check(result.code==0,"competing peer failed shutdown");}const auto result=source_node.stop();std::cout<<result.out<<result.err;check(result.code==0,"source failed shutdown");};
     try{
      for(size_t i=0;i<followers;++i){
       auto cfg=node;cfg["capella"].erase("validator");cfg["capella"]["recoveryWitness"]=(directory/("peer-witness-"+std::to_string(i))).string();cfg["bootnodes"]=Json::array({source_address});const auto file=directory/("peer-"+std::to_string(i)+".json");save(file,cfg);
       peers.push_back(std::make_unique<Process>(std::vector<std::string>{cli_binary,"run",(directory/"genesis.json").string(),(directory/("peer-db-"+std::to_string(i))).string(),file.string()},Env{{"NANO_TEST_NETWORK_TRACE","1"}}));
       auto& peer=*peers.back();ports.push_back(peer.port());const auto address=peer.event("listen").at("address").get<std::string>();identities.push_back(address.substr(address.rfind("/p2p/")+5));check(peer.event("altair-ipc-ready").at("networkProtocolActive")==true,"competing peer inactive");source_node.pump(0);
       if(i==0){
        slow.pid=peer.event("p2p-process").at("pid").get<pid_t>();check(slow.pid>1,"invalid slow daemon pid");
        until([&]{source_node.pump(0);peer.pump(0);std::istringstream events(source_node.output());std::string text;while(std::getline(events,text)){const auto event=Json::parse(text,nullptr,false);if(event.is_object()&&event.value("event","")=="test-capella-catalog-relay"&&event.value("peerId","")==identities[0])return true;}return false;},"slow peer completed initial relay before pause",30);
        check(::kill(slow.pid,SIGSTOP)==0,"could not pause slow daemon");slow.paused=true;
       }
      }
      const auto pause_start=std::chrono::steady_clock::now();const auto floor=rpc(source_port,"eth_getBlockByNumber",Json::array({"latest",false}));const auto floor_number=quantity(floor.at("number"));
      std::vector<bool> progressed(followers,false);std::vector<std::set<std::string>> kinds(followers);std::vector<size_t> attempts(followers,0);
      // Measure only work completed after all contenders have started.
      const auto before=source_node.output().size();
      const auto deadline=pause_start+std::chrono::seconds(40);
      while(std::chrono::steady_clock::now()<deadline){
       source_node.pump(0);check(source_node.running(),"source exited with slow peer");for(auto& peer:peers){peer->pump(0);check(peer->running(),"peer core exited under contention");}
       for(size_t i=1;i<followers;++i){const auto head=rpc(ports[i],"eth_getBlockByNumber",Json::array({"latest",false}));if(quantity(head.at("number"))>floor_number){const auto expected=rpc(source_port,"eth_getBlockByNumber",Json::array({head.at("number"),false}));if(!expected.is_null()&&head.at("hash")==expected.at("hash")&&head.at("stateRoot")==expected.at("stateRoot"))progressed[i]=true;}}
       std::this_thread::sleep_for(std::chrono::milliseconds(250));
      }
      const auto during=source_node.output().substr(before);std::istringstream lines(during);std::string line;
      while(std::getline(lines,line)){auto event=Json::parse(line,nullptr,false);if(!event.is_object()||event.value("event","")!="test-capella-catalog-relay")continue;for(size_t i=0;i<followers;++i)if(event.value("peerId","")==identities[i]){++attempts[i];kinds[i].insert(event.at("kind").get<std::string>());}}
      check(attempts[0]>0,"paused peer did not exercise an active relay attempt");
      for(size_t i=1;i<followers;++i)check(progressed[i]&&attempts[i]>0&&kinds[i].size()==4,"healthy peer starved while daemon paused");
      std::cout<<Json{{"event","capella-competing-peers-progress"},{"followers",followers},{"pausedDaemonSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-pause_start).count()},{"floorNumber",floor.at("number")},{"completedAttempts",attempts},{"healthyPeersProgressed",followers-1},{"allHealthyKindsScheduled",true}}.dump()<<std::endl;
      slow.resume();
      until([&]{source_node.pump(0);for(auto& p:peers)p->pump(0);const auto target=rpc(source_port,"eth_getBlockByNumber",Json::array({"latest",false}));for(auto port:ports){const auto head=rpc(port,"eth_getBlockByNumber",Json::array({"latest",false}));if(head.at("hash")!=target.at("hash")||head.at("stateRoot")!=target.at("stateRoot"))return false;}return true;},"all competing peers recovered common execution head",60);
      stop_all();std::cout<<Json{{"scenario","capella-competing-peers"},{"nodes",followers+1},{"healthyPeersProgressedDuringPause",followers-1},{"allKindsScheduled",true},{"allPeersRecovered",true},{"realLibp2p",true},{"distributedFinalityQualified",false}}.dump()<<std::endl;return;
     }catch(...){slow.resume();for(auto& peer:peers){const auto result=peer->stop();std::cerr<<result.out<<result.err;}const auto result=source_node.stop();std::cerr<<result.out<<result.err;throw;}
    }
    auto follower_config=node;follower_config["capella"].erase("validator");follower_config["capella"]["recoveryWitness"]=(directory/"follower-witness").string();follower_config["bootnodes"]=Json::array({source_address});const auto follower_file=directory/"follower.json";save(follower_file,follower_config);
    Env follower_env{{"NANO_TEST_NETWORK_TRACE","1"}};
    if(queued_bls_changes){SignedCapellaBlsChange change;Key withdrawal_key(1001);change.validator_index=1;copy_to(change.from_bls_pubkey,withdrawal_key.public_key);change.to_execution_address[19]=43;const auto validators_root=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);copy_to(change.signature,withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,validators_root))));follower_env["NANO_TEST_BLS_BOOTSTRAP"]=hex(encode_capella_bls_change(change));follower_env["NANO_TEST_BLS_DAEMON"]=daemon_binary;follower_config["p2p_binary"]=fs::absolute(executable).string();save(follower_file,follower_config);}
    if(exit_network){
     // Test-only supervisor child submits to the non-validator before execing the real daemon.
     // The producer can only learn this operation over libp2p; no direct DB injection.
     const auto& seed=initial.blocks.at(initial.timed.view.anchor).state.ssz;
     SignedConsensusVoluntaryExit operation;operation.epoch=0;operation.validator_index=0;ConsensusBytes4 version{};
     std::copy_n(seed.begin()+(operation.epoch<little(seed,56,8)?48:52),4,version.begin());
     operation.signature=Key(0).sign(consensus_signing_root(consensus_voluntary_exit_root(0,0),consensus_domain({4,0,0,0},version,root_at(seed,8))));
     follower_env["NANO_TEST_EXIT_BOOTSTRAP"]=hex(encode_consensus_voluntary_exit_ssz(operation));follower_env["NANO_TEST_BLS_DAEMON"]=daemon_binary;
     follower_config["p2p_binary"]=fs::absolute(executable).string();save(follower_file,follower_config);
    }
    Process follower_node({cli_binary,"run",(directory/"genesis.json").string(),(directory/"follower-db").string(),follower_file.string()},follower_env);const auto follower_port=follower_node.port();const auto follower_address=follower_node.event("listen").at("address").get<std::string>();check(follower_node.event("altair-ipc-ready").at("networkProtocolActive")==true,"follower network inactive");
    until([&]{source_node.pump(0);follower_node.pump(0);const auto a=rpc(source_port,"eth_getBlockByNumber",Json::array({"latest",false}));const auto b=rpc(follower_port,"eth_getBlockByNumber",Json::array({"latest",false}));return quantity(a.at("number"))>=2&&a.at("hash")==b.at("hash")&&a.at("stateRoot")==b.at("stateRoot");},"two real Capella peers converged",50);
    if(queued_bls_changes){until([&]{source_node.pump(0);follower_node.pump(0);const auto params=Json::array({"0x000000000000000000000000000000000000002b","latest"});return quantity(rpc(source_port,"eth_getBalance",params))>0&&quantity(rpc(follower_port,"eth_getBalance",params))>0;},"BLS change relayed from non-validator and withdrawn on both peers",60);std::cout<<Json{{"event","capella-bls-network-withdrawal"},{"submittedOnlyToFollower",true},{"bothPeersPaid",true},{"realLibp2p",true}}.dump()<<std::endl;follower_config["p2p_binary"]=daemon_binary;}
    if(exit_network){
     const std::string recipient="0x000000000000000000000000000000000000002a";
     uint64_t paid_height=0;Int paid_balance=0;Json finalized;
     try{
      until([&]{source_node.pump(0);follower_node.pump(0);check(source_node.running()&&follower_node.running(),"exit network node stopped");
       const auto balance=quantity(rpc(source_port,"eth_getBalance",Json::array({recipient,"latest"})));
       if(balance<=Int("30000000000000000000"))return false;
       paid_balance=balance;paid_height=u64(quantity(rpc(source_port,"eth_blockNumber")));return true;
      },"full exit withdrawal observed on producer",480);
      until([&]{source_node.pump(0);follower_node.pump(0);
       const auto a=rpc(source_port,"eth_getBlockByNumber",Json::array({"finalized",false})),b=rpc(follower_port,"eth_getBlockByNumber",Json::array({"finalized",false}));
       if(a.is_null()||b.is_null()||u64(quantity(a.at("number")))<paid_height||a.at("hash")!=b.at("hash"))return false;
       check(a.at("stateRoot")==b.at("stateRoot"),"exit finality state mismatch");
       const auto final_balance=quantity(rpc(source_port,"eth_getBalance",Json::array({recipient,a.at("number")})));
       check(final_balance>=paid_balance,"finalized balance lost withdrawal");
       check(quantity(rpc(follower_port,"eth_getBalance",Json::array({recipient,a.at("number")})))==final_balance,"peers disagree on finalized withdrawal balance");
       std::cout<<Json{{"event","capella-exit-finalized-payment"},{"firstObservedBalance",quantity(paid_balance)},{"finalizedBalance",quantity(final_balance)},{"number",a.at("number")}}.dump()<<std::endl;paid_balance=final_balance;
       finalized=a;return true;
      },"exit withdrawal finalized identically on both peers",180);
     }catch(...){const auto a=source_node.stop(),b=follower_node.stop();std::cerr<<a.out<<a.err<<b.out<<b.err;throw;}
     const auto a=source_node.stop(),b=follower_node.stop();std::cout<<a.out<<a.err<<b.out<<b.err;check(a.code==0&&b.code==0,"exit peers did not stop cleanly");
     // Reopen both durable owners and account for every canonical payout, including
     // the initial partial withdrawal. Only index 0 owns this fixture recipient.
     for(const auto& [db,witness]:std::array<std::pair<std::string,std::string>,2>{{{"source-db","witness"},{"follower-db","follower-witness"}}}){
      Blockchain reopened(directory/db,g);ChainStore recovered_witness(directory/witness);CapellaDurableReactor recovered(reopened,rules,limits,recovered_witness);
      const auto current=recovered.current();const auto& head=capella_fork_head(*current.state,rules);const auto& state=head.state.ssz;
      const auto& layout=t.consensus.state;const size_t hist=176+64*layout.slots_per_historical_root;
      const auto registry=little(state,hist+88,4),balances=little(state,hist+92,4);
      const auto exit_epoch=little(state,registry+105,8),residual=little(state,balances,8);check(exit_epoch<=little(state,40,8)/slots,"validator still active after withdrawal");
      check(recovered.pending_exits().empty(),"finalized exit remained pending");
      Int total=0,finalized_total=0;size_t payments=0,capital_payments=0;const auto final_height=u64(quantity(finalized.at("number")));
      for(uint64_t height=0;height<=u64(head.execution_block.header.number);++height){const auto block=reopened.store().block_by_number(Int(std::to_string(height)));check(bool(block),"canonical payout history missing");for(const auto& w:block->withdrawals)if(quantity(w.at("validatorIndex"))==0){check(w.at("address")==recipient,"withdrawal paid wrong credentials");const auto amount=quantity(w.at("amount"));total+=amount*1000000000;if(height<=final_height)finalized_total+=amount*1000000000;if(amount>Int("30000000000"))++capital_payments;++payments;
       std::cout<<Json{{"event","capella-exit-payout"},{"owner",db},{"height",height},{"withdrawal",w}}.dump()<<std::endl;}}
      check(capital_payments==1&&finalized_total==paid_balance&&head.execution_state.get(recipient).balance==total,"withdrawal accounting or duplicate capital payout mismatch");
      // A withdrawn validator can still earn sync rewards until its assigned
      // committee expires. Compare exact payouts, not a permanently frozen balance.
      ChainStore signer_disk(directory/"network-key-0"),signer_witness(directory/"network-key-witness-0");
      if(db=="source-db"){BeaconSigningJournal journal(signer_disk,t.chain_id,genesis.header.hash(),root_at(state,8),Key(0).public_key,slots,std::nullopt,&signer_witness);const auto proposal=journal.latest_proposal_slot(),vote=journal.latest_attestation_target();check((!proposal||*proposal/slots<exit_epoch)&&(!vote||*vote<exit_epoch),"proposal or attestation after validator exit");}
      std::cout<<Json{{"event","capella-exit-owner-reopened"},{"owner",db},{"payments",payments},{"balance",quantity(total)},{"validatorBalanceGwei",residual},{"capitalPayments",capital_payments},{"exitEpoch",exit_epoch},{"finalizedExitRetired",true}}.dump()<<std::endl;
     }
     std::cout<<Json{{"scenario","capella-exit-network"},{"submittedOnlyToNonValidator",true},{"realLibp2p",true},{"recipient",recipient},{"paidBalance",quantity(paid_balance)},{"finalizedHash",finalized.at("hash")},{"finalizedNumber",finalized.at("number")},{"shortenedTestTenure",true},{"testWithdrawalDelayEpochs",1},{"independentValidatorHosts",false}}.dump()<<std::endl;return;
    }
    const auto follower_result=follower_node.stop();check(follower_result.code==0,"first follower failed shutdown");
    follower_config["p2p_identity"]=(directory/"follower-db.peerkey").string();follower_config["capella"]["recoveryWitness"]=(directory/"fresh-follower-witness").string();save(follower_file,follower_config);
    Process restarted({cli_binary,"run",(directory/"genesis.json").string(),(directory/"fresh-follower-db").string(),follower_file.string()},{{"NANO_TEST_NETWORK_TRACE","1"},{"NANO_TEST_DROP_ALTAIR_REPLY_ONCE","1"}});const auto restarted_port=restarted.port();const auto restarted_address=restarted.event("listen").at("address").get<std::string>();check(follower_address.substr(follower_address.rfind("/p2p/"))==restarted_address.substr(restarted_address.rfind("/p2p/")),"restarted peer identity changed");check(restarted.event("altair-ipc-ready").at("networkProtocolActive")==true,"restarted follower network inactive");
    try{until([&]{source_node.pump(0);restarted.pump(0);const auto a=rpc(source_port,"eth_getBlockByNumber",Json::array({"latest",false}));const auto b=rpc(restarted_port,"eth_getBlockByNumber",Json::array({"latest",false}));const bool matched=quantity(a.at("number"))>=2&&a.at("hash")==b.at("hash")&&a.at("stateRoot")==b.at("stateRoot")&&restarted.output().find("test-altair-reply-dropped")!=std::string::npos;if(matched)std::cout<<Json{{"event","capella-catchup-match"},{"number",a.at("number")},{"hash",a.at("hash")},{"stateRoot",a.at("stateRoot")}}.dump()<<std::endl;return matched;},"same peer fresh state recovered despite lost ACK",50);}catch(...){const auto a=source_node.stop(),b=restarted.stop();std::cerr<<a.out<<a.err<<b.out<<b.err;throw;}
    const auto source_result=source_node.stop(),restarted_result=restarted.stop();check(source_result.code==0&&restarted_result.code==0,"network nodes failed shutdown");std::cout<<source_result.out<<follower_result.out<<restarted_result.out;
    std::cout<<Json{{"scenario","capella-two-peer-catchup"},{"realLibp2p",true},{"lateFollower",true},{"samePeerFreshState",true},{"lostAcknowledgementRecovered",true},{"equalExecutionHeadAndState",true},{"distributedFinalityQualified",false}}.dump()<<std::endl;return;
   }
   const auto node_file=directory/"node.json";save(node_file,node);
   const std::vector<std::string> args{cli_binary,"run",(directory/"genesis.json").string(),(directory/"managed-db").string(),node_file.string()};
   const Env env{{"NANO_TEST_MANAGED_CHILD","1"},{key_env,hex(bytes(Int(1),32))},{"NANO_PRIVATE_KEY","synthetic-user-secret"},{"NANO_NETWORK_TOKEN","must-be-replaced"}};
   auto bad=node;bad["rpc_host"]="0.0.0.0";save(node_file,bad);auto denied=run(args,env);check(denied.code!=0&&denied.err.find("local RPC")!=std::string::npos,"managed Capella allowed public RPC");
   bad=node;bad["bellatrix"]=Json::object();save(node_file,bad);denied=run(args,env);check(denied.code!=0&&denied.err.find("exclusive")!=std::string::npos,"managed Capella accepted conflicting profiles");
   const unsigned starts=daemon_binary.empty()?3:2;for(unsigned restart=0;restart<starts;++restart){
    save(node_file,node);auto child_env=env;if(restart==2)child_env["NANO_TEST_MANAGED_CHILD_EXIT"]="1";
    Process child(args,child_env);child.port();
    if(daemon_binary.empty()){const auto ready=child.event("capella-managed-child-ready");check(ready.at("executionBound")==true&&ready.at("secretsExcluded")==true,"managed child IPC/secret isolation failed");}
    else{const auto ready=child.event("altair-ipc-ready");check(ready.at("profile")=="capella"&&ready.at("networkProtocolActive")==false,"daemon did not recover Capella IPC privately");}
    const auto result=restart==2?child.wait():child.stop();check(result.code==(restart==2?1:0)&&result.out.find("capella-runtime-stopped")!=std::string::npos,"managed runtime did not stop cleanly");
    check(result.out.find("synthetic-user-secret")==std::string::npos&&result.err.find("synthetic-user-secret")==std::string::npos,"managed secret logged");if(!daemon_binary.empty())std::cout<<result.out;node["capella"]["initial"]=nullptr;
   }
   if(!daemon_binary.empty()){std::cout<<Json{{"scenario","capella-daemon-ipc"},{"starts",starts},{"libp2pProcess",true},{"consensusNetworkActive",false},{"witnessRestart",true}}.dump()<<std::endl;return;}
   std::cout<<Json{{"scenario","capella-managed-node"},{"starts",3},{"witnessRestart",true},{"negativeConfigs",2},{"childFailureStopsNode",true},{"validatorKeyExcludedFromChild",true},{"libp2p",false}}.dump()<<std::endl;return;
  }
  const auto token=hex(Bytes(32,0x42));const std::vector<std::string> args{cli_binary,"serve",(directory/"genesis.json").string(),(directory/"cli-db").string(),"0","127.0.0.1",(directory/"serve.json").string()};
  auto denied=run(args,{{"NANO_NETWORK_TOKEN",std::nullopt}});check(denied.code!=0&&denied.err.find("NANO_NETWORK_TOKEN")!=std::string::npos,"CLI accepted missing private token");auto public_args=args;public_args[5]="0.0.0.0";denied=run(public_args,{{"NANO_NETWORK_TOKEN",token}});check(denied.code!=0&&denied.err.find("loopback")!=std::string::npos,"CLI accepted public Capella bind");
  for(unsigned restart=0;restart<2;++restart){Process child(args,{{"NANO_NETWORK_TOKEN",token}});PersistentRpc client(child.port());check(client.call("nano_capellaStatus",Json::array({token})).at("clockHealthy")==true,"CLI runtime unavailable");const auto handle=client.call("nano_capellaCatalogOpen",Json::array({token,"block"})).at("handle");check(client.call("nano_capellaCatalogPage",Json::array({token,handle,nullptr,"0x1"})).at("entries").empty(),"CLI anchor fabricated archived block");const auto result=child.stop();check(result.code==0&&result.out.find("capella-runtime-stopped")!=std::string::npos,"CLI failed clean shutdown");config["initial"]=nullptr;{std::ofstream f(directory/"serve.json");f<<Json{{"capella",config}};}}
  std::cout<<Json{{"scenario","capella-cli-serve"},{"starts",2},{"witnessRestart",true},{"negativeConfigs",2},{"p2p",false}}.dump()<<std::endl;return;
 }
 struct Environments{std::vector<std::string> names;~Environments(){for(const auto& name:names)unsetenv(name.c_str());}} env;
 Json keys=Json::array();for(uint64_t i=0;i<64;++i){const auto name="NANO_TEST_CAPELLA_RUNTIME_"+std::to_string(getpid())+"_"+std::to_string(i);check(std::getenv(name.c_str())==nullptr,"synthetic key env collision");check(setenv(name.c_str(),hex(bytes(Int(std::to_string(i+1)),32)).c_str(),1)==0,"synthetic env failure");env.names.push_back(name);keys.push_back({{"index",quantity(Int(std::to_string(i)))},{"feeRecipient",genesis.header.coinbase},{"keyEnv",name},{"journal",(directory/("key-"+std::to_string(i))).string()},{"restart",nullptr},{"recoveryWitness",(directory/("key-witness-"+std::to_string(i))).string()}});}
 const uint64_t block_budget=t.consensus.maximum_block_bytes,vote_budget=228+(uint64_t(t.consensus.body.validators_per_committee)+8)/8,slash_budget=464+16*uint64_t(t.consensus.body.validators_per_committee),largest=std::max({block_budget,vote_budget,slash_budget});
 config["admission"]={{"frameBytes",quantity(Int(std::to_string(1024+2*largest)))},{"blockBytes",quantity(Int(std::to_string(block_budget)))},{"attestationBytes",quantity(Int(std::to_string(vote_budget)))},{"slashingBytes",quantity(Int(std::to_string(slash_budget)))},{"jobs","0x2"},{"payloadBytes",quantity(Int(std::to_string(2*largest)))}};
 config["catalogs"]={{"handles","0x2"},{"rootBytes","0x10000"},{"ttlMs","0xea60"}};
 config["validator"]={{"pollIntervalMs",quantity(Int(std::to_string(rules.slot_duration_ms)))},{"maximumTransactionTrials","0x10"},{"maximumVoteTrials","0x40"},{"keys",keys}};
 std::atomic<uint64_t> now{initial.timed.clock.time_ms};auto wall=[&]{return now.load();};
 auto refused=[&](const Json& candidate){bool rejected=false;try{auto invalid=start_capella_runtime(*owner,candidate,{},wall);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"invalid runtime configuration accepted");};
 if(automatic_recovery){config["automaticRecovery"]=true;auto invalid=config;invalid["automaticRecovery"]="yes";refused(invalid);invalid=config;invalid.erase("recoveryWitness");refused(invalid);invalid=config;invalid["initial"]=nullptr;refused(invalid);}
 auto bad=config;bad["unexpected"]=true;refused(bad);bad=config;bad["profile"]["sha256"]=hex(Bytes(32,0));refused(bad);bad=config;bad["clock"]["intervalMs"]="0x0";refused(bad);
 bad=config;bad["validator"]["keys"][0]["keyEnv"]=env.names[1];refused(bad);bool closed=false;try{owner->tip();}catch(const std::runtime_error&){closed=true;}check(closed,"failed key load left execution open");
 if(!automatic_recovery)config["initial"]=nullptr;bad=config;bad["admission"]["frameBytes"]="0x400";refused(bad);check(!fs::exists(directory/"key-0"),"invalid admission opened signing journal");bad=config;bad["catalogs"]["ttlMs"]="0x0";refused(bad);check(!fs::exists(directory/"key-0"),"invalid catalogs opened signing journal");if(automatic_registration){
  for(auto& key:config["validator"]["keys"])key["index"]=nullptr;
  const auto name="NANO_TEST_CAPELLA_PENDING_"+std::to_string(getpid());check(std::getenv(name.c_str())==nullptr,"pending env collision");setenv(name.c_str(),hex(bytes(Int(65),32)).c_str(),1);env.names.push_back(name);
  config["validator"]["keys"].push_back({{"index",nullptr},{"feeRecipient",genesis.header.coinbase},{"keyEnv",name},{"journal",(directory/"pending-key").string()},{"restart",nullptr},{"recoveryWitness",(directory/"pending-witness").string()}});
 }
 auto runtime=start_capella_runtime(*owner,config,{},wall);check(bool(runtime.validator)&&runtime.clock->healthy()&&runtime.admission&&runtime.catalogs,"configured runtime unavailable");const auto runtime_identity=runtime.reactor->network_identity();if(automatic_registration)runtime.validator->set_duty_trace_limit(4);
 const auto secret=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;const auto sender=SignedTransaction::sign(tx,secret).sender();uint64_t proposals=0;
 for(uint64_t slot=2;slot<=2*slots+1;++slot){
  tx.nonce=owner->state().get(sender).nonce;check(owner->submit(SignedTransaction::sign(tx,secret)),"runtime transaction refused");now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;runtime.validator->poll();check(owner->state().get(sender).nonce==Int(std::to_string(slot)),"runtime block failed to execute");check(runtime.beacon_read("/eth/v2/beacon/blocks/head",{}).body.at("data").at("message").at("body").at("execution_payload").at("block_hash")==hex(owner->tip().header.hash()),"runtime Beacon handler diverged");now=now.load()+rules.attestation_due_ms;runtime.validator->poll();if(automatic_registration){const auto status=runtime.validator->status(true);check(status.at("healthy")==true&&status.at("awaitingRegistration")==1,"configured pending key did not wait");for(const auto& entry:status.at("dutyTrace"))for(const auto& message:entry.at("latestMessages"))check(message.at("local")==true,"automatic key trace used unresolved index");const auto commitments=runtime.validator->restart_commitments();check(commitments.back().at("index").is_null()&&commitments.back().at("publicKey")==hex(Key(64).public_key),"pending key restart identity lost");}
  if(slot==slots+1){const auto receipt=stop_capella_runtime(runtime);check(receipt.at("clockHealthy")==true&&receipt.at("validatorKeys").size()==(automatic_registration?65:64),"runtime stop receipt incomplete");proposals+=receipt.at("validator").at("proposalsAdmitted").get<uint64_t>();runtime={};owner.reset();owner=std::make_unique<Blockchain>(directory/"owner",g);runtime=start_capella_runtime(*owner,config,{},wall);}
 }
 check(runtime.reactor->network_identity()==runtime_identity,"runtime identity changed across restart");const auto saved=runtime.reactor->current();const auto root=consensus_beacon_header_root(capella_fork_head(*saved.state,rules).header);Json envelope=runtime_identity;envelope["kind"]="block";envelope["ssz"]=hex(*runtime.reactor->signed_block(root));auto done=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto result=done->get_future();check(runtime.admission->submit(envelope.dump(),std::make_shared<std::atomic<bool>>(false),[done](auto d){done->set_value(d);}),"runtime admission refused duplicate");check(result.wait_for(std::chrono::seconds(10))==std::future_status::ready&&result.get()==CapellaConsensusService::Decision::accepted&&runtime.reactor->current().token==saved.token,"runtime admission changed duplicate checkpoint");
 {
  Rpc rpc(*owner,"capella-test-token",AdmissionBudget::Clock::now,{}, {}, {},std::make_shared<CapellaRuntime>(runtime));check(rpc.requires_private_bind()&&rpc.local_only(),"Capella RPC exposed public bind");
  struct RunningHttp {HttpServer& server;std::thread thread;explicit RunningHttp(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningHttp(){server.stop();thread.join();}};
  std::unique_ptr<HttpServer> http_server;std::unique_ptr<RunningHttp> http_running;std::unique_ptr<PersistentRpc> http_client;
  if(capella_http){bool public_refused=false;try{HttpServer forbidden(rpc,"0.0.0.0",0);}catch(const std::invalid_argument&){public_refused=true;}check(public_refused,"Capella HTTP accepted public bind");http_server=std::make_unique<HttpServer>(rpc,"127.0.0.1",0);http_running=std::make_unique<RunningHttp>(*http_server);http_client=std::make_unique<PersistentRpc>(http_server->port());}
  auto call=[&](const char* method,Json params){if(http_client){try{return Json{{"result",http_client->call(method,std::move(params))}};}catch(const PersistentRpcError& e){return Json{{"error",{{"code",e.code}}}};}}return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});};

  for(const auto* method:{"nano_capellaStatus","nano_capellaJobs","nano_capellaCatalogGeneration"})check(call(method,Json::array({"wrong-token"})).at("error").at("code")==-32001,"unauthorized Capella read accepted");
  check(call("nano_capellaCatalogOpen",Json::array({"wrong-token","block"})).at("error").at("code")==-32001,"unauthorized catalog opened");
  check(call("nano_capellaSubmit",Json::array({"wrong-token","invalid","0x1",envelope.dump()})).at("error").at("code")==-32001,"unauthorized submit accepted");
  const auto status=call("nano_capellaStatus",Json::array({"capella-test-token"})).at("result");check(status.at("consensusIdentity")==runtime_identity.at("consensusIdentity")&&status.at("networkProtocolActive")==false,"Capella RPC status differs");
  const auto rpc_session=status.at("admissionSession");check(!call("nano_capellaSubmit",Json::array({"capella-test-token",rpc_session,"0x1",envelope.dump()})).contains("error"),"Capella RPC submit failed");
  const auto rpc_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);Json rpc_job;do{rpc_job=call("nano_capellaJob",Json::array({"capella-test-token",rpc_session,"0x1"})).at("result");if(rpc_job.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<rpc_deadline);check(rpc_job.at("state")=="completed"&&rpc_job.at("decision")=="accepted"&&runtime.reactor->current().token==saved.token,"Capella RPC result changed duplicate checkpoint");
  const auto exported=call("nano_capellaExport",Json::array({"capella-test-token","block",hex(Bytes(root.begin(),root.end()))})).at("result");check(exported.at("envelope")==envelope,"Capella RPC export differs");
  const auto handle=call("nano_capellaCatalogOpen",Json::array({"capella-test-token","block"})).at("result").at("handle");check(call("nano_capellaCatalogPage",Json::array({"capella-test-token",handle,nullptr,"0x2"})).at("result").at("entries").size()==2,"Capella RPC page failed");check(call("nano_capellaCatalogClose",Json::array({"capella-test-token",handle})).at("result")==true,"Capella RPC close failed");check(call("nano_capellaCatalogClose",Json::array({"capella-test-token",handle})).at("result")==true,"Capella RPC repeated close failed");
  check(call("nano_capellaCatalogPage",Json::array({"capella-test-token",handle,nullptr,"0x2"})).contains("error"),"closed RPC catalog readable");
  check(call("nano_capellaExport",Json::array({"capella-test-token","block","0x01"})).at("error").at("code")==-32602,"invalid RPC root accepted");
  if(capella_http){namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;asio::io_context context;beast::tcp_stream stream(context);stream.connect({asio::ip::make_address("127.0.0.1"),http_server->port()});http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"localhost");request.set(http::field::origin,"https://example.invalid");request.body()=Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_capellaStatus"},{"params",Json::array({"capella-test-token"})}}.dump();request.prepare_payload();http::write(stream,request);beast::flat_buffer buffer;http::response<http::string_body> response;http::read(stream,buffer,response);check(response.result()==http::status::forbidden,"browser-origin Capella request accepted");}

 }
 const auto runtime_catalog=runtime.catalogs->open("authenticated-test-peer","block");check(!runtime.catalogs->page("authenticated-test-peer",runtime_catalog,{},2,512).at("entries").empty(),"configured catalog empty");
 const auto session=runtime.admission->receipts().at("session").get<std::string>();
 runtime.admission->submit_receipt(session,1,envelope.dump());
 Json receipt;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 do{receipt=runtime.admission->receipt(session,1);if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
 check(receipt.at("state")=="completed"&&receipt.at("decision")=="accepted","Capella receipt lost committed result");
 check(runtime.admission->submit_receipt(session,1,envelope.dump())==receipt,"duplicate receipt changed result");
 auto conflict=envelope;auto conflict_bytes=*runtime.reactor->signed_block(root);conflict_bytes[4]^=1;conflict["ssz"]=hex(conflict_bytes);bool conflict_rejected=false;try{runtime.admission->submit_receipt(session,1,conflict.dump());}catch(const std::invalid_argument&){conflict_rejected=true;}check(conflict_rejected,"receipt sequence reused for different payload");
 runtime.admission->receipt(session,1,false,true);check(runtime.admission->submit_receipt(session,1,envelope.dump()).at("state")=="retired","retired receipt replay admitted");
 check(runtime.admission->receipt(session,2,true).at("state")=="retired"&&runtime.admission->submit_receipt(session,2,envelope.dump()).at("state")=="retired","cancel-before-submit fence failed");
 const auto rotated=runtime.admission->rotate_session(session);bool stale=false;try{runtime.admission->submit_receipt(session,3,envelope.dump());}catch(const std::invalid_argument&){stale=true;}check(stale&&rotated.at("session")!=session&&runtime.reactor->current().token==saved.token,"session fence changed consensus");
 {
  Rpc ipc(*owner,"capella-ipc-test",AdmissionBudget::Clock::now,{}, {}, {},std::make_shared<CapellaRuntime>(runtime));
  const auto old_session=runtime.admission->receipts().at("session").get<std::string>();runtime.admission->submit_receipt(old_session,3,envelope.dump());
  boost::asio::io_context io;Json recovered;auto call=[&](std::string method,Json params,auto reply){check(method.starts_with("nano_altair"),"unexpected recovery method");method.replace(0,11,"nano_capella");auto response=ipc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});reply(response.contains("result")?response.at("result"):Json(nullptr));};
  auto recovery=std::make_shared<AltairCoreRecovery>(io,call,"capella-ipc-test",runtime_identity.at("chainId").get<std::string>(),runtime_identity.at("genesisHash").get<std::string>(),2,std::chrono::milliseconds(1),std::chrono::seconds(10),[&](Json result){recovered=std::move(result);});recovery->start();io.run();
  check(recovered.is_object()&&recovered.at("session")!=old_session&&recovered.at("highWater")==3&&recovered.at("networkAdmissionEnabled")==false&&runtime.admission->receipts().at("jobs").empty(),"Capella IPC recovery failed to fence/drain");
  InboundBudget catalog_budget;auto ingress=std::make_shared<AltairMessageIngress>(AltairWireProfile{runtime_identity.at("chainId").get<std::string>(),runtime_identity.at("genesisHash").get<std::string>(),runtime_identity.at("forkDigest").get<std::string>(),size_t(1024+2*largest),size_t(block_budget),size_t(vote_budget),size_t(slash_budget),runtime_identity.at("consensusIdentity").get<std::string>()},catalog_budget);
  size_t relayed=0;for(const auto* kind:{"block","attestation","attester_slashing"}){
   io.restart();Json result;
   auto catalog_call=[&](std::string method,Json params,Reply reply){const auto response=ipc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});reply(response.contains("result")?response.at("result"):Json(nullptr));};
   auto deliver=[&](std::string,Json frame,Reply reply){const bool submitted=runtime.admission->submit(frame.dump(),std::make_shared<std::atomic<bool>>(false),[reply](auto decision){reply(Json{{"accepted",decision==CapellaConsensusService::Decision::accepted}});});if(!submitted)reply(nullptr);};
   auto relay=std::make_shared<CapellaCatalogRelay>(io,catalog_call,deliver,"capella-ipc-test",kind,ingress,2,limits.blocks+limits.messages+limits.equivocations,[&](Json value){result=std::move(value);});relay->start(std::chrono::seconds(10));io.run();
   check(result.at("success")==true&&result.at("closed")==true,"real Capella catalog relay failed");relayed+=result.at("delivered").get<size_t>();
  }
  check(relayed>=proposals&&runtime.reactor->current().token==saved.token,"catalog relay mutated duplicate checkpoint or omitted blocks");
  io.restart();Json lost_result;std::string lost_handle;unsigned releases=0;bool peer_current=true;
  auto lose_open=[&](std::string method,Json params,Reply reply){auto response=ipc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});if(method=="nano_capellaCatalogAcquire"){check(response.contains("result"),"fault injection open failed before reply loss");lost_handle=response.at("result").at("handle");peer_current=false;reply(nullptr);return;}if(method=="nano_capellaCatalogRelease")++releases;reply(response.contains("result")?response.at("result"):Json(nullptr));};
  auto gated_core=capella_peer_core(lose_open,[&]{return peer_current;});
  auto lost_relay=std::make_shared<CapellaCatalogRelay>(io,gated_core,[](auto,auto,auto){throw std::runtime_error("lost open delivered payload");},"capella-ipc-test","block",ingress,1,2,[&](Json value){lost_result=std::move(value);});lost_relay->start(std::chrono::seconds(10));io.run();
  check(lost_result.at("success")==false&&lost_result.at("closed")==true&&releases==1&&!lost_handle.empty(),"disconnected peer lost open not released");
  bool released=false;try{runtime.catalogs->page("capella-private-daemon",lost_handle,{},1,512);}catch(const std::invalid_argument&){released=true;}check(released,"lost open retained snapshot");
  const auto free_quota=runtime.catalogs->open("quota-check","block");runtime.catalogs->close("quota-check",free_quota);
  unsigned forwarded=0,denied=0;auto disconnected=capella_peer_core([&](const std::string&,Json,Reply reply){++forwarded;reply(true);},[]{return false;});
  for(const auto* method:{"nano_capellaCatalogGeneration","nano_capellaCatalogAcquire","nano_capellaCatalogPage","nano_capellaExport","nano_capellaSubmit"})disconnected(method,Json::array(),[&](Json value){check(value.is_null(),"disconnected peer started new core work");++denied;});
  check(forwarded==0&&denied==5,"disconnected gate failed to deny new work");



 }
 if(queued_bls_changes){
  SignedCapellaBlsChange change;Key key(1001);change.validator_index=1;copy_to(change.from_bls_pubkey,key.public_key);change.to_execution_address[19]=43;const auto genesis_root=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);copy_to(change.signature,key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,genesis_root))));
  Rpc rpc(*owner,"capella-test-token",AdmissionBudget::Clock::now,{}, {}, {},std::make_shared<CapellaRuntime>(runtime));HttpServer server(rpc,"127.0.0.1",0);std::thread http_thread([&]{server.run();});struct StopHttp{HttpServer& server;std::thread& thread;~StopHttp(){server.stop();thread.join();}} stop{server,http_thread};PersistentRpc client(server.port());
  const auto status=client.call("nano_capellaStatus",Json::array({"capella-test-token"}));const auto active=runtime.admission->receipts();const auto ticket=quantity(quantity(active.at("highWater"))+1);auto operation=runtime_identity;operation["kind"]="bls_to_execution_change";operation["sszBase64"]=altair_base64(encode_capella_bls_change(change));const auto before=runtime.reactor->current().token;
  bool denied=false;try{client.call("nano_capellaSubmit",Json::array({"wrong-token",active.at("session"),ticket,operation}));}catch(const PersistentRpcError& e){denied=e.code==-32001;}check(denied&&runtime.reactor->current().token==before,"unauthorized BLS RPC admitted");
  client.call("nano_capellaSubmit",Json::array({"capella-test-token",active.at("session"),ticket,operation}));Json receipt;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);do{receipt=client.call("nano_capellaJob",Json::array({"capella-test-token",active.at("session"),ticket}));if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
  check(receipt.value("decision",std::string())=="accepted"&&runtime.reactor->current().token.sequence==before.sequence+1&&runtime.reactor->pending_bls_changes().size()==1,"BLS HTTP request not durably queued");
 }
 const auto final=stop_capella_runtime(runtime);bool catalog_closed=false;try{runtime.catalogs->page("authenticated-test-peer",runtime_catalog,{},2,512);}catch(const std::invalid_argument&){catalog_closed=true;}check(catalog_closed,"stopped runtime retained catalog");catalog_closed=false;try{runtime.catalogs->open("authenticated-test-peer","block");}catch(const std::runtime_error&){catalog_closed=true;}check(catalog_closed,"stopped runtime opened catalog");check(!runtime.admission->submit(envelope.dump(),std::make_shared<std::atomic<bool>>(false),[](auto){}),"stopped runtime accepted admission");proposals+=final.at("validator").at("proposalsAdmitted").get<uint64_t>();check(proposals==2*slots&&final.at("clockHealthy")==true,"runtime proposal total differs");runtime={};closed=false;try{owner->tip();}catch(const std::runtime_error&){closed=true;}check(closed,"released runtime left execution readable");
 if(automatic_recovery){auto missing=config;missing["recoveryWitness"]=(directory/"missing-recovery-witness").string();bool rejected=false;try{auto invalid=start_capella_runtime(*owner,missing,{},wall);}catch(const std::exception&){rejected=true;}check(rejected,"automatic recovery bootstrapped over primary with missing witness");auto resumed=start_capella_runtime(*owner,config,{},wall);check(resumed.reactor->current().token==saved.token,"automatic recovery changed retained checkpoint");stop_capella_runtime(resumed);}
 std::cout<<Json{{"automaticRecovery",automatic_recovery},{"scenario","capella-configured-runtime"},{"blocks",proposals},{"keysLoaded",automatic_registration?65:64},{"automaticRegistration",automatic_registration},{"witnessRestart",true},{"signerWitnessRestart",true},{"admissionReceiptChecks",6},{"httpTransport",capella_http},{"runtimeCatalogChecks",3},{"invalidConfigurationsRejected",6},{"workersStopped",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void verify_beacon_reads(CapellaDurableReactor& reactor,const CapellaForkBlockRules& rules){
 const auto snapshot=reactor.current();const auto& head=capella_fork_head(*snapshot.state,rules);const auto id=consensus_beacon_header_root(head.header);auto text=[](const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));};
 const auto header=reactor.beacon_read("/eth/v1/beacon/headers/head");check(header.status==200&&header.body.at("data").at("root")==text(id)&&header.body.at("data").at("canonical")==true,"Capella Beacon head header differs");
 check(reactor.beacon_read("/eth/v1/beacon/headers/"+std::to_string(head.header.slot)).body==header.body&&reactor.beacon_read("/eth/v1/beacon/headers/"+text(id)).body==header.body,"Capella Beacon header selectors differ");
 const auto full=reactor.beacon_read("/eth/v2/beacon/blocks/head");check(full.status==200&&full.version=="capella"&&full.body.at("version")=="capella"&&full.ssz==*reactor.signed_block(id),"Capella Beacon signed bytes differ");const auto& body=full.body.at("data").at("message").at("body");const auto& payload=body.at("execution_payload");
 check(payload.at("block_hash")==hex(head.execution_block.header.hash())&&payload.at("state_root")==hex(head.execution_state.root())&&payload.at("transactions").size()==head.execution_block.transactions.size(),"Capella Beacon execution projection differs");
 const auto& withdrawals=payload.at("withdrawals");check(withdrawals.size()==head.execution_block.withdrawals.size(),"Beacon withdrawal count differs");for(size_t i=0;i<withdrawals.size();++i){const auto& expected=head.execution_block.withdrawals[i];check(withdrawals[i].at("index")==quantity(expected.at("index")).get_str()&&withdrawals[i].at("validator_index")==quantity(expected.at("validatorIndex")).get_str()&&withdrawals[i].at("amount")==quantity(expected.at("amount")).get_str()&&withdrawals[i].at("address")==expected.at("address"),"Beacon withdrawal fields differ");}
 check(reactor.beacon_read("/eth/v1/beacon/states/head/root").body.at("data").at("root")==text(head.header.state_root),"Beacon state root differs");
 const auto checkpoints=capella_fork_checkpoints(head.state,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);const auto finality=reactor.beacon_read("/eth/v1/beacon/states/head/finality_checkpoints");check(finality.body.at("data").at("finalized").at("epoch")==std::to_string(checkpoints.realized.finalized.epoch)&&finality.body.at("data").at("finalized").at("root")==text(checkpoints.realized.finalized.root),"Beacon finalized state checkpoint differs");
 const auto spec=reactor.beacon_read("/eth/v1/config/spec");check(spec.body.at("data").at("MAX_WITHDRAWALS_PER_PAYLOAD")==std::to_string(rules.transition.withdrawals.maximum_per_payload),"Beacon Capella spec differs");
 check(reactor.beacon_read("/eth/v1/beacon/headers/0x12").status==400&&reactor.beacon_read("/eth/v1/beacon/headers/genesis").status==404&&reactor.beacon_read("/eth/v1/node/syncing").status==404,"Beacon unsupported selector behavior");
 bool cancelled=false;try{reactor.beacon_read("/eth/v2/beacon/blocks/head",[]{throw ExecutionCancelled("Beacon read cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&reactor.current().token==snapshot.token,"Beacon read cancellation changed checkpoint");++beacon_read_checks;
}
static void scenario(bool migrate,uint64_t skipped=0,bool finality=false,unsigned participation=100){
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",refs=fixtures/"consensus-bellatrix-containers-official",source=fixtures/"consensus-capella-slots-official";
 const auto manifest=load(source/"manifest.json");check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f","initial state revision");
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="slots_1")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(source/"slots_1"/path))).substr(2)==info.at("sha256").get<std::string>(),"initial state digest");
 auto common=altair_transition_rules(fixtures);if(local_exits||exit_reindex)common.exit.shard_committee_period=1;if(exit_network||exit_lifecycle_sync){common.exit.shard_committee_period=1;common.exit.minimum_withdrawability_delay=1;common.epoch.registry.minimum_withdrawability_delay=1;common.epoch.slashing.minimum_withdrawability_delay=1;}const auto preset=read(refs/"minimal-bellatrix.yaml");auto p=[&](const char* k){return parameter(preset,k);};
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
 CapellaSlotState beacon{binary(source/"slots_1/pre.ssz"),std::nullopt};put(beacon.ssz,field+12,48);if(!cli_binary.empty())put(beacon.ssz,0,uint64_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())-(exit_network?slots+1:2)*seconds);const auto genesis_time=little(beacon.ssz,0,8);const auto genesis_root=root_at(beacon.ssz,8);
 Key withdrawal_key(1000);auto credentials=sha256(withdrawal_key.public_key);credentials[0]=0;const auto validators_at=little(beacon.ssz,hist+88,4),balances_at=little(beacon.ssz,hist+92,4);
 std::copy(credentials.begin(),credentials.end(),beacon.ssz.begin()+validators_at+48);put(beacon.ssz,balances_at,r.withdrawals.maximum_effective_balance+1000000000);
 SignedCapellaBlsChange change;change.validator_index=0;copy_to(change.from_bls_pubkey,withdrawal_key.public_key);change.to_execution_address[19]=42;
 copy_to(change.signature,withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},r.genesis_version,genesis_root))));
 SignedCapellaBlsChange local_change;Key local_withdrawal_key(1001);
 if(local_bls_changes||pruning_bls){put(beacon.ssz,balances_at+8,r.withdrawals.maximum_effective_balance+1000000000);auto credential=sha256(local_withdrawal_key.public_key);credential[0]=0;std::copy(credential.begin(),credential.end(),beacon.ssz.begin()+validators_at+121+48);local_change.validator_index=1;copy_to(local_change.from_bls_pubkey,local_withdrawal_key.public_key);local_change.to_execution_address[19]=43;copy_to(local_change.signature,local_withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(local_change),consensus_domain({10,0,0,0},r.genesis_version,genesis_root))));}
 if(queued_bls_reorg){auto credential=sha256(Key(1002).public_key);credential[0]=0;std::copy(credential.begin(),credential.end(),beacon.ssz.begin()+validators_at+242+48);}
 const auto key=bytes(Int(777),32);const std::string target="0x0000000000000000000000000000000000001234";Transaction tx;tx.type=2;tx.to=target;tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;
 State state;if(pruning_native){std::ifstream file(fixtures/"ethereum-blocks/for_prague--prague--eip6110_deposits--deposits--deposit.json");Json data;file>>data;state.get(ethereum_deposit_contract)=State::from_json(data.begin().value().at("pre")).get(ethereum_deposit_contract);}
 state.get(SignedTransaction::sign(tx,key).sender()).balance=Int(deposit_reorg?"70000000000000000000":(deposit_activation||deposit_invalid_signature)?"34000000000000000000":pruning_native?"3000000000000000000":"1000000000000000000");state.get(target).code=unhex("602a5f5500");
 Block parent;parent.header.difficulty=0;parent.header.timestamp=Int(std::to_string(genesis_time));parent.header.state_root=state.root();if(!migrate)parent.header.withdrawals_root=withdrawals_root(Json::array());
 if(!cli_binary.empty()){cli_genesis={{"config",{{"chainId",1337},{"consensus","beacon"},{"evmFork","Shanghai"}}},{"timestamp",quantity(Int(std::to_string(genesis_time)))},{"alloc",state.to_json()}};const auto parsed=Genesis::from_json(cli_genesis);parent=parsed.block;state=parsed.state;}
 if(pruning_native){const ExecutionDepositHistory empty(parent.header.hash());const auto root=empty.root();std::copy(root.begin(),root.end(),beacon.ssz.begin()+hist+4);put(beacon.ssz,hist+36,0);const auto hash=parent.header.hash();std::copy(hash.begin(),hash.end(),beacon.ssz.begin()+hist+44);put(beacon.ssz,hist+80,0);}
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
 const uint64_t first_slot=migrate?slots+skipped:exit_network?slots:1;
 std::map<Int,Bytes> hashes{{0,parent.header.hash()}};std::function<Bytes(const Int&)> history=[&](const Int& n){return hashes.at(n);};
 const auto disk_genesis=parent;const auto disk_genesis_state=state;
 CapellaForkBlockRules store_rules{r,seconds*1000,seconds*1000/3,40};if(pruning_native){store_rules.native_deposit_votes=true;store_rules.deposit_contract_code_hash=keccak(state.get(ethereum_deposit_contract).code);}
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
 std::optional<CapellaForkStore> catchup_anchor;
 std::shared_ptr<const CapellaInventorySnapshot> stable_inventory;Json stable_first;
 InboundBudget ingress_budget;std::unique_ptr<CapellaConsensusService> service;size_t wire_messages=0;
 auto deliver=[&](const char* kind,const Bytes& bytes){Json envelope=reactor->network_identity();envelope["kind"]=kind;envelope["ssz"]=hex(bytes);auto promise=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto future=promise->get_future();check(service->submit(envelope.dump(),std::make_shared<std::atomic<bool>>(false),[promise](auto decision){promise->set_value(decision);}),"Capella wire enqueue refused");check(future.wait_for(std::chrono::seconds(10))==std::future_status::ready,"Capella worker deadline");++wire_messages;return future.get()==CapellaConsensusService::Decision::accepted;};
 unsigned accepted=0,rejected=0,cancelled=0,contexts=0,votes=0,vote_reorgs=0;uint64_t paid=0;
 const uint64_t local_change_slot=queued_bls_reorg?first_slot+slots:first_slot+1;
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
  if(slot==first_slot)check(withdrawals.empty(),"credentials paid before change");if(slot==first_slot+(migrate?4:1))check(!withdrawals.empty(),"credentials not paid at expected sweep");if(local_bls_changes&&slot==local_change_slot)for(const auto& withdrawal:withdrawals)check(withdrawal.validator_index!=1,"local credential change paid in same block");paid+=withdrawals.size();
  State expected=state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(expected,execution,false);
  auto ep=payload(execution);ep.withdrawals=withdrawals;const auto encoded=encode_capella_execution_payload(ep,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);
  const size_t sync_bytes=(l.sync_committee_size+7)/8+96,fixed=228+sync_bytes;
  Bytes body(fixed,0);std::copy(reveal.begin(),reveal.end(),body.begin());std::copy_n(advanced.ssz.begin()+hist+4,72,body.begin()+96);body[220+(l.sync_committee_size+7)/8]=0xc0;
  for(size_t at:std::array<size_t,6>{200,204,208,212,216,220+sync_bytes})put(body,at,fixed,4);put(body,224+sync_bytes,fixed+encoded.size(),4);body.insert(body.end(),encoded.begin(),encoded.end());if(slot==first_slot){const auto wire=encode_capella_bls_change(change);body.insert(body.end(),wire.begin(),wire.end());}if(local_bls_changes&&slot==local_change_slot){const auto operation=encode_capella_bls_change(local_change);body.insert(body.end(),operation.begin(),operation.end());}
  SignedConsensusVoluntaryExit selected_exit{epoch,2,{}};
  if(local_exits&&slot==first_slot+slots){
   selected_exit.signature=Key(2).sign(consensus_signing_root(consensus_voluntary_exit_root(epoch,2),consensus_domain({4,0,0,0},version,genesis_root)));
   const auto operation=encode_consensus_voluntary_exit_ssz(selected_exit);
#ifdef NANO_SYNC_FAULT
   if(exit_sync){const auto current=reactor->current();qualify_bls_sync(executable,store_rules,capella_store_snapshot(*current.state,store_rules,crash_limits),disk_genesis,disk_genesis_state,current.state->timed.clock.time_ms,operation,true);return;}
#endif
   body.insert(body.begin()+fixed,operation.begin(),operation.end());
   put(body,220+sync_bytes,fixed+operation.size(),4);put(body,224+sync_bytes,fixed+operation.size()+encoded.size(),4);
  }
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
   const auto now=(genesis_time+slot*seconds)*1000;reactor->tick(now);
   if(queued_exits&&slot==first_slot+slots){
    const auto before=reactor->current();const auto operation=encode_consensus_voluntary_exit_ssz(selected_exit);auto invalid=operation;invalid.back()^=1;
    check(!reactor->voluntary_exit_ssz(invalid)&&reactor->current().token==before.token,"invalid exit queue admission mutated state");
    bool cancelled=false;try{reactor->voluntary_exit_ssz(operation,[]{throw ExecutionCancelled("exit admission cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&reactor->current().token==before.token,"cancelled exit admission mutated state");
    {Temp bounded_dir;ChainStore disk(bounded_dir/"queue");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-exit-capacity");disk.commit(disk_genesis,disk_genesis_state,{});auto limits=snapshot_limits;limits.messages=1;CapellaDurableReactor bounded(disk,r.chain_id,disk_genesis.header.hash(),store_rules,limits,*before.state);
     check(bounded.voluntary_exit_ssz(operation),"bounded exit admission refused");const auto full=bounded.current().token;auto second=selected_exit;second.validator_index=3;second.signature=Key(3).sign(consensus_signing_root(consensus_voluntary_exit_root(second.epoch,3),consensus_domain({4,0,0,0},version,genesis_root)));bool saturated=false;try{bounded.voluntary_exit_ssz(encode_consensus_voluntary_exit_ssz(second));}catch(const std::length_error&){saturated=true;}
     check(saturated&&bounded.current().token==full&&bounded.pending_exits().size()==1,"exit capacity failure mutated queue");check(bounded.voluntary_exit_ssz(operation)&&bounded.current().token==full,"full exit queue refused duplicate");
    }
    if(exit_wire){InboundBudget budget;CapellaConsensusService admission(*reactor,budget,16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,4,8*1024*1024);
     auto envelope=reactor->network_identity();envelope["kind"]="voluntary_exit";envelope["ssz"]=hex(operation);
     for(size_t length:{111u,113u}){auto wrong=envelope;wrong["ssz"]=hex(Bytes(length));check(!admission.decode(wrong.dump()),"exit wire size accepted");}
     for(const auto* field:{"chainId","genesisHash","forkDigest","consensusIdentity"}){auto wrong=envelope;wrong[field]="0x00";check(!admission.decode(wrong.dump()),"exit wire identity accepted");}
     auto send=[&](Json frame){auto done=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto future=done->get_future();check(admission.submit(frame.dump(),std::make_shared<std::atomic<bool>>(false),[done](auto value){done->set_value(value);}),"exit service enqueue refused");check(future.wait_for(std::chrono::seconds(10))==std::future_status::ready,"exit service deadline");return future.get();};
     auto wrong=envelope;wrong["ssz"]=hex(invalid);check(send(wrong)==CapellaConsensusService::Decision::rejected&&reactor->current().token==before.token,"invalid exit service mutated state");check(send(envelope)==CapellaConsensusService::Decision::accepted,"valid exit service rejected");
     auto compact=envelope;compact.erase("ssz");compact["sszBase64"]=altair_base64(operation);const auto session=admission.receipts().at("session").get<std::string>();const auto saved=reactor->current().token;admission.submit_receipt(session,1,compact);Json receipt;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);do{receipt=admission.receipt(session,1);if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);check(receipt.value("decision",std::string())=="accepted"&&reactor->current().token==saved,"compact exit replay changed state");admission.stop();admission.join();
    }
    check(reactor->voluntary_exit_ssz(operation),"valid queued exit refused");const auto saved=reactor->current();check(saved.token.sequence==before.token.sequence+1&&capella_store_snapshot(*saved.state,store_rules,snapshot_limits)==capella_store_snapshot(*before.state,store_rules,snapshot_limits),"exit admission changed consensus state");
    check(reactor->voluntary_exit_ssz(operation)&&reactor->current().token==saved.token,"exit duplicate changed checkpoint");
    reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,saved.token);
    if(exit_wire){const auto id=root_at(sha256(operation),0);const auto page=reactor->capture_inventory("voluntary_exit",32)->page({},1,1024);check(page.at("entries")==Json::array({hex(sha256(operation))}),"exit catalog changed root");check(reactor->inventory("voluntary_exit",{},1,1024).at("entries")==page.at("entries"),"exit inventory differs");check(reactor->export_message("voluntary_exit",id,112).at("envelope").at("ssz")==hex(operation),"exit export changed wire");bool bounded=false;try{reactor->export_message("voluntary_exit",id,111);}catch(const std::length_error&){bounded=true;}check(bounded,"exit export byte budget ignored");bounded=false;try{reactor->capture_inventory("voluntary_exit",31);}catch(const std::length_error&){bounded=true;}check(bounded,"exit root budget ignored");}
    const auto restored=reactor->pending_exits();check(restored.size()==1&&encode_consensus_voluntary_exit_ssz(restored.front())==operation,"exit lost across reopen");
    std::cout<<Json{{"scenario","capella-queued-exits"},{"invalidRejected",true},{"duplicateIdempotent",true},{"reopened",true},{"consensusUnchangedOnAdmission",true},{"capacityChecked",true}}.dump()<<std::endl;
   }
   if(queued_bls_changes&&slot==local_change_slot){
    const auto before=reactor->current();auto invalid=local_change;invalid.signature[0]^=1;
    if(queued_bls_reorg){Temp bounded_dir;ChainStore bounded_disk(bounded_dir/"queue");bounded_disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-bls-capacity");bounded_disk.commit(disk_genesis,disk_genesis_state,{});auto limits=snapshot_limits;limits.messages=1;CapellaDurableReactor bounded(bounded_disk,r.chain_id,disk_genesis.header.hash(),store_rules,limits,*before.state);check(bounded.bls_change_ssz(encode_capella_bls_change(local_change)),"bounded queue first admission failed");const auto full=bounded.current().token;auto second=local_change;second.validator_index=2;Key second_key(1002);copy_to(second.from_bls_pubkey,second_key.public_key);copy_to(second.signature,second_key.sign(consensus_signing_root(capella_bls_change_root(second),consensus_domain({10,0,0,0},r.genesis_version,genesis_root))));bool saturated=false;try{bounded.bls_change_ssz(encode_capella_bls_change(second));}catch(const std::length_error&){saturated=true;}check(saturated&&bounded.current().token==full&&bounded.pending_bls_changes().size()==1,"full queue mutated checkpoint or lost entry");check(bounded.bls_change_ssz(encode_capella_bls_change(local_change))&&bounded.current().token==full,"full queue duplicate was not idempotent");std::cout<<Json{{"scenario","capella-bls-queue-capacity"},{"capacity",1},{"validOverflowRejected",true},{"checkpointUnchanged",true},{"duplicateAccepted",true}}.dump()<<std::endl;}

    check(!(wire_mode?deliver("bls_to_execution_change",encode_capella_bls_change(invalid)):reactor->bls_change_ssz(encode_capella_bls_change(invalid)))&&reactor->current().token==before.token,"invalid queue change published");
    bool cancelled=false;try{reactor->bls_change_ssz(encode_capella_bls_change(local_change),[]{throw ExecutionCancelled("queue stopped");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&reactor->current().token==before.token,"cancelled queue change published");
    check(wire_mode?deliver("bls_to_execution_change",encode_capella_bls_change(local_change)):reactor->bls_change_ssz(encode_capella_bls_change(local_change)),"valid queue change rejected");const auto committed=reactor->current();check(committed.token.sequence==before.token.sequence+1&&capella_fork_head(*committed.state,store_rules).state==capella_fork_head(*before.state,store_rules).state,"queue admission changed consensus state");
    check(reactor->bls_change_ssz(encode_capella_bls_change(local_change))&&reactor->current().token==committed.token,"duplicate queue change published");
    auto conflicting=local_change;conflicting.to_execution_address[19]=44;copy_to(conflicting.signature,local_withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(conflicting),consensus_domain({10,0,0,0},r.genesis_version,genesis_root))));check(!reactor->bls_change_ssz(encode_capella_bls_change(conflicting))&&reactor->current().token==committed.token,"conflicting change replaced queue");
    if(wire_mode){
     auto envelope=reactor->network_identity();envelope["kind"]="bls_to_execution_change";envelope["ssz"]=hex(encode_capella_bls_change(local_change));
     for(size_t size:{size_t(171),size_t(173)}){auto bad=envelope;bad["ssz"]=hex(Bytes(size));check(!service->decode(bad.dump()),"wrong BLS wire size accepted");}
     auto compact=envelope;compact.erase("ssz");compact["sszBase64"]=altair_base64(encode_capella_bls_change(local_change));check(bool(service->decode_compact(compact)),"compact BLS decoding failed");
     const auto session=service->receipts().at("session").get<std::string>();bool wrong_session=false;try{service->submit_receipt("wrong-session",1,compact);}catch(const std::invalid_argument&){wrong_session=true;}check(wrong_session&&reactor->current().token==committed.token,"wrong session admitted BLS change");
     service->submit_receipt(session,1,compact);Json receipt;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);do{receipt=service->receipt(session,1);if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
     check(receipt.value("decision",std::string())=="accepted"&&reactor->current().token==committed.token,"compact duplicate BLS admission changed checkpoint");
    }
    if(service){service->stop();service->join();service.reset();}
    reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,committed.token);
    if(wire_mode)service=std::make_unique<CapellaConsensusService>(*reactor,ingress_budget,16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,4,8*1024*1024);
    const auto operation_wire=encode_capella_bls_change(local_change);const auto operation_id=root_at(sha256(operation_wire),0);const auto catalog=reactor->capture_inventory("bls_to_execution_change",32);const auto page=catalog->page({},1,1024);check(page.at("entries")==Json::array({hex(sha256(operation_wire))})&&page.at("next").is_null(),"BLS catalog differs");check(reactor->inventory("bls_to_execution_change",{},1,1024).at("entries")==page.at("entries"),"live BLS inventory differs");const auto exported=reactor->export_message("bls_to_execution_change",operation_id,172);check(exported.at("envelope").at("ssz")==hex(operation_wire),"BLS export changed signed bytes");bool bounded=false;try{reactor->capture_inventory("bls_to_execution_change",31);}catch(const std::length_error&){bounded=true;}check(bounded,"BLS catalog ignored root budget");bounded=false;try{reactor->export_message("bls_to_execution_change",operation_id,171);}catch(const std::length_error&){bounded=true;}check(bounded,"BLS export ignored byte budget");
    const auto restored=reactor->pending_bls_changes();check(restored.size()==1&&encode_capella_bls_change(restored[0])==encode_capella_bls_change(local_change),"pending BLS change lost after restart");
   }
   const auto current=reactor->current();auto& journal=journal_for(proposer);unsigned calls=0;auto provider=[&](const ConsensusRoot& root){++calls;return Key(proposer).sign(root);};
   check(!reactor->sign_randao(UINT64_MAX,slot,[&]{return now;},current.token,journal,provider)&&calls==0,"unassigned RANDAO signer invoked");
   const auto reveal=reactor->sign_randao(proposer,slot,[&]{return now;},current.token,journal,provider);check(reveal&&*reveal==Bytes(body.begin(),body.begin()+96)&&calls==1,"local RANDAO differs");++local_randao;
   if(slot==first_slot+1){
    auto stale=current.token;++stale.sequence;bool rejected=false;try{reactor->sign_proposal(proposer,slot,body,[&]{return now;},stale,journal,provider);}catch(const StaleCapellaDuty&){rejected=true;}check(rejected&&calls==1,"stale proposal reached signer");
    auto invalid=body;invalid[0]^=1;check(!reactor->sign_proposal(proposer,slot,invalid,[&]{return now;},current.token,journal,provider)&&calls==1,"invalid RANDAO body reached signer");
    bool cancelled=false;try{reactor->sign_proposal(proposer,slot,body,[&]{return now;},current.token,journal,provider,[]{throw ExecutionCancelled("proposal stop");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&calls==1,"cancelled proposal reached signer");
   }
   if(!finality){std::optional<Bytes> automatic;
    if(local_exits&&!queued_exits&&slot==first_slot+slots){
     auto invalid=selected_exit;invalid.signature[0]^=1;
     auto future=selected_exit;future.epoch=epoch+1;future.signature=Key(2).sign(consensus_signing_root(consensus_voluntary_exit_root(future.epoch,2),consensus_domain({4,0,0,0},version,genesis_root)));
     auto malformed=selected_exit;malformed.signature.pop_back();
     const std::array<SignedConsensusVoluntaryExit,5> candidates{malformed,invalid,future,selected_exit,selected_exit};
     automatic=reactor->prepare_local_body_with_exits(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64,candidates,5);
     check(reactor->current().token==current.token,"exit selection changed checkpoint");
     const auto limited=reactor->prepare_local_body_with_exits(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64,candidates,3);
     check(limited&&limited->size()+112==body.size(),"exit trial budget ignored");
     bool stopped=false;try{reactor->prepare_local_body_with_exits(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64,candidates,5,[]{throw ExecutionCancelled("exit selection cancelled");});}catch(const ExecutionCancelled&){stopped=true;}
     check(stopped&&reactor->current().token==current.token,"cancelled exit selection changed checkpoint");
     std::cout<<Json{{"scenario","capella-local-exits"},{"selected",1},{"invalidOrDuplicateRejected",4},{"trialBudgetChecked",true},{"cancellationChecked",true}}.dump()<<std::endl;
    }else if(local_bls_changes&&!queued_bls_changes&&slot==local_change_slot){
     auto invalid=local_change;invalid.signature[0]^=1;const std::array<SignedCapellaBlsChange,3> candidates{invalid,local_change,local_change};
     automatic=reactor->prepare_local_body_with_bls_changes(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64,candidates,3);
     check(reactor->current().token==current.token,"BLS change selection published state");
     const auto limited=reactor->prepare_local_body_with_bls_changes(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64,candidates,1);
     check(limited&&limited->size()+172==body.size(),"BLS selection ignored trial limit");
     bool stopped=false;try{reactor->prepare_local_body_with_bls_changes(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64,candidates,3,[]{throw ExecutionCancelled("change selection stopped");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped&&reactor->current().token==current.token,"BLS selection cancellation published");
    }else automatic=reactor->prepare_local_body(proposer,slot,*reveal,execution.header.coinbase,current.token,execution.transactions,16,64);
    check(automatic&&*automatic==body,"assembled body differs from independent construction");body=*automatic;++assembled_bodies;}
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
  if(beacon_reads&&slot==first_slot){const auto rendered=beacon_block_json(wire,store_rules,{}).at("message").at("body").at("bls_to_execution_changes");check(rendered.size()==1&&rendered[0].at("message").at("validator_index")=="0"&&rendered[0].at("message").at("from_bls_pubkey")==hex(Bytes(change.from_bls_pubkey.begin(),change.from_bls_pubkey.end()))&&rendered[0].at("message").at("to_execution_address")==hex(Bytes(change.to_execution_address.begin(),change.to_execution_address.end()))&&rendered[0].at("signature")==hex(Bytes(change.signature.begin(),change.signature.end())),"Beacon anchor BLS change projection differs");}
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
   if(wire_mode){catchup_anchor=*store;service=std::make_unique<CapellaConsensusService>(*reactor,ingress_budget,16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,4,8*1024*1024);Json original=reactor->network_identity();original["kind"]="block";original["ssz"]=hex(wire);const auto before=reactor->current().token;auto deny_envelope=[&](const std::string& frame){check(!service->submit(frame,std::make_shared<std::atomic<bool>>(false),[](auto){}),"bad Capella envelope enqueued");};for(const auto* field:{"chainId","genesisHash","forkDigest","consensusIdentity"}){auto bad=original;bad[field]=field==std::string("chainId")?"0x1":field==std::string("forkDigest")?"0xffffffff":hex(Bytes(32,1));deny_envelope(bad.dump());}auto bad=original;bad.erase("consensusIdentity");deny_envelope(bad.dump());bad=original;bad["kind"]="tick";deny_envelope(bad.dump());bad["kind"]="voluntary_exit";deny_envelope(bad.dump());deny_envelope("{\"kind\":\"block\","+original.dump().substr(1));deny_envelope(std::string(16*1024*1024+1,'x'));check(reactor->current().token==before,"envelope rejection changed checkpoint");}

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
   check(wire_mode?deliver("block",wire):reactor->block(wire),"durable Capella block rejected");check(capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(*admitted_store,store_rules,snapshot_limits),"durable Store differs");
   if(wire_mode&&slot==first_slot+4){stable_inventory=reactor->capture_inventory("block",4*sizeof(ConsensusRoot));stable_first=stable_inventory->page({},2,512);}
   const auto accepted_token=reactor->current().token;check(reactor->block(wire)&&reactor->current().token==accepted_token,"duplicate block rewrote checkpoint");
   const auto admitted_id=consensus_beacon_header_root(admitted);signed_wires.emplace(admitted_id,wire);
   check(reactor->signed_block(admitted_id)==std::optional<Bytes>(wire),"Capella signed archive differs");if(beacon_reads){verify_beacon_reads(*reactor,store_rules);const auto rendered=reactor->beacon_read("/eth/v2/beacon/blocks/head").body.at("data").at("message").at("body").at("bls_to_execution_changes");check(rendered.empty(),"Beacon repeated anchor BLS change");}

   check(reactor->block(bad_signature)&&reactor->current().token==accepted_token&&reactor->signed_block(admitted_id)==std::optional<Bytes>(wire),"duplicate signature replaced archive");
   store=*admitted_store;
   if(!finality&&slot==first_slot+slots){
    auto alternate_execution=execution;auto alternative_tx=tx;alternative_tx.value=1;alternate_execution.transactions[0]=SignedTransaction::sign(alternative_tx,key);if(owner_reorg)alternate_execution.transactions.clear();
    State alternate_state=state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(alternate_state,alternate_execution,false);
    auto alternative_payload=payload(alternate_execution);alternative_payload.withdrawals=withdrawals;const auto alternative_encoded=encode_capella_execution_payload(alternative_payload,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);
    auto alternate_body=body;alternate_body.resize(fixed);put(alternate_body,220+sync_bytes,fixed,4);put(alternate_body,224+sync_bytes,fixed+alternative_encoded.size(),4);alternate_body.insert(alternate_body.end(),alternative_encoded.begin(),alternative_encoded.end());

    Temp owner_directory;std::unique_ptr<Blockchain> execution_owner;std::unique_ptr<CapellaDurableReactor> bound;
    if(owner_reorg){Genesis g;g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=disk_genesis;g.state=disk_genesis_state;execution_owner=std::make_unique<Blockchain>(owner_directory/"chain",g);bound=std::make_unique<CapellaDurableReactor>(*execution_owner,store_rules,snapshot_limits,*store);}
    std::optional<ConsensusRoot> registration_root;size_t registration_misses=0,registration_hits=0;
    const std::set<Bytes> registration_keys{Key(0).public_key,Key(63).public_key,Key(64).public_key};
    const std::map<Bytes,uint64_t> registration_expected{{Key(0).public_key,0},{Key(63).public_key,63}};
    auto verify_owner=[&](const CapellaForkStore& reference){if(!bound)return;const auto& h=capella_fork_head(reference,store_rules);const auto hash=execution.transactions.front().hash();const bool included=!h.execution_block.transactions.empty();
     size_t first_checks=0,warm_checks=0;
     check(bound->current_validator_indices(registration_keys,[&]{++first_checks;})==registration_expected,"registration lookup after branch change differs");
     check(bound->current_validator_indices(registration_keys,[&]{++warm_checks;})==registration_expected,"cached registration differs");
     if(!registration_root||*registration_root!=h.header.state_root){check(first_checks>warm_checks,"changed branch reused stale registry cache");++registration_misses;}
     else check(first_checks==warm_checks,"same authenticated head missed registry cache");
     ++registration_hits;registration_root=h.header.state_root;
     bool cancelled_lookup=false;try{bound->current_validator_indices(registration_keys,[]{throw ExecutionCancelled("cached lookup stop");});}catch(const ExecutionCancelled&){cancelled_lookup=true;}check(cancelled_lookup,"cached lookup ignored cancellation");
     const auto subset=bound->current_validator_indices({Key(63).public_key});check(subset==std::map<Bytes,uint64_t>{{Key(63).public_key,63}},"changed requested keys reused stale cache");
     check(bound->current_validator_indices(registration_keys)==registration_expected,"full request after subset differs");
     check(execution_owner->tip().header.hash()==h.execution_block.header.hash()&&execution_owner->state().root()==h.execution_state.root(),"reorg owner state mismatch");
     check(bool(execution_owner->store().receipt(hash))==included&&bool(execution_owner->store().transaction(hash))==included,"reorg owner stale transaction indexes");
     check(execution_owner->pending_count()==(included?0:1)&&bool(execution_owner->pending_transaction(hash))==!included,"reorg orphan pool mismatch");
     Rpc rpc(*execution_owner);const auto response=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_getTransactionReceipt"},{"params",Json::array({hex(hash)})}});check(!response.contains("error")&&response.at("result").is_null()==!included,"reorg receipt RPC mismatch");
    };verify_owner(*store);
    const auto alternate=prepare_capella_unsigned_block(beacon,slot,proposer,parent_root,alternate_body,r,parent.header,state,history,64,100000);check(alternate.has_value(),"alternate construction failed");
    const auto branch=prepare_capella_received_block(*store,sign_message(alternate->message_ssz),store_rules,64,100000);check(branch&&branch->blocks.size()==store->blocks.size()+1,"sibling branch lost");
    const auto& branch_head=capella_fork_head(*branch,store_rules);check(branch_head.execution_state.root()==expected.root()||branch_head.execution_state.root()==alternate_state.root(),"sibling head execution mismatch");
    check(wire_mode?deliver("block",sign_message(alternate->message_ssz)):reactor->block(sign_message(alternate->message_ssz)),"durable alternate block rejected");for(const auto& [id,b]:branch->blocks)if(!store->blocks.contains(id))signed_wires.emplace(id,sign_message(alternate->message_ssz));reactor->tick((genesis_time+(slot+1)*seconds)*1000);
    if(bound){check(bound->block(sign_message(alternate->message_ssz)),"bound sibling refused");bound->tick((genesis_time+(slot+1)*seconds)*1000);}
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
     const auto next=prepare_capella_store_vote(voting,vote,store_rules,64,100000);check(next.has_value(),"Capella branch vote rejected");voting=*next;check((wire_mode?deliver("attestation",encode_consensus_attestation_ssz(vote,r.consensus.body.validators_per_committee)):reactor->vote(vote))&&capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(voting,store_rules,snapshot_limits),"durable Capella vote differs");if(bound){check(bound->vote(vote),"bound branch vote refused");verify_owner(voting);}if(beacon_reads){verify_beacon_reads(*reactor,store_rules);const auto selected=consensus_beacon_header_root(capella_fork_head(voting,store_rules).header);for(const auto& id:{canonical,alternate_root}){const auto response=reactor->beacon_read("/eth/v1/beacon/headers/"+hex(Bytes(id.begin(),id.end())));check(response.status==200&&response.body.at("data").at("canonical")==Json(id==selected)&&response.body.at("finalized")==false,"Beacon sibling canonical/finalized flag differs");}}++votes;return std::pair{members[position],vote};
    };
    auto verify_bls_branch=[&](bool applicable,bool reopen){
     if(queued_exits){const auto saved=reactor->current().token;if(reopen){reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,saved);}
      const auto pending=reactor->pending_exits();check(pending.size()==size_t(applicable),"exit applicability differs after reorg");if(applicable)check(encode_consensus_voluntary_exit_ssz(pending.front())==encode_consensus_voluntary_exit_ssz(selected_exit),"reorg exit bytes changed");
      const auto record=reactor_disk->fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes,saved);check(record&&Json::parse(record->snapshot).at("signedExits").size()==1,"unfinalized exit erased from queue");
      std::cout<<Json{{"scenario","capella-exit-reorg"},{"applicable",applicable},{"reopened",reopen},{"retained",true}}.dump()<<std::endl;
     }
     if(!queued_bls_reorg)return;
     const auto before=reactor->current();if(reopen){reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(*reactor_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,before.token);check(reactor->current().token==before.token,"BLS branch restart changed checkpoint");}
     const auto pending=reactor->pending_bls_changes();check(pending.size()==size_t(applicable),"BLS candidate applicability wrong after reorg");const auto catalog=reactor->capture_inventory("bls_to_execution_change",32)->page({},1,1024);check(catalog.at("entries").size()==size_t(applicable),"BLS catalog did not follow reorg");
     const auto record=reactor_disk->fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes,before.token);check(record&&Json::parse(record->snapshot).at("blsChanges").size()==1,"unfinalized BLS change removed from durable queue");
     if(applicable){check(encode_capella_bls_change(pending[0])==encode_capella_bls_change(local_change),"reorg changed BLS operation");const auto current=reactor->current();const auto& head=capella_fork_head(*current.state,store_rules);const auto next_slot=slot+1;const auto next_proposer=prepare_capella_proposer_identity(head.state,head.header,next_slot,epoch_rules,l,duties,64,100000);check(bool(next_proposer),"reorg proposer missing");ConsensusRoot next_epoch{};for(unsigned i=0;i<8;++i)next_epoch[i]=uint8_t((next_slot/slots)>>(8*i));const auto randao_reveal=Key(next_proposer->index).sign(consensus_signing_root(next_epoch,consensus_domain({2,0,0,0},version,genesis_root)));const auto body=reactor->prepare_local_body(next_proposer->index,next_slot,randao_reveal,head.execution_block.header.coinbase,current.token,{},16,64);const auto operation=encode_capella_bls_change(local_change);check(body&&body->size()>=operation.size()&&std::equal(operation.begin(),operation.end(),body->end()-operation.size()),"orphaned BLS change not reselected into valid proposal");check(reactor->current().token==current.token,"reorg proposal mutated durable state");}
    };
    const auto first_vote=vote_for(0,canonical);check(capella_fork_head(voting,store_rules).execution_state.root()==expected.root(),"canonical vote head mismatch");verify_bls_branch(false,false);
    vote_for(1,alternate_root);vote_for(2,alternate_root);check(capella_fork_head(voting,store_rules).execution_state.root()==alternate_state.root(),"votes did not select alternate EVM state");++vote_reorgs;verify_bls_branch(true,true);
    vote_for(3,canonical);vote_for(4,canonical);vote_for(5,canonical);check(capella_fork_head(voting,store_rules).execution_state.root()==expected.root(),"votes did not restore canonical EVM state");++vote_reorgs;verify_bls_branch(false,true);if(queued_bls_reorg)std::cout<<Json{{"scenario","capella-bls-reorg"},{"branchSwitches",2},{"checkpointReopens",2},{"orphanedChangeReselected",true},{"retainedUntilFinality",true},{"networkTransport",false}}.dump()<<std::endl;roundtrip(voting);if(bound)std::cout<<Json{{"scenario","capella-owner-reorg"},{"branchSwitches",2},{"orphanReinserted",true},{"receiptRpcChecked",true},{"networkTransport",false}}.dump()<<std::endl;
    auto conflicting=first_vote.second;conflicting.data.beacon_block_root=alternate_root;if(slot%slots==0)conflicting.data.target.root=alternate_root;
    conflicting.signature=Key(first_vote.first).sign(consensus_signing_root(consensus_attestation_root(conflicting.data),consensus_domain({1,0,0,0},version,genesis_root)));
    const auto slashed=prepare_capella_store_slashing(voting,{first_vote.second.data,{first_vote.first},first_vote.second.signature},{conflicting.data,{first_vote.first},conflicting.signature},store_rules);
    const IndexedConsensusAttestation proof_a{first_vote.second.data,{first_vote.first},first_vote.second.signature},proof_b{conflicting.data,{first_vote.first},conflicting.signature};
    const auto before_proof=reactor->current().token;auto bad_proof=proof_b;bad_proof.signature[0]^=1;check(!reactor->slashing(proof_a,bad_proof)&&reactor->current().token==before_proof&&reactor->signed_slashings().empty(),"invalid slashing persisted");
    bool proof_cancelled=false;try{reactor->slashing(proof_a,proof_b,[]{throw ExecutionCancelled("proof stop");});}catch(const ExecutionCancelled&){proof_cancelled=true;}check(proof_cancelled&&reactor->current().token==before_proof,"cancelled slashing persisted");
    check(slashed&&slashed->timed.view.equivocating_indices.contains(first_vote.first),"Capella equivocation evidence rejected");roundtrip(*slashed);check((wire_mode?deliver("attester_slashing",encode_consensus_attester_slashing_ssz(proof_a,proof_b,r.consensus.body.validators_per_committee)):reactor->slashing(proof_a,proof_b))&&capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(*slashed,store_rules,snapshot_limits),"durable Capella slashing differs");
    const auto proof_token=reactor->current().token;check(reactor->signed_slashings().size()==1,"missing slashing archive");
    check(reactor->slashing(proof_a,proof_b)&&reactor->slashing(proof_b,proof_a)&&reactor->current().token==proof_token,"duplicate/reversed slashing rewrote checkpoint");
    for(unsigned variant=1;variant<=4;++variant){auto alternative=proof_b;alternative.data.beacon_block_root[0]^=uint8_t(variant);alternative.signature=Key(first_vote.first).sign(consensus_signing_root(consensus_attestation_root(alternative.data),consensus_domain({1,0,0,0},version,genesis_root)));check(reactor->slashing(proof_a,alternative)&&reactor->signed_slashings().size()==1&&reactor->current().token==proof_token,"equivalent offender grew archive");}
    if(bound){check(registration_misses>=2&&registration_hits>=3,"registration cache branch coverage missing");std::cout<<Json{{"scenario","capella-registration-cache-reorg"},{"rootMisses",registration_misses},{"warmHits",registration_hits},{"cancelledHitRejected",true},{"requestChangeChecked",true}}.dump()<<std::endl;}
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
      check((wire_mode?deliver("attestation",encode_consensus_attestation_ssz(vote,r.consensus.body.validators_per_committee)):reactor->vote(vote))&&capella_store_snapshot(*reactor->current().state,store_rules,snapshot_limits)==capella_store_snapshot(*before.state,store_rules,snapshot_limits),"current-slot vote changed fork choice");
      const auto accepted=reactor->current().token;check(accepted!=before.token&&reactor->vote(vote)&&reactor->current().token==accepted,"deferred duplicate rewrote checkpoint");deferred_votes.push_back(vote);
     }
    }
   }
  }
  if(slot==first_slot){short_snapshot=capella_store_snapshot(*store,store_rules,snapshot_limits);if(deposit_reorg){deposit_reorg_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(configured_runtime){runtime_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(scheduled){scheduler_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}
  if(local_finality){local_finality_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}}
  ++contexts;
  beacon=received->consensus;state=received->execution.state;parent=received->execution.block;hashes[parent.header.number]=parent.header.hash();++accepted;
 }
 if(finality){if(participation==100)check(store->timed.view.finalized.epoch>=2&&store->timed.view.justified.epoch>=3&&included_attestations>=128,"Capella participation did not advance finality");else check(store->timed.view.finalized.epoch==0&&store->timed.view.justified.epoch==0,"insufficient participation finalized a checkpoint");}
 if(wire_mode){
  check(bool(stable_inventory)&&stable_inventory->page({},2,512)==stable_first,"inventory changed across block progression");
  const auto next_bytes=unhex(stable_first.at("next").get<std::string>());ConsensusRoot stable_cursor{};std::copy(next_bytes.begin(),next_bytes.end(),stable_cursor.begin());const auto stable_last=stable_inventory->page(stable_cursor,2,512);check(stable_last.at("entries").size()==2&&stable_last.at("next").is_null()&&stable_last.at("digest")==stable_first.at("digest")&&stable_last.at("digest")!=hex(reactor->current().token.digest),"stable inventory lost continuation");
  bool snapshot_refused=false;try{reactor->capture_inventory("block",1);}catch(const std::length_error&){snapshot_refused=true;}check(snapshot_refused,"snapshot root budget ignored");
  snapshot_refused=false;try{stable_inventory->page(ConsensusRoot{},2,512);}catch(const std::invalid_argument&){snapshot_refused=true;}check(snapshot_refused,"snapshot unknown cursor accepted");
  auto catalog_now=std::chrono::steady_clock::time_point{};CapellaInventorySessions catalogs(*reactor,2,9*sizeof(ConsensusRoot),std::chrono::seconds(1),[&]{return catalog_now;});
  const auto handle=catalogs.open("peer-a","block");check(catalogs.page("peer-a",handle,{},2,512).at("entries").size()==2,"catalog handle failed");
  bool catalog_refused=false;try{catalogs.page("peer-b",handle,{},2,512);}catch(const std::invalid_argument&){catalog_refused=true;}check(catalog_refused,"foreign principal read catalog");
  catalog_refused=false;try{catalogs.close("peer-b",handle);}catch(const std::invalid_argument&){catalog_refused=true;}check(catalog_refused,"foreign principal closed catalog");
  catalog_refused=false;try{catalogs.open("peer-a","attestation");}catch(const std::length_error&){catalog_refused=true;}check(catalog_refused,"catalog aggregate byte quota ignored");
  catalogs.close("peer-a",handle);catalogs.close("peer-a",handle); // lost close ACK: same request is safe
  catalog_refused=false;try{catalogs.close("",handle);}catch(const std::invalid_argument&){catalog_refused=true;}check(catalog_refused,"anonymous idempotent close accepted");
  catalog_refused=false;try{catalogs.close("peer-a","bad-handle");}catch(const std::invalid_argument&){catalog_refused=true;}check(catalog_refused,"malformed idempotent close accepted");
  const auto vote_handle=catalogs.open("peer-a","attestation");const auto slash_handle=catalogs.open("peer-b","attester_slashing");
  catalog_refused=false;try{catalogs.open("peer-c","attester_slashing");}catch(const std::length_error&){catalog_refused=true;}check(catalog_refused,"catalog handle quota ignored");
  catalog_now+=std::chrono::seconds(1);catalog_refused=false;try{catalogs.page("peer-a",vote_handle,{},2,512);}catch(const std::invalid_argument&){catalog_refused=true;}check(catalog_refused,"expired catalog readable");catalogs.close("peer-a",vote_handle);catalogs.close("peer-a",vote_handle);
  const auto replacement=catalogs.open("peer-a","block");check(replacement!=handle&&catalogs.page("peer-a",replacement,{},2,512).at("entries").size()==2,"expired quota not reclaimed");catalogs.close("peer-a",replacement);
  // Request-addressed catalogs survive a lost open ACK without waiting for TTL.
  {
   CapellaInventorySessions leases(*reactor,2,9*sizeof(ConsensusRoot),std::chrono::seconds(1),[&]{return catalog_now;});
   const auto id=[](char c){return "0x"+std::string(64,c);};auto gen=leases.generation();
   const auto opened=leases.acquire("a",gen,id('1'),"block");
   check(leases.acquire("a",gen,id('1'),"block")==opened,"duplicate acquire allocated another snapshot");
   bool refused=false;try{leases.acquire("a",gen,id('1'),"attestation");}catch(const std::invalid_argument&){refused=true;}check(refused,"acquire changed kind");
   leases.release("b",gen,id('1'));check(!leases.page("a",opened,{},2,512).at("entries").empty(),"foreign release removed snapshot");
   leases.release("a",gen,id('1'));leases.release("a",gen,id('1'));
   refused=false;try{leases.acquire("a",gen,id('1'),"block");}catch(const std::invalid_argument&){refused=true;}check(refused,"released request resurrected");
   gen=leases.generation();leases.release("a",gen,id('2'));
   refused=false;try{leases.acquire("a",gen,id('2'),"block");}catch(const std::invalid_argument&){refused=true;}check(refused,"cancel-before-acquire resurrected");
   gen=leases.generation();const auto live=leases.acquire("a",gen,id('3'),"block");
   for(char c='4';c<='8';++c)leases.release("a",leases.generation(),id(c));
   check(leases.generation()!=gen&&!leases.page("a",live,{},2,512).at("entries").empty(),"bounded retirement lost live handle");
   leases.release("a",gen,id('3')); // old generation still releases live request
   const auto current=leases.generation();const auto replacement=leases.acquire("a",current,id('9'),"block");
   catalog_now+=std::chrono::seconds(1);
   refused=false;try{leases.acquire("a",current,id('9'),"block");}catch(const std::invalid_argument&){refused=true;}check(refused,"expired acquire extended lease");
   CapellaInventorySessions restarted(*reactor,2,9*sizeof(ConsensusRoot),std::chrono::seconds(1));
   refused=false;try{restarted.acquire("a",current,id('9'),"block");}catch(const std::invalid_argument&){refused=true;}check(refused,"restart accepted stale generation");
   leases.stop();refused=false;try{leases.generation();}catch(const std::runtime_error&){refused=true;}check(refused,"stopped catalog exposed generation");
  }
  if(catalog_concurrency)std::cout<<qualify_capella_catalog_concurrency(*reactor).dump()<<std::endl;
  const auto checkpoint=reactor->current();size_t exports=0;
  auto verify_export=[&](const char* kind,const ConsensusRoot& id,const Bytes& bytes){const auto result=reactor->export_message(kind,id,16*1024*1024,checkpoint.token);check(result.at("available")==true&&result.at("envelope").at("ssz")==hex(bytes)&&result.at("digest")==hex(checkpoint.token.digest),"Capella export changed bytes/checkpoint");auto decoded=service->decode(result.at("envelope").dump());check(decoded&&decoded->ssz==bytes,"Capella exported envelope cannot be admitted");++exports;};
  for(const auto& [id,wire]:signed_wires)verify_export("block",id,wire);
  for(const auto& [id,wire]:reactor->signed_votes()){ConsensusRoot root{};std::copy(id.begin(),id.end(),root.begin());verify_export("attestation",root,wire);}
  for(const auto& [id,wire]:reactor->signed_slashings()){ConsensusRoot root{};std::copy(id.begin(),id.end(),root.begin());verify_export("attester_slashing",root,wire);}
  size_t listed=0;for(const auto* kind:{"block","attestation","attester_slashing"}){std::optional<ConsensusRoot> cursor;std::set<std::string> seen;do{const auto page=reactor->inventory(kind,cursor,2,512,checkpoint.token);check(page.dump().size()<=512&&page.at("entries").size()<=2,"inventory exceeded budget");for(const auto& entry:page.at("entries")){const auto bytes=unhex(entry.get<std::string>());ConsensusRoot id{};std::copy(bytes.begin(),bytes.end(),id.begin());check(seen.insert(entry.get<std::string>()).second&&reactor->export_message(kind,id,16*1024*1024,checkpoint.token).at("available")==true,"inventory duplicate or unavailable archive");++listed;}if(page.at("next").is_null())break;const auto bytes=unhex(page.at("next").get<std::string>());ConsensusRoot id{};std::copy(bytes.begin(),bytes.end(),id.begin());cursor=id;}while(true);}check(listed==16,"inventory omitted messages");
  bool inventory_refused=false;try{reactor->inventory("block",{},1,1);}catch(const std::length_error&){inventory_refused=true;}check(inventory_refused,"inventory ignored byte capacity");
  auto inventory_stale=checkpoint.token;++inventory_stale.sequence;inventory_refused=false;try{reactor->inventory("block",{},1,512,inventory_stale);}catch(const std::invalid_argument&){inventory_refused=true;}check(inventory_refused,"inventory accepted stale checkpoint");
  inventory_refused=false;try{reactor->inventory("block",signed_wires.begin()->first,1,512);}catch(const std::invalid_argument&){inventory_refused=true;}check(inventory_refused,"unfenced continuation accepted");
  inventory_refused=false;try{reactor->inventory("block",ConsensusRoot{},1,512,checkpoint.token);}catch(const std::invalid_argument&){inventory_refused=true;}check(inventory_refused,"unknown inventory cursor accepted");
  const auto anchor=reactor->export_message("block",checkpoint.state->timed.view.anchor,16*1024*1024);check(anchor.at("available")==false&&anchor.at("known")==true&&anchor.at("anchor")==true,"Capella export fabricated anchor signature");
  ConsensusRoot unknown{};check(reactor->export_message("block",unknown,1024).at("available")==false,"unknown export available");
  auto stale=checkpoint.token;++stale.sequence;bool refused=false;try{reactor->export_message("block",signed_wires.begin()->first,16*1024*1024,stale);}catch(const std::invalid_argument&){refused=true;}check(refused,"stale archive checkpoint accepted");
  refused=false;try{reactor->export_message("block",signed_wires.begin()->first,1);}catch(const std::length_error&){refused=true;}check(refused,"archive export ignored byte limit");
  refused=false;try{reactor->export_message("block",unknown,1024,{},[]{throw ExecutionCancelled("export cancelled");});}catch(const ExecutionCancelled&){refused=true;}check(refused&&reactor->current().token==checkpoint.token&&exports==16,"export changed consensus or missed messages");
 }
 if(wire_mode){
  const auto before=reactor->current().token;auto bad=sync_wire;bad.at(184+168)^=1;check(!deliver("block",bad)&&reactor->current().token==before,"invalid signed wire changed checkpoint");
  Json envelope=reactor->network_identity();envelope["kind"]="block";envelope["ssz"]=hex(sync_wire);const auto frame=envelope.dump();
  std::promise<void> entered,release;auto gate=release.get_future();auto started=entered.get_future();
  CapellaConsensusService bounded(*reactor,ingress_budget,16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,1,8*1024*1024,[&](const auto& check){entered.set_value();gate.wait();check();});
  auto promise=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto result=promise->get_future();check(bounded.submit(frame,std::make_shared<std::atomic<bool>>(false),[promise](auto d){promise->set_value(d);}),"bounded worker first job refused");started.wait();const bool extra=bounded.submit(frame,std::make_shared<std::atomic<bool>>(false),[](auto){});bounded.stop();release.set_value();bounded.join();check(!extra&&result.get()==CapellaConsensusService::Decision::local_failure&&reactor->current().token==before,"saturated/stopped worker published");
  CapellaConsensusService tiny(*reactor,ingress_budget,16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,4,1);check(!tiny.submit(frame,std::make_shared<std::atomic<bool>>(false),[](auto){}),"worker byte budget ignored");
 }
 if(wire_mode){
  Temp follower_directory;ChainStore follower_disk(follower_directory/"db");follower_disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-reactor-test");follower_disk.commit(disk_genesis,disk_genesis_state,{});
  CapellaDurableReactor follower(follower_disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,*catchup_anchor);InboundBudget follower_budget;CapellaConsensusService follower_service(follower,follower_budget,16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,2,8*1024*1024);
  const auto source=reactor->current();follower.tick(source.state->timed.clock.time_ms);size_t fetched=0;
  for(const auto* kind:{"block","attestation","attester_slashing"}){
   const auto catalog=reactor->capture_inventory(kind,1000*sizeof(ConsensusRoot));std::optional<ConsensusRoot> cursor;std::vector<Json> messages;size_t buffered=0;
   for(;;){const auto page=catalog->page(cursor,2,512);for(const auto& entry:page.at("entries")){const auto bytes=unhex(entry.get<std::string>());ConsensusRoot root{};std::copy(bytes.begin(),bytes.end(),root.begin());auto message=reactor->export_message(kind,root,8*1024*1024,source.token);check(message.at("available")==true,"catchup archive unavailable");buffered+=message.dump().size();check(messages.size()<1000&&buffered<=16*1024*1024,"catchup batch exceeds budget");messages.push_back(std::move(message));}if(page.at("next").is_null())break;const auto bytes=unhex(page.at("next").get<std::string>());ConsensusRoot root{};std::copy(bytes.begin(),bytes.end(),root.begin());cursor=root;}
   if(std::string_view(kind)=="block"){
    std::vector<CapellaSyncBlock> batch;for(const auto& message:messages){const auto bytes=unhex(message.at("root").get<std::string>());ConsensusRoot root{};std::copy(bytes.begin(),bytes.end(),root.begin());batch.push_back({root,unhex(message.at("envelope").at("ssz").get<std::string>())});}
    const std::set<ConsensusRoot> known{catchup_anchor->timed.view.anchor};const auto order=order_capella_sync_blocks(batch,known,store_rules,1000,16*1024*1024);
    auto bad=batch;bad[0].root[0]^=1;bool refused=false;try{order_capella_sync_blocks(bad,known,store_rules,1000,16*1024*1024);}catch(const std::invalid_argument&){refused=true;}check(refused,"sync root claim trusted");
    bad=batch;bad.push_back(batch[0]);refused=false;try{order_capella_sync_blocks(bad,known,store_rules,1000,16*1024*1024);}catch(const std::invalid_argument&){refused=true;}check(refused,"sync duplicate accepted");
    refused=false;try{order_capella_sync_blocks(batch,{},store_rules,1000,16*1024*1024);}catch(const std::invalid_argument&){refused=true;}check(refused,"sync missing anchor accepted");
    refused=false;try{order_capella_sync_blocks(batch,known,store_rules,1000,1);}catch(const std::length_error&){refused=true;}check(refused,"sync payload budget ignored");
    std::set<ConsensusRoot> parents;for(const auto& message:messages){const auto bytes=unhex(message.at("parentRoot").get<std::string>());ConsensusRoot root{};std::copy(bytes.begin(),bytes.end(),root.begin());parents.insert(root);}std::vector<ConsensusRoot> tips;for(const auto& block:batch)if(!parents.contains(block.root))tips.push_back(block.root);
    size_t requests=0;auto fetch=[&](const ConsensusRoot& id,size_t allowance,const std::function<void()>& check){++requests;if(check)check();const auto exported=reactor->export_message("block",id,allowance);if(!exported.at("available").get<bool>())return std::optional<Bytes>{};return std::optional<Bytes>{unhex(exported.at("envelope").at("ssz").get<std::string>())};};
    const auto collected=collect_capella_sync_blocks(tips,known,store_rules,1000,16*1024*1024,fetch);check(collected.size()==batch.size()&&requests==batch.size()&&tips.size()==2,"dependency fetch omitted or repeated ancestors");for(size_t i=0;i<order.size();++i)check(collected[i].root==batch[order[i]].root,"dependency fetch order differs");
    refused=false;try{collect_capella_sync_blocks(tips,known,store_rules,1000,16*1024*1024,[](const auto&,size_t,const auto&){return std::optional<Bytes>{};});}catch(const std::invalid_argument&){refused=true;}check(refused,"unavailable dependency ignored");
    refused=false;try{collect_capella_sync_blocks(tips,known,store_rules,2,16*1024*1024,fetch);}catch(const std::length_error&){refused=true;}check(refused,"dependency count budget ignored");
    const auto before_cancel=requests;refused=false;try{collect_capella_sync_blocks(tips,known,store_rules,1000,16*1024*1024,fetch,[]{throw ExecutionCancelled("dependency cancelled");});}catch(const ExecutionCancelled&){refused=true;}check(refused&&requests==before_cancel,"cancelled dependency started I/O");
    std::vector<Json> sorted;for(size_t i=0;i<order.size();++i){auto message=std::move(messages[order[i]]);message["envelope"]["ssz"]=hex(collected[i].ssz);sorted.push_back(std::move(message));}messages=std::move(sorted);
   }

   if(std::string_view(kind)=="block"){
    asio::io_context io;InboundBudget delivery_budget;const auto identity=reactor->network_identity();auto ingress=std::make_shared<AltairMessageIngress>(AltairWireProfile{identity.at("chainId").get<std::string>(),identity.at("genesisHash").get<std::string>(),identity.at("forkDigest").get<std::string>(),16*1024*1024,r.consensus.maximum_block_bytes,65536,65536,identity.at("consensusIdentity").get<std::string>()},delivery_budget);auto knowledge=std::make_shared<AltairRelayProgress>(1000);size_t exports=0;
    auto core=[&](std::string method,Json params,Reply reply){check(method=="nano_capellaExport"&&params[0]=="test-token"&&params[1]=="block","unexpected delivery export");++exports;const auto bytes=unhex(params[2].get<std::string>());ConsensusRoot id{};std::copy(bytes.begin(),bytes.end(),id.begin());reply(reactor->export_message("block",id,8*1024*1024));};
    auto send=[&](Json frame,Reply reply){const bool queued=follower_service.submit(frame.dump(),std::make_shared<std::atomic<bool>>(false),[&,reply](auto decision){const bool accepted=decision==CapellaConsensusService::Decision::accepted;if(accepted)++fetched;reply(Json{{"accepted",accepted}});});if(!queued)reply(nullptr);};
    const auto& anchor=catchup_anchor->timed.view.anchor;
    // Deliberately ask for descendants first; delivery must discover ancestors.
    for(auto it=messages.rbegin();it!=messages.rend();++it){io.restart();Json result;auto delivery=std::make_shared<CapellaBlockDelivery>(io,core,send,"test-token",hex(Bytes(anchor.begin(),anchor.end())),it->at("root").get<std::string>(),ingress,knowledge,1000,16*1024*1024,[&](Json value){result=value;});delivery->start(std::chrono::seconds(10));io.run();check(result.at("accepted")==true,"ancestor-first delivery failed");}
    check(exports==messages.size()&&fetched==messages.size(),"delivery duplicated confirmed ancestors");continue;
   }
   for(const auto& message:messages){auto promise=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto result=promise->get_future();check(follower_service.submit(message.at("envelope").dump(),std::make_shared<std::atomic<bool>>(false),[promise](auto d){promise->set_value(d);}),"catchup enqueue refused");check(result.wait_for(std::chrono::seconds(10))==std::future_status::ready&&result.get()==CapellaConsensusService::Decision::accepted,"catchup message rejected");++fetched;}
  }
  follower_service.stop();follower_service.join();const auto recovered=follower.current();const auto& expected_head=capella_fork_head(*source.state,store_rules);const auto& recovered_head=capella_fork_head(*recovered.state,store_rules);
  check(fetched==16&&consensus_beacon_header_root(expected_head.header)==consensus_beacon_header_root(recovered_head.header)&&expected_head.execution_state.root()==recovered_head.execution_state.root()&&source.state->timed.view.finalized==recovered.state->timed.view.finalized,"catchup head/state/finality diverged");
  check(follower.signed_votes()==reactor->signed_votes()&&follower.signed_slashings()==reactor->signed_slashings(),"catchup lost gossip evidence");
 }
 if(service){service->stop();service->join();service.reset();}
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
 if(queued_bls_sync){qualify_bls_sync(executable,store_rules,short_snapshot,disk_genesis,disk_genesis_state,sync_time,encode_capella_bls_change(local_change));return;}
 if(sync_scenario){
  Genesis owner_genesis;owner_genesis.chain_id=r.chain_id;owner_genesis.consensus="beacon";owner_genesis.evm_revision=EvmRevision::Shanghai;owner_genesis.block=disk_genesis;owner_genesis.state=disk_genesis_state;
  for(unsigned disposition=0;disposition<(owner_witness?3u:2u);++disposition){const bool lose=disposition==1,fault_witness=disposition==2;Temp tmp;
   if(owner_sync){ChainStore witness(tmp/"witness");Blockchain owner(tmp/"db",owner_genesis);auto first=capella_store_from_snapshot(short_snapshot,sha256(Bytes(short_snapshot.begin(),short_snapshot.end())),store_rules,snapshot_limits);CapellaDurableReactor initial(owner,store_rules,snapshot_limits,std::move(first),{},owner_witness?&witness:nullptr);}else{ChainStore disk(tmp/"db");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-sync-test");disk.commit(disk_genesis,disk_genesis_state,{});auto first=capella_store_from_snapshot(short_snapshot,sha256(Bytes(short_snapshot.begin(),short_snapshot.end())),store_rules,snapshot_limits);CapellaDurableReactor initial(disk,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,std::move(first));}
   if(owner_witness)fs::copy(tmp/"db",tmp/"stale",fs::copy_options::recursive);
   save(tmp/"bundle.json",Json{{"profile",profile},{"genesis",disk_genesis.to_json(true)},{"wire",hex(sync_wire)},{"time",sync_time},{"executionOwner",owner_sync},{"alloc",disk_genesis_state.to_json()},{"witness",owner_witness},{"faultWitness",fault_witness}});
   const auto result=success(run({executable,"--sync-writer",tmp.path.string()}));const auto wal=tmp/(fault_witness?"witness":"db")/result.at("wal").get<std::string>();check(fs::file_size(wal)==result.at("after").get<uint64_t>()&&result.at("after")>result.at("before"),"sync WAL suffix missing");
   if(lose){const int fd=open(wal.c_str(),O_WRONLY);check(fd>=0,"open owned WAL");const bool ok=ftruncate(fd,result.at("before").get<uint64_t>())==0&&fsync(fd)==0;close(fd);check(ok,"truncate owned WAL");}
   const auto expected=load(tmp/"expected.json");std::unique_ptr<ChainStore> standalone;std::unique_ptr<Blockchain> owner;if(owner_sync)owner=std::make_unique<Blockchain>(tmp/"db",owner_genesis);else standalone=std::make_unique<ChainStore>(tmp/"db");const auto& disk=owner_sync?owner->store():*standalone;const auto record=disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes);
   check(record&&record->snapshot==expected.at((lose||fault_witness)?"old":"new").get<std::string>()&&record->token.sequence==expected.at("sequence").get<uint64_t>()+((lose||fault_witness)?0:1),"mixed Capella recovery disposition");
   if(owner_witness){
    ChainStore witness(tmp/"witness");auto resumed=std::make_unique<CapellaDurableReactor>(*owner,store_rules,snapshot_limits,witness);const auto current=resumed->current();
    check(capella_store_snapshot(*current.state,store_rules,snapshot_limits)==expected.at("new").get<std::string>(),"witness failed to finish pending publication");const auto& head=capella_fork_head(*current.state,store_rules);const auto id=consensus_beacon_header_root(head.header);
    check(resumed->signed_block(id)==std::optional<Bytes>(sync_wire),"witness did not restore signed archive");check(owner->tip().header.hash()==head.execution_block.header.hash()&&owner->state().root()==head.execution_state.root(),"witness owner state mismatch");
    for(const auto& tx:head.execution_block.transactions)check(disk.receipt(tx.hash())&&disk.transaction(tx.hash()),"witness transaction indexes missing");
    Rpc rpc(*owner);check(!rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_blockNumber"},{"params",Json::array()}}).contains("error"),"witness owner RPC unavailable");
    const auto token=current.token;resumed.reset();
    {const size_t budget=2048+8*snapshot_limits.envelope_bytes;const auto saved=witness.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),budget);check(bool(saved),"witness intent missing");auto corrupt=Json::parse(saved->snapshot);auto wire=unhex(corrupt.at("archive").at(0).at(2).get<std::string>());wire[4]^=1;corrupt["archive"][0][2]=hex(wire);ChainStore malformed(tmp/"bad-witness");malformed.bind_chain(r.chain_id,disk_genesis.header.hash(),"nano-capella-recovery-v1");malformed.commit_fork_checkpoint(r.chain_id,disk_genesis.header.hash(),{},corrupt.dump(),budget);bool refused=false;try{CapellaDurableReactor invalid(*owner,store_rules,snapshot_limits,malformed);}catch(const std::invalid_argument&){refused=true;}check(refused&&disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes)->token==token,"corrupt witness archive accepted");}
    {ChainStore empty(tmp/"empty-witness");bool refused=false;try{CapellaDurableReactor invalid(*owner,store_rules,snapshot_limits,empty);}catch(const std::invalid_argument&){refused=true;}check(refused&&disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes)->token==token,"empty witness authorized recovery");}
    {Blockchain stale(tmp/"stale",owner_genesis);bool refused=false;try{CapellaDurableReactor invalid(stale,store_rules,snapshot_limits,witness);}catch(const std::invalid_argument&){refused=true;}check(refused,"witness accepted older rollback");}
    resumed=std::make_unique<CapellaDurableReactor>(*owner,store_rules,snapshot_limits,witness);check(resumed->current().token==token,"witness duplicate restart changed token");
    std::cout<<Json{{"scenario","capella-owner-witness"},{"lostSuffix",lose},{"faultWitness",fault_witness},{"corruptArchiveRejected",true},{"blockedOperations",result.at("blocked")},{"restoredSignedArchive",true},{"emptyWitnessRejected",true},{"oldRollbackRejected",true},{"physicalPowerLoss",false},{"networkActivated",false}}.dump()<<std::endl;continue;
   }
   // The test resolves both known outcomes; production must authenticate its restart token independently.
   auto coordinator=owner_sync?std::make_unique<CapellaDurableReactor>(*owner,store_rules,snapshot_limits,record->token):std::make_unique<CapellaDurableReactor>(*standalone,r.chain_id,disk_genesis.header.hash(),store_rules,snapshot_limits,record->token);auto& restored=*coordinator;const auto recovered=restored.current();const auto& head=capella_fork_head(*recovered.state,store_rules);const auto latest=disk.latest();
   check(latest&&latest->header.hash()==head.execution_block.header.hash()&&latest->withdrawals==head.execution_block.withdrawals&&disk.state_at(latest->header.number)->root()==head.execution_state.root()&&disk.count()==latest->header.number+1,"Capella recovery paired state mismatch");
   for(const auto& [id,b]:recovered.state->blocks)for(const auto& tx:b.execution_block.transactions)check(disk.receipt(tx.hash())&&disk.receipt(tx.hash())->block_hash==b.execution_block.header.hash()&&disk.transaction(tx.hash()),"Capella recovered transaction index missing");
   const auto candidate_bytes=expected.at("new").get<std::string>();const auto candidate=capella_store_from_snapshot(candidate_bytes,sha256(Bytes(candidate_bytes.begin(),candidate_bytes.end())),store_rules,snapshot_limits);const auto& next_head=capella_fork_head(candidate,store_rules);
   const auto next_id=consensus_beacon_header_root(next_head.header);check(bool(disk.checkpoint_archive(Bytes(next_id.begin(),next_id.end()),snapshot_limits.envelope_bytes))==!lose,"Capella archive detached from WAL outcome");
   check(bool(disk.block_by_number(next_head.execution_block.header.number))==!lose,"lost Capella block remained indexed");for(const auto& tx:next_head.execution_block.transactions)check(bool(disk.receipt(tx.hash()))==!lose&&bool(disk.transaction(tx.hash()))==!lose,"lost Capella transaction remained indexed");
   check(restored.block(sync_wire),"Capella replay after recovery rejected");const auto completed=restored.current();check(restored.signed_block(next_id)==std::optional<Bytes>(sync_wire),"Capella replay archive differs");check(capella_store_snapshot(*completed.state,store_rules,snapshot_limits)==expected.at("new").get<std::string>(),"Capella recovery replay differs");
   if(owner_sync){const auto& h=capella_fork_head(*completed.state,store_rules);check(owner->tip().header.hash()==h.execution_block.header.hash()&&owner->state().root()==h.execution_state.root(),"recovered owner cache mismatch");Rpc rpc(*owner);const auto response=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_getBlockByNumber"},{"params",Json::array({"latest",false})}});check(!response.contains("error")&&response.at("result").at("hash")==hex(h.execution_block.header.hash()),"recovered owner RPC mismatch");}
   std::cout<<Json{{"scenario","capella-reactor-sync"},{"executionOwner",owner_sync},{"lostSuffix",lose},{"blockedOperations",result.at("blocked")},{"replay",true},{"physicalPowerLoss",false},{"networkActivated",false}}.dump()<<'\n';
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
 if(local_bls_changes&&!queued_bls_reorg)check(paid>=2&&state.get("0x000000000000000000000000000000000000002b").balance>0,"locally selected credential change never paid recipient");
 std::cout<<Json{{"queuedBlsChangeSelection",queued_bls_changes},{"localBlsChangeSelection",local_bls_changes},{"wireMessages",wire_messages},{"beaconReadChecks",beacon_read_checks},{"scenario","capella-signed-transition"},{"migrated",migrate},{"finalityScenario",finality},{"participationPercent",participation},{"finalizedEpoch",store->timed.view.finalized.epoch},{"includedAttestations",included_attestations},{"skippedForkSlots",skipped},{"blocks",accepted},{"contexts",contexts},{"storeBlocks",store->blocks.size()},{"snapshotReopened",true},{"durableReactor",true},{"signedArchives",signed_wires.size()},{"signedVotes",durable_votes.size()},{"signedSlashings",durable_proofs.size()},{"deferredVotes",deferred_votes.size()},{"localAttestations",local_signatures},{"localProposals",local_proposals},{"localRandao",local_randao},{"assembledBodies",assembled_bodies},{"storeSigkillReopens",2},{"storeVotes",votes},{"voteReorgs",vote_reorgs},{"withdrawals",paid},{"rejected",rejected},{"cancellations",cancelled},{"networkActivated",false}}.dump()<<'\n';
}
#include "support/capella_exit_owner.hpp"
int main(int argc,char** argv){try{executable=argv[0];
 if(argc==5&&std::string(argv[1])=="--verify-exit-owner"){verify_external_exit_owner(argv[2],argv[3],argv[4]);return 0;}
 if(argc>=4&&(std::getenv("NANO_TEST_BLS_BOOTSTRAP")||std::getenv("NANO_TEST_EXIT_BOOTSTRAP"))){
  const auto* token=std::getenv("NANO_NETWORK_TOKEN");const auto* daemon=std::getenv("NANO_TEST_BLS_DAEMON");check(token&&daemon,"BLS test shim missing supervisor context");PersistentRpc client(std::stoi(argv[1]));const auto status=client.call("nano_capellaStatus",Json::array({token}));Json envelope;for(const auto* key:{"chainId","genesisHash","forkDigest","consensusIdentity"})envelope[key]=status.at(key);const auto* exit=std::getenv("NANO_TEST_EXIT_BOOTSTRAP");envelope["kind"]=exit?"voluntary_exit":"bls_to_execution_change";envelope["ssz"]=exit?exit:std::getenv("NANO_TEST_BLS_BOOTSTRAP");const auto session=status.at("admissionSession");client.call("nano_capellaSubmit",Json::array({token,session,"0x1",envelope.dump()}));Json receipt;const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);do{receipt=client.call("nano_capellaJob",Json::array({token,session,"0x1"}));if(receipt.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);check(receipt.value("decision",std::string())=="accepted","BLS test shim admission failed");client.call("nano_capellaForget",Json::array({token,session,"0x1"}));
  std::string path=daemon;unsetenv("NANO_TEST_EXIT_BOOTSTRAP");unsetenv("NANO_TEST_BLS_BOOTSTRAP");unsetenv("NANO_TEST_BLS_DAEMON");std::vector<char*> args;args.push_back(path.data());for(int i=1;i<argc;++i)args.push_back(argv[i]);args.push_back(nullptr);execv(path.c_str(),args.data());throw std::runtime_error("BLS test shim could not exec daemon");
 }
 if(argc==4&&std::getenv("NANO_TEST_MANAGED_CHILD")){
  check(!std::getenv("NANO_TEST_MANAGED_CAPELLA_KEY")&&!std::getenv("NANO_PRIVATE_KEY"),"supervisor leaked private keys");
  const auto* token=std::getenv("NANO_NETWORK_TOKEN");check(token&&std::string(token)!="must-be-replaced"&&std::getenv("NANO_SUPERVISOR_PID"),"supervisor did not replace IPC credentials");
  PersistentRpc client(std::stoi(argv[1]));const auto status=client.call("nano_capellaStatus",Json::array({token}));check(status.at("executionBound")==true&&status.at("networkAdmissionEnabled")==false,"managed Capella runtime status invalid");
  const auto network=client.call("nano_networkStatus");check(network.at("capellaIpc")==true&&network.at("bellatrixIpc")==false&&network.at("altairIpc")==false,"managed IPC profile ambiguous");
  std::cout<<Json{{"event","capella-managed-child-ready"},{"executionBound",true},{"secretsExcluded",true}}.dump()<<std::endl;
  if(std::getenv("NANO_TEST_MANAGED_CHILD_EXIT"))return 7;for(;;)pause();
 }
 if(argc==3&&std::string(argv[1])=="--export-exit-network"){exit_network=true;network_export=fs::absolute(argv[2]);network_cli=true;managed_cli=true;cli_binary="export-only";configured_runtime=true;scenario(false);return 0;}
 if(argc==3&&std::string(argv[1])=="--export-network"){network_export=fs::absolute(argv[2]);network_cli=true;managed_cli=true;cli_binary="export-only";configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--sync-rotation-outage-cli"){distributed_sync_rotation=true;sync_distributed_cli=true;transport_outage_cli=true;distributed_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--sync-transport-outage-cli"){sync_distributed_cli=true;transport_outage_cli=true;distributed_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--transport-outage-cli"){transport_outage_cli=true;distributed_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--sync-rotation-distributed-cli"){distributed_sync_rotation=true;sync_distributed_cli=true;distributed_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--sync-distributed-cli"){sync_distributed_cli=true;distributed_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--distributed-cli"){distributed_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--multipeer-cli"){multipeer_cli=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--exit-network-cli"){exit_network=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--bls-network-cli"){queued_bls_changes=true;local_bls_changes=true;network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--network-cli"){network_cli=true;managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==4&&std::string(argv[1])=="--daemon-cli"){managed_cli=true;cli_binary=argv[2];daemon_binary=fs::absolute(argv[3]).string();configured_runtime=true;scenario(false);return 0;}
 if(argc==3&&std::string(argv[1])=="--managed-cli"){managed_cli=true;cli_binary=argv[2];configured_runtime=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--exit-reindex"){exit_reindex=true;deposit_reorg_activation=true;deposit_reorg=true;pruning_native=true;scenario(false);return 0;}
#ifdef NANO_SYNC_FAULT
if(argc==3&&std::string(argv[1])=="--exit-lifecycle-writer")return exit_lifecycle_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--exit-lifecycle-sync"){exit_lifecycle_sync=true;local_exits=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
#endif
if(argc==2&&std::string(argv[1])=="--exit-pending-pruning"){exit_pending_pruning=true;local_exits=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--exit-retirement"){exit_retirement=true;local_exits=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==3&&std::string(argv[1])=="--cli"){cli_binary=argv[2];configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--http"){configured_runtime=true;capella_http=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--catalog-concurrency"){wire_mode=true;catalog_concurrency=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-reorg"){queued_bls_reorg=true;queued_bls_changes=true;local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-automatic"){pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--slashing-retirement"){slashing_retirement=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-slashing"){pruning_slashing=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--local-sync"){local_sync=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-invalid-signature"){deposit_invalid_signature=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--automatic-registration"){automatic_registration=true;configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-auto-registration"){automatic_registration=true;deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-activation-restart"){deposit_activation_restart=true;deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-activation"){deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-consumption"){deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-native"){pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-bls"){pruning_bls=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-primitive"){pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-owner"){queued_bls_changes=true;local_bls_changes=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-rpc"){queued_bls_changes=true;local_bls_changes=true;configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-wire"){wire_mode=true;queued_bls_changes=true;local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-changes"){queued_bls_changes=true;local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--exit-wire"){exit_wire=true;queued_exits=true;local_exits=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-exits"){queued_exits=true;local_exits=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--local-exits"){local_exits=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--local-bls-changes"){local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--wire"){wire_mode=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--beacon-reads"){beacon_reads=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--automatic-recovery"){automatic_recovery=true;configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--configured-runtime"){configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--scheduler-sync-rotation"){scheduled=true;sync_rotation=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-reorg-activation"){deposit_reorg_activation=true;deposit_reorg=true;pruning_native=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-reorg"){deposit_reorg=true;pruning_native=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--scheduler-registration"){automatic_registration=true;scheduled=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--scheduler"){scheduled=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--owner-reorg"){owner_reorg=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--owner-finality"){owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&(std::string(argv[1])=="--local-finality"||std::string(argv[1])=="--local-finality-half")){local_finality=std::string(argv[1])=="--local-finality"?100:50;scenario(false);return 0;}
#ifdef NANO_SYNC_FAULT
if(argc==3&&std::string(argv[1])=="--deposit-vote-sync-writer")return deposit_vote_sync_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--deposit-reorg-sync"){deposit_reorg_sync=true;deposit_reorg=true;pruning_native=true;scenario(false);return 0;}
if(argc==3&&std::string(argv[1])=="--pruning-sync-writer")return pruning_sync_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--pruning-sync"){pruning_sync=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==3&&std::string(argv[1])=="--exit-sync-writer")return bls_sync_writer(argv[2],true);
if(argc==2&&std::string(argv[1])=="--queued-exit-sync"){exit_sync=true;local_exits=true;scenario(false);return 0;}
if(argc==3&&std::string(argv[1])=="--bls-sync-writer")return bls_sync_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--queued-bls-sync"){queued_bls_sync=true;local_bls_changes=true;scenario(false);return 0;}
if(argc==3&&std::string(argv[1])=="--sync-writer")return sync_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--owner-witness"){owner_witness=true;owner_sync=true;sync_scenario=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--owner-sync"){owner_sync=true;sync_scenario=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--reactor-sync"){sync_scenario=true;scenario(false);return 0;}
#endif
if(argc==3&&std::string(argv[1])=="--crash-writer")return crash_writer(argv[2]);if(argc==2&&std::string(argv[1])=="--finality-half"){scenario(false,0,true,50);return 0;}if(argc==2&&std::string(argv[1])=="--finality"){scenario(false,0,true);return 0;}scenario(false);scenario(true);scenario(true,3);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
