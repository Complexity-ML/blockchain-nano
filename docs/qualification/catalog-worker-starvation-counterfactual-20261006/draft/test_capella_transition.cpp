#include "nano/consensus_capella_audit.hpp"
#include "nano/consensus_bls.hpp"
#include "support/altair_transition_rules.hpp"
#include "nano/consensus_capella_transition.hpp"
#include "nano/consensus_capella_bls_change.hpp"
#include "nano/consensus_capella_fork_state.hpp"
#include "nano/consensus_capella_fork_store.hpp"
#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/storage.hpp"
#include <rocksdb/db.h>
#include "support/rocksdb_fixture.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_duty.hpp"
#include "nano/rpc.hpp"
#include "nano/http.hpp"
#include "nano/simulation_queue.hpp"
#include "support/persistent_rpc.hpp"
#include "nano/consensus_capella_validator.hpp"
#include "nano/consensus_capella_health.hpp"
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
static bool slashing_reorg=false;
static bool slashing_multi=false;
static bool slashing_sync=false;
static bool slashing_new_validator=false;
static bool slashing_unknown_finality=false;
static unsigned local_finality=0;
static bool owner_mode=false;
static bool owner_reorg=false;
static bool owner_sync=false;
static bool owner_witness=false;
static bool scheduled=false,sync_rotation=false,clock_diagnostics=false,duty_timing=false,signer_health=false;
static bool configured_runtime=false;
static bool slot_continuation=false,prepared_block=false,capacity_pressure=false,atomic_proposal=false,proposal_gap=false,incoming_alternation=false,vote_target_alternation=false,incoming_parent_interference=false;
static bool automatic_recovery=false;
static bool capella_http=false,capella_http_queue=false;
static std::string cli_binary;static Json cli_genesis;static bool managed_cli=false;static std::string daemon_binary;static bool network_cli=false;static bool multipeer_cli=false;static bool distributed_cli=false;static bool sync_distributed_cli=false,distributed_sync_rotation=false;static bool transport_outage_cli=false;static fs::path network_export;
static bool beacon_reads=false;
static std::string pruning_migration_helper;
static bool deposit_network=false;static bool historical_pruning_restore=false;static bool historical_state_pruning=false;static bool historical_export=false;static bool exit_lifecycle_sync=false;static bool exit_pending_pruning=false;static bool exit_reindex=false;static bool exit_retirement=false;static bool exit_sync=false;static bool exit_network=false;static bool exit_wire=false;static bool queued_exits=false;static bool local_exits=false;static bool deposit_reorg_sync=false;static bool deposit_reorg_activation=false;static bool deposit_reorg=false;static bool automatic_registration=false;static bool deposit_activation_restart=false;static bool local_sync=false;static bool deposit_invalid_signature=false;static bool deposit_activation=false;static bool deposit_consumption=false;static bool slashing_retirement=false;static bool pruning_slashing=false;static bool pruning_sync=false;static bool pruning_automatic=false;static bool pruning_native=false;static bool pruning_bls=false;static bool pruning_primitive=false;static bool queued_bls_sync=false;static bool queued_bls_reorg=false;static bool queued_bls_changes=false;static bool local_bls_changes=false;static bool wire_mode=false;static bool catalog_concurrency=false;
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
#include "support/capella_slashing_sync.hpp"
#include "support/capella_pruning_sync.hpp"
#include "support/capella_deposit_reorg.hpp"
static void local_finality_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 if(local_sync){CapellaSyncMessage golden{};golden.slot=0x0807060504030201ULL;golden.validator_index=0x1817161514131211ULL;golden.beacon_block_root.fill(0xaa);golden.signature.fill(0xbb);const auto expected=unhex("0102030405060708"+std::string(64,'a')+"1112131415161718"+std::string(192,'b'));check(encode_capella_sync_message(golden)==expected,"sync SSZ field ordering/endian mismatch");const auto decoded=decode_capella_sync_message(expected);check(decoded&&decoded->slot==golden.slot&&decoded->validator_index==golden.validator_index,"sync SSZ golden decode mismatch");}
 Temp directory;auto limits=crash_limits;if(exit_lifecycle_sync||slashing_sync||historical_pruning_restore)limits.blocks=128;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;const auto genesis_root=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
 Genesis owner_genesis;owner_genesis.chain_id=t.chain_id;owner_genesis.consensus="beacon";owner_genesis.evm_revision=EvmRevision::Shanghai;owner_genesis.block=genesis;owner_genesis.state=genesis_state;
 std::array<fs::path,2> node_locations{directory/"node-0",directory/"node-1"};fs::path key_location=directory.path;
 auto hashes=[](const fs::path& path){std::map<std::string,std::string> out;for(const auto& e:fs::recursive_directory_iterator(path))if(e.is_regular_file()){const auto data=read(e.path());out.emplace(fs::relative(e.path(),path).string(),hex(sha256(Bytes(data.begin(),data.end()))));}return out;};
 std::map<std::string,std::string> source_node_hashes,backup_hashes;std::map<uint64_t,ForkCheckpointToken> restore_signing_pins;std::map<uint64_t,std::map<std::string,std::string>> source_signer_hashes;
 uint64_t restored_from_epoch=0;bool physically_restored=false;
 std::array<std::unique_ptr<ChainStore>,2> lifecycle_witnesses;
 std::array<std::unique_ptr<ChainStore>,2> disks;std::array<std::unique_ptr<Blockchain>,2> owners;std::array<std::unique_ptr<CapellaDurableReactor>,2> nodes;
 for(size_t i=0;i<2;++i){if(owner_mode){owners[i]=std::make_unique<Blockchain>(node_locations[i],owner_genesis);bool blocked=false;try{owners[i]->tip();}catch(const std::runtime_error&){blocked=true;}check(blocked,"unattached Capella owner readable");if(exit_lifecycle_sync||slashing_sync||historical_pruning_restore)lifecycle_witnesses[i]=std::make_unique<ChainStore>(directory/("exit-witness-"+std::to_string(i)));nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,initial,std::function<void()>{},lifecycle_witnesses[i].get());continue;}disks[i]=std::make_unique<ChainStore>(node_locations[i]);disks[i]->bind_chain(t.chain_id,genesis.header.hash(),"capella-local-loop");disks[i]->commit(genesis,genesis_state,{});nodes[i]=std::make_unique<CapellaDurableReactor>(*disks[i],t.chain_id,genesis.header.hash(),rules,limits,initial);}
 if(pruning_automatic)for(auto& node:nodes)node->set_automatic_pruning(true);
 if(historical_state_pruning)for(auto& node:nodes)node->set_historical_state_pruning(true);
 if(queued_bls_changes){SignedCapellaBlsChange change;Key withdrawal_key(1001);change.validator_index=1;copy_to(change.from_bls_pubkey,withdrawal_key.public_key);change.to_execution_address[19]=43;copy_to(change.signature,withdrawal_key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,genesis_root))));for(auto& node:nodes)check(node->bls_change_ssz(encode_capella_bls_change(change)),"owner queue admission rejected");}
 if(pruning_bls){SignedCapellaBlsChange change;Key key(1001);change.validator_index=1;copy_to(change.from_bls_pubkey,key.public_key);change.to_execution_address[19]=43;copy_to(change.signature,key.sign(consensus_signing_root(capella_bls_change_root(change),consensus_domain({10,0,0,0},t.genesis_version,genesis_root))));check(nodes[1]->bls_change_ssz(encode_capella_bls_change(change)),"pruning BLS admission failed");}
 std::map<uint64_t,std::unique_ptr<ChainStore>> key_disks,key_witnesses;std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>> journals;
 auto journal=[&](uint64_t index)->BeaconSigningJournal&{if(!journals.contains(index)){key_disks[index]=std::make_unique<ChainStore>(key_location/("key-"+std::to_string(index)));if(historical_pruning_restore)key_witnesses[index]=std::make_unique<ChainStore>(directory/("key-witness-"+std::to_string(index)));journals[index]=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots,std::nullopt,key_witnesses.contains(index)?key_witnesses.at(index).get():nullptr);}return *journals.at(index);};
 const auto sender_key=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;const auto sender=SignedTransaction::sign(tx,sender_key).sender();
 size_t blocks=0,votes=0,restarts=0,reopened_journals=0;uint64_t previous_finalized=0;const uint64_t end=((exit_lifecycle_sync||slashing_sync)?10:pruning_automatic?9:5)*slots+1;size_t automatic_moves=0,maximum_records=0;auto last_anchor=initial.timed.view.anchor;
 auto equal=[&]{const auto a=nodes[0]->current(),b=nodes[1]->current();check((pruning_bls||exit_pending_pruning||a.token==b.token)&&capella_store_snapshot(*a.state,rules,limits)==capella_store_snapshot(*b.state,rules,limits)&&nodes[0]->signed_votes()==nodes[1]->signed_votes(),"local loop nodes diverged");const auto& head=capella_fork_head(*a.state,rules);for(size_t i=0;i<2;++i){const auto& disk=owner_mode?owners[i]->store():*disks[i];check(disk.latest()->header.hash()==head.execution_block.header.hash()&&disk.state_at(head.execution_block.header.number)->root()==head.execution_state.root(),"local loop disk/state diverged");
   if(owner_mode){check(owners[i]->tip().header.hash()==head.execution_block.header.hash()&&owners[i]->state().root()==head.execution_state.root()&&owners[i]->pending_count()==0,"Capella owner cache/pool diverged");const auto finalized=a.state->blocks.at(a.state->timed.view.finalized.root).execution_block;check(owners[i]->finalized_block()->header.hash()==finalized.header.hash(),"Capella finalized cache mismatch");Rpc rpc(*owners[i]);auto call=[&](const char* method,const Json& params){const auto response=rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});check(!response.contains("error"),"Capella owner RPC error");return response.at("result");};check(call("eth_getBlockByNumber",Json::array({"latest",false})).at("hash")==hex(head.execution_block.header.hash()),"latest RPC mismatch");check(call("eth_getBlockByNumber",Json::array({"finalized",false})).at("hash")==hex(finalized.header.hash()),"finalized RPC mismatch");check(call("eth_getTransactionCount",Json::array({sender,"latest"}))==quantity(head.execution_state.get(sender).nonce),"nonce RPC mismatch");}
  }};
 auto add_pruning_proof=[&]{const auto current=nodes[0]->current();const auto& justified=current.state->blocks.at(current.state->timed.view.justified.root);const auto context=capella_fork_block_context(justified.state,justified.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);const auto old_root=initial.timed.view.anchor;IndexedConsensusAttestation first{};first.indices={0};first.data.slot=initial.blocks.at(old_root).header.slot;first.data.beacon_block_root=old_root;first.data.source={0,old_root};first.data.target={0,old_root};auto second=first;second.data.beacon_block_root[0]^=1;const auto version=first.data.target.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;for(auto* proof:{&first,&second})proof->signature=Key(0).sign(consensus_signing_root(consensus_attestation_root(proof->data),consensus_domain({1,0,0,0},version,context.genesis_root)));for(auto& node:nodes)check(node->slashing(first,second),"old double-sign proof rejected before pruning");return std::pair{first,second};};
 std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>> retirement_proof;bool saw_pending_proof=false,saw_retired_proof=false;size_t skipped_slashed_proposers=0;uint64_t retirement_epoch=0;
 if(slashing_retirement)retirement_proof=add_pruning_proof();
#include "support/capella_slashing_multi.hpp"
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
 std::set<unsigned> slashing_fault_stages;
 auto qualify_slashing_transition=[&](const Bytes& operation,bool is_tick,uint64_t time){
  if(!slashing_sync||slashing_fault_stages.size()==2)return;
  const auto before=nodes[1]->current();const auto next=is_tick?std::optional<CapellaForkStore>(prepare_capella_store_tick(*before.state,time,rules,limits.replay_slots,limits.sync_trials)):prepare_capella_received_block(*before.state,operation,rules,limits.replay_slots,limits.sync_trials);check(bool(next),"slashing fault candidate missing");
  auto registry=[&](const CapellaForkStore& store){const auto& b=store.blocks.at(store.timed.view.finalized.root);return capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state).registry;};
  const auto old_registry=registry(*before.state),new_registry=registry(*next);
  for(unsigned offender=0;offender<2;++offender)if(!slashing_fault_stages.contains(offender)&&!old_registry.validators.at(offender).slashed&&new_registry.validators.at(offender).slashed){
   nodes[1].reset();owners[1].reset();lifecycle_witnesses[1].reset();
   qualify_slashing_sync(directory/"node-1",directory/"exit-witness-1",rules,limits,owner_genesis,operation,offender,is_tick,time);
   lifecycle_witnesses[1]=std::make_unique<ChainStore>(directory/"exit-witness-1");owners[1]=std::make_unique<Blockchain>(directory/"node-1",owner_genesis);nodes[1]=std::make_unique<CapellaDurableReactor>(*owners[1],rules,limits,*lifecycle_witnesses[1]);check(nodes[1]->current().token==before.token,"slashing fault changed original owner");slashing_fault_stages.insert(offender);
  }
 };
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
  qualify_slashing_transition({},true,now);
  qualify_lifecycle_transition({},true,now);
#endif
  for(auto& node:nodes)node->tick(now);
  if((exit_retirement||exit_pending_pruning||exit_lifecycle_sync)&&slot==slots+1){const auto current=nodes[0]->current();const auto& head=capella_fork_head(*current.state,rules);exit_origin=consensus_beacon_header_root(head.header);
   const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
   SignedConsensusVoluntaryExit operation{slot/slots,0,{}};const auto version=operation.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
   operation.signature=Key(0).sign(consensus_signing_root(consensus_voluntary_exit_root(operation.epoch,0),consensus_domain({4,0,0,0},version,context.genesis_root)));retirement_exit=encode_consensus_voluntary_exit_ssz(operation);
   if(exit_pending_pruning)check(nodes[1]->voluntary_exit_ssz(retirement_exit),"pending exit admission rejected");else for(auto& node:nodes)check(node->voluntary_exit_ssz(retirement_exit),"retirement exit admission rejected");verify_exit_archive();
  }
  const auto current=nodes[0]->current();const auto& head=capella_fork_head(*current.state,rules);const auto proposer=prepare_capella_proposer_identity(head.state,head.header,slot,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,t.consensus.attestation.duties,limits.replay_slots,limits.sync_trials);if(slashing_retirement&&(!proposer||(slashing_multi&&current.state->timed.view.equivocating_indices.contains(proposer->index)))){++skipped_slashed_proposers;continue;}check(bool(proposer),"local loop proposer missing");
  auto provider=[&](const ConsensusRoot& root){return Key(proposer->index).sign(root);};auto& proposer_journal=journal(proposer->index);
  const auto reveal=nodes[0]->sign_randao(proposer->index,slot,[&]{return now;},current.token,proposer_journal,provider);check(bool(reveal),"local loop RANDAO missing");
  if(local_sync&&slot==2){
   const auto members=nodes[0]->current_sync_members(slot);check(!members.empty(),"missing skipped-slot sync members");const auto member=*members.begin();size_t calls=0;uint64_t wall=now;auto clock=[&]{return wall;};auto signer=[&](const ConsensusRoot& root){++calls;return Key(member).sign(root);};
   // Regressions before and inside the provider must leave both durable
   // commitments and the ephemeral sync catalog untouched.
   for(const std::string mode:{"entry","before-provider","after-provider"}){
    const auto token=nodes[0]->current().token,reservation=journal(member).commitment();unsigned reads=0,providers=0;
    const auto baseline=now+rules.slot_duration_ms/3+uint64_t(rules.slot_duration_ms%3!=0);uint64_t observed=baseline;
    auto fault_clock=[&]{++reads;if(mode=="entry")return now-1;if(mode=="before-provider"&&reads>1)return baseline-1;return observed;};
    auto fault_provider=[&](const ConsensusRoot& root){++providers;observed=baseline-1;return Key(member).sign(root);};
    std::string failure;try{nodes[0]->sync_current(member,slot,fault_clock,journal(member),fault_provider);}catch(const std::runtime_error& e){failure=e.what();}
    const auto expected_now=mode=="entry"?now-1:baseline-1,expected_last=mode=="entry"?now:baseline;
    check(failure.starts_with("Sync duty clock regressed:")&&failure.find("nowMs="+std::to_string(expected_now)+",")!=std::string::npos&&failure.find("lastMs="+std::to_string(expected_last)+",")!=std::string::npos&&failure.find("durableMs="+std::to_string(now)+",")!=std::string::npos&&failure.find("phase="+mode+",")!=std::string::npos&&failure.find("beforeLast=1")!=std::string::npos,"sync clock diagnostic lost values or phase");
    check(providers==(mode=="after-provider"?1u:0u)&&nodes[0]->current().token==token&&journal(member).commitment()==reservation,"sync clock regression reached provider early or changed durable commitments");
    check(nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"regressed sync duty published a message");
    std::cout<<Json{{"scenario","sync-signing-clock-regression"},{"phase",mode},{"providerCalls",providers},{"failure",failure},{"durableCommitmentsUnchanged",true},{"messagePublished",false}}.dump()<<std::endl;
   }
   check(!nodes[0]->sync_current(member,slot,clock,journal(member),signer)&&calls==0,"sync signed skipped head before one-third slot");
   wall+=rules.slot_duration_ms/3+uint64_t(rules.slot_duration_ms%3!=0);check(bool(nodes[0]->sync_current(member,slot,clock,journal(member),signer))&&calls==1,"sync did not sign skipped head at one-third slot");
  }

  tx.nonce=head.execution_state.get(sender).nonce;const std::vector<SignedTransaction> pending{SignedTransaction::sign(tx,sender_key)};
  if(owner_mode)for(auto& owner:owners)check(owner->submit(pending.front())&&owner->pending_count()==1,"Capella pool submission failed");
  Bytes expected_sync;std::vector<ConsensusRoot> exported_sync;std::optional<CapellaSyncMessage> cached_old_parent_message;std::optional<uint64_t> cached_before_parent_change;
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
    const auto warm_context=nodes[0]->sync_admission_stats();
    check(nodes[0]->submit_sync_aggregate(slot,partials[first_part],current.token),"duplicate partial sync not idempotent");
    const auto reused_context=nodes[0]->sync_admission_stats();
    check(reused_context.at("contextBuilds")==warm_context.at("contextBuilds")&&reused_context.at("contextHits").get<uint64_t>()==warm_context.at("contextHits").get<uint64_t>()+1,"same parent/slot rebuilt sync state");
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
     check(nodes[0]->sync_admission_stats().at("contextBuilds")==warm_context.at("contextBuilds"),"individual sync messages rebuilt warmed aggregate context");
     auto forged_cached=message;forged_cached.signature[95]^=1;
     check(!nodes[0]->submit_sync_message(forged_cached,current.token)&&nodes[0]->sync_admission_stats().at("contextBuilds")==warm_context.at("contextBuilds"),"cache bypassed individual signature verification");
     check(empty_catalog->page({},64,16384)["entries"].empty(),"pinned sync catalog changed");
     const auto catalog=nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384);
     check(catalog["entries"].size()==individual_signatures.size(),"sync replay duplicated or lost exported members");
     bool bounded=false;try{nodes[0]->capture_inventory("sync_committee_message",0);}catch(const std::length_error&){bounded=true;}check(bounded,"sync root budget ignored");
     bool legacy=false;try{nodes[0]->inventory("sync_committee_message",{},64,16384);}catch(const std::invalid_argument&){legacy=true;}check(legacy,"ephemeral inventory accepted durable cursor API");
     for(const auto& entry:catalog["entries"]){ConsensusRoot id{};copy_to(id,unhex(entry.get<std::string>()));exported_sync.push_back(id);const auto out=nodes[0]->export_message("sync_committee_message",id,144);check(out["available"]==true,"sync export missing");const auto bytes=unhex(out["envelope"]["ssz"].get<std::string>());check(hex(sha256(bytes))==entry.get<std::string>(),"sync export hash differs");check(nodes[1]->sync_message_ssz(bytes),"second reactor rejected exported sync");bool small=false;try{nodes[0]->export_message("sync_committee_message",id,143);}catch(const std::length_error&){small=true;}check(small,"sync export byte bound ignored");}

     if(slot==4){
      // With no block yet at slot 4, both slot-3 and slot-4 signatures can
      // reference the same parent. Collecting slot 4 must not erase slot 3.
      const auto before_next=nodes[0]->sync_admission_stats().at("contextBuilds").get<uint64_t>();
      auto next_message=message;next_message.slot=slot;check(nodes[0]->submit_sync_message(next_message,current.token),"next-slot sync context rejected");
      check(nodes[0]->sync_admission_stats().at("contextBuilds").get<uint64_t>()==before_next+1,"same parent/new slot reused wrong context");
      check(nodes[0]->submit_sync_message(message,current.token)&&nodes[0]->sync_admission_stats().at("contextBuilds").get<uint64_t>()==before_next+2,"single-entry cache failed to restore previous slot context");
      cached_old_parent_message=next_message;
      std::cout<<Json{{"scenario","sync-admission-context-cache"},{"sameParentSlotReused",true},{"individualAndAggregateShared",true},{"forgedSignatureRejected",true},{"twoSlotReplacement",true},{"durableCheckpointUnchanged",nodes[0]->current().token==current.token}}.dump()<<std::endl;
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
  auto body=owner_mode?nodes[0]->prepare_local_body_from_pool(proposer->index,slot,*reveal,head.execution_block.header.coinbase,current.token,16,64):nodes[0]->prepare_local_body(proposer->index,slot,*reveal,head.execution_block.header.coinbase,current.token,pending,16,64);check(bool(body),"local loop body missing");if(local_sync)check(std::equal(expected_sync.begin(),expected_sync.end(),body->begin()+220),"selected sync aggregate differs or stale aggregate leaked");
  if(slashing_multi)filter_slashing_body(*body);
  const auto wire=(queued_bls_changes||local_sync)?nodes[0]->propose_current(proposer->index,slot,head.execution_block.header.coinbase,16,64,[&]{return now;},proposer_journal,provider):nodes[0]->sign_proposal(proposer->index,slot,*body,[&]{return now;},current.token,proposer_journal,provider);check(bool(wire),"local loop signed block missing");
#ifdef NANO_SYNC_FAULT
  qualify_slashing_transition(*wire,false,0);
  qualify_lifecycle_transition(*wire,false,0);
#endif
  if(cached_old_parent_message){check(nodes[0]->submit_sync_message(*cached_old_parent_message,current.token),"cache rewarm for parent change failed");cached_before_parent_change=nodes[0]->sync_admission_stats().at("contextBuilds").get<uint64_t>();}
  for(auto& node:nodes)check(node->block(*wire),"local loop block admission failed");++blocks;
  if(cached_old_parent_message){const auto published=nodes[0]->current();const auto before=nodes[0]->sync_admission_stats();
   check(!nodes[0]->submit_sync_message(*cached_old_parent_message,published.token),"new parent accepted cached old-parent sync root");
   const auto after=nodes[0]->sync_admission_stats();check(after.at("contextBuilds").get<uint64_t>()==*cached_before_parent_change+1&&after.at("root-mismatch").get<uint64_t>()==before.at("root-mismatch").get<uint64_t>()+1&&nodes[0]->current().token==published.token,"same slot/changed parent did not rebuild or changed checkpoint");
   std::cout<<Json{{"scenario","sync-admission-context-parent-change"},{"sameSlotNewParentRebuilt",true},{"oldParentRootRejected",true},{"durableCheckpointUnchanged",true}}.dump()<<std::endl;
  }
  if(local_sync){CapellaSyncMessage obsolete;obsolete.slot=slot-1;const auto stats=nodes[0]->sync_admission_stats();const auto published=nodes[0]->current();check(!nodes[0]->submit_sync_message(obsolete,published.token),"old-parent sync accepted after proposal");check(nodes[0]->sync_admission_stats().at("head-at-or-after-inclusion").get<uint64_t>()==stats.at("head-at-or-after-inclusion").get<uint64_t>()+1&&nodes[0]->current().token==published.token,"obsolete sync changed checkpoint or wrong cause");}
  if(local_sync){check(nodes[0]->capture_inventory("sync_committee_message",4096)->page({},64,16384)["entries"].empty(),"changed head retained sync catalog");for(const auto& id:exported_sync)check(nodes[0]->export_message("sync_committee_message",id,144)["available"]==false,"changed head exported stale sync");}
  const auto admitted=nodes[0]->current();const auto duties=prepare_capella_attestation_duties(*admitted.state,rules,slot,limits.replay_slots,limits.sync_trials);
  for(const auto& [index,duty]:duties){if(slashing_retirement&&(index==0||(slashing_multi&&index==1)))continue;if(local_finality==50&&index%2)continue;
   const auto vote=nodes[0]->sign_attestation(index,slot,[&]{return now;},nodes[0]->current().token,journal(index),[&](const ConsensusRoot& root){return Key(index).sign(root);});check(bool(vote),"local loop attestation missing");
   for(auto& node:nodes)check(node->vote(*vote),"local loop gossip vote rejected");++votes;
  }
  verify_exit_archive();
  if(slashing_multi)verify_multi_slashing();
  if(slashing_retirement&&!slashing_multi){for(auto& node:nodes){const auto state=node->current();const auto& finalized=state.state->blocks.at(state.state->timed.view.finalized.root);const auto context=capella_fork_block_context(finalized.state,finalized.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);const bool retired=context.registry.validators.at(0).slashed;check(node->signed_slashings().size()==size_t(!retired),"slashing proof retired before finality or retained afterward");if(retired){saw_retired_proof=true;retirement_epoch=state.state->timed.view.finalized.epoch;const auto token=state.token;check(node->slashing(retirement_proof->second,retirement_proof->first)&&node->current().token==token&&node->signed_slashings().empty(),"finalized proof replay regrew archive");}else saw_pending_proof=true;}}
  if(pruning_automatic){const auto current=nodes[0]->current();maximum_records=std::max(maximum_records,current.state->blocks.size());if(current.state->timed.view.anchor!=last_anchor){++automatic_moves;last_anchor=current.state->timed.view.anchor;const auto id=hex(Bytes(last_anchor.begin(),last_anchor.end()));for(auto& node:nodes){const auto header=node->beacon_read("/eth/v1/beacon/headers/"+id),full=node->beacon_read("/eth/v2/beacon/blocks/"+id);check(header.status==200&&full.status==200&&header.body.at("finalized")==true&&full.body.at("finalized")==true,"pruned signed anchor unavailable to Beacon API");}}check(nodes[0]->network_identity()==nodes[1]->network_identity(),"automatic pruning changed network identity");}
  const auto finalized=nodes[0]->current().state->timed.view.finalized.epoch;check(finalized>=previous_finalized,"local loop finality regressed");previous_finalized=finalized;
  if(slot%slots==0||slot==end){equal();std::cout<<Json{{"progressSlot",slot},{"finalizedEpoch",finalized},{"signedVotes",votes}}.dump()<<std::endl;}
  if(historical_pruning_restore&&slot==4*slots){
   const auto current=nodes[1]->current();restored_from_epoch=current.state->timed.view.finalized.epoch;
   const auto& stopped_head=capella_fork_head(*current.state,rules);const auto marker=owners[1]->store().state_pruning();check(marker&&marker->before>1,"restore source was not pruned");
   for(const auto& [index,value]:journals)restore_signing_pins[index]=value->commitment();
   nodes[1].reset();owners[1].reset();journals.clear();key_disks.clear();
   source_node_hashes=hashes(node_locations[1]);const auto backup=directory/"pruned-backup";fs::create_directory(backup);fs::copy(node_locations[1],backup/"node",fs::copy_options::recursive);
   for(const auto& [index,pin]:restore_signing_pins){const auto old=directory/("key-"+std::to_string(index));source_signer_hashes[index]=hashes(old);fs::copy(old,backup/("key-"+std::to_string(index)),fs::copy_options::recursive);}
   backup_hashes=hashes(backup);const auto restored=directory/"pruned-restored";fs::copy(backup,restored,fs::copy_options::recursive);check(hashes(restored)==backup_hashes,"pruned node/journal copy differs");
   const auto before_audit=hashes(restored);{ChainStore audit(restored/"node",ChainStore::OpenMode::ReadOnly);const auto result=audit_capella_canonical_history(audit,t.chain_id,genesis.header.hash(),rules,limits,initial,current.token,1000,64*1024*1024);check(result.at("canonicalHistoryVerified")==true&&result.at("prunedExecutionStatesReplayed").get<size_t>()>0,"restored pruned history failed trusted-genesis replay");}
   check(hashes(restored)==before_audit,"restored offline audit changed files");
   fs::path restored_node=restored/"node";
   if(!pruning_migration_helper.empty()){
    // Model the legacy on-disk schema on an already audited owned copy only.
    fs::rename(restored_node,restored/"chain");restored_node=restored/"chain";
    rocksdb::Options options;options.create_if_missing=false;auto legacy=test::open_owned_fixture(options,restored_node.string());rocksdb::WriteOptions write;write.sync=true;check(legacy->Put(write,"meta/schema","nano/1").ok(),"write owned legacy schema");legacy.reset();
    bool refused=false;try{ChainStore disk(restored_node,ChainStore::OpenMode::ReadOnly);}catch(const std::runtime_error& error){refused=std::string(error.what()).find("legacy pruned database requires explicit migration")!=std::string::npos;}check(refused,"new reader accepted legacy pruned fixture before migration");
    std::ofstream marker(restored/".nano-owned-schema-migration-copy");marker<<"owned-offline-schema-migration-v1\n";marker.close();check(bool(marker),"migration marker write failed");
    const auto request=restored/"migration-request.json";save(request,Json{{"expectedCheckpoint",Json{{"sequence",quantity(Int(std::to_string(current.token.sequence)))},{"digest",hex(current.token.digest)}}},{"limits",Json{{"stateBytes",quantity(Int(std::to_string(limits.envelope_bytes)))}}}});
    const std::vector<std::string> args={pruning_migration_helper,"--offline-owned-copy",restored_node.string(),"--audit-request",request.string()};const auto migrated=run(args);check(migrated.code==0,"owned schema migration failed: "+migrated.err);const auto receipt=Json::parse(migrated.out);check(receipt.at("onlySchemaChanged")==true,"migration changed non-schema state");const auto replay=run(args);check(replay.code==0,"migration idempotent replay failed");const auto repeated=Json::parse(replay.out);check(repeated.at("writes")==0&&repeated.at("nonSchemaContentSha256")==receipt.at("nonSchemaContentSha256"),"migration replay wrote or changed content");
    ChainStore audit(restored_node,ChainStore::OpenMode::ReadOnly);const auto result=audit_capella_canonical_history(audit,t.chain_id,genesis.header.hash(),rules,limits,initial,current.token,1000,64*1024*1024);check(result.at("canonicalHistoryVerified")==true&&result.at("prunedExecutionStatesReplayed").get<size_t>()>0,"migrated state failed trusted-genesis audit");
   }
   node_locations[1]=restored_node;key_location=restored;
   owners[1]=std::make_unique<Blockchain>(node_locations[1],owner_genesis);nodes[1]=std::make_unique<CapellaDurableReactor>(*owners[1],rules,limits,*lifecycle_witnesses[1]);check(nodes[1]->current().token==current.token,"independent consensus witness differs after restore");nodes[1]->set_automatic_pruning(true);nodes[1]->set_historical_state_pruning(true);
   for(const auto& [index,pin]:restore_signing_pins){key_disks[index]=std::make_unique<ChainStore>(key_location/("key-"+std::to_string(index)));journals[index]=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots,pin,key_witnesses.at(index).get());check(journals[index]->commitment()==pin,"restored signer commitment differs");}
   ConsensusBytes4 restore_version{};std::copy_n(stopped_head.state.ssz.begin()+52,4,restore_version.begin());auto conflicting=stopped_head.header;conflicting.body_root[0]^=1;bool signed_again=false,refused=false;try{journals.at(conflicting.proposer_index)->sign_proposal(conflicting,restore_version,[&](const ConsensusRoot& root){signed_again=true;return Key(conflicting.proposer_index).sign(root);});}catch(const std::exception&){refused=true;}check(refused&&!signed_again,"restored journal allowed conflicting proposal signer callback");
   physically_restored=true;equal();
  }
  if(slot==3*slots||(exit_retirement&&slot==slots+1)){
   for(size_t i=0;i<2;++i){const auto saved=nodes[i]->current().token;nodes[i].reset();if(owner_mode){bool blocked=false;try{owners[i]->tip();}catch(const std::runtime_error&){blocked=true;}check(blocked,"detached Capella owner remained readable");owners[i].reset();owners[i]=std::make_unique<Blockchain>(node_locations[i],owner_genesis);if(exit_lifecycle_sync||slashing_sync||historical_pruning_restore)nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,*lifecycle_witnesses[i]);else nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,saved);}else{disks[i].reset();disks[i]=std::make_unique<ChainStore>(node_locations[i]);nodes[i]=std::make_unique<CapellaDurableReactor>(*disks[i],t.chain_id,genesis.header.hash(),rules,limits,saved);}if(pruning_automatic)nodes[i]->set_automatic_pruning(true);if(historical_state_pruning)nodes[i]->set_historical_state_pruning(true);++restarts;}
   reopened_journals=journals.size();for(auto& [index,value]:journals){const auto saved=value->commitment();value.reset();key_disks[index].reset();key_disks[index]=std::make_unique<ChainStore>(key_location/("key-"+std::to_string(index)));value=std::make_unique<BeaconSigningJournal>(*key_disks[index],t.chain_id,genesis.header.hash(),genesis_root,Key(index).public_key,slots,saved,key_witnesses.contains(index)?key_witnesses.at(index).get():nullptr);}
   equal();verify_exit_archive();
  }
 }
 CapellaSignedVotes pruning_proofs;
 if(pruning_slashing){add_pruning_proof();pruning_proofs=nodes[0]->signed_slashings();check(pruning_proofs.size()==1&&nodes[1]->signed_slashings()==pruning_proofs,"pruning slashing archive differs");}
 if(historical_pruning_restore){check(physically_restored&&nodes[0]->current().state->timed.view.finalized.epoch>restored_from_epoch,"restored pruned consensus did not advance finality");equal();size_t advanced=0;for(const auto& [index,pin]:restore_signing_pins){const auto current=journals.at(index)->commitment();check(current.sequence>=pin.sequence,"restored signing journal regressed");if(current.sequence>pin.sequence)++advanced;check(hashes(directory/("key-"+std::to_string(index)))==source_signer_hashes.at(index),"restored duties changed signer backup source");}check(advanced>0&&hashes(directory/"node-1")==source_node_hashes&&hashes(directory/"pruned-backup")==backup_hashes,"restored duties failed to advance journals or mutated backups");std::cout<<Json{{"scenario","capella-pruned-consensus-physical-restore"},{"beforeFinalizedEpoch",restored_from_epoch},{"afterFinalizedEpoch",nodes[0]->current().state->timed.view.finalized.epoch},{"signingPins",restore_signing_pins.size()},{"journalsAdvanced",advanced},{"independentWitnessesRetained",true},{"conflictingProposalRejectedBeforeSigner",true},{"sourceBackupUnchanged",true},{"networkTransport",false},{"physicalPowerLoss",false}}.dump()<<std::endl;}
 if(!pruning_migration_helper.empty()){size_t advanced=0;for(const auto& [index,pin]:restore_signing_pins)if(journals.at(index)->commitment().sequence>pin.sequence)++advanced;check(advanced==restore_signing_pins.size(),"not all journals resumed after migration");std::cout<<Json{{"scenario","capella-pruned-migration-duties"},{"beforeFinalizedEpoch",restored_from_epoch},{"afterFinalizedEpoch",nodes[0]->current().state->timed.view.finalized.epoch},{"journalsAdvanced",advanced},{"signingPins",restore_signing_pins.size()},{"independentWitnessesRetained",true},{"schemaMigrationHelperUsed",true},{"conflictingProposalRejectedBeforeSigner",true},{"networkTransport",false},{"physicalPowerLoss",false}}.dump()<<std::endl;}
 const auto result=nodes[0]->current();const auto& head=capella_fork_head(*result.state,rules);check(head.execution_state.get(sender).nonce==Int(std::to_string(blocks+1)),"local loop execution transactions missing");
 if(local_finality==100)check(result.state->timed.view.finalized.epoch>=2&&result.state->timed.view.justified.epoch>=3,"assembled local chain did not finalize");else check(result.state->timed.view.finalized.epoch==0,"half-participation local chain finalized");
#ifdef NANO_SYNC_FAULT
 if(slashing_sync)check(slashing_fault_stages.size()==2,"slashing fault boundaries not covered");
 if(pruning_sync)qualify_pruning_sync(*result.state,nodes[0]->signed_votes(),rules,owner_genesis);
#endif
 if(historical_state_pruning){
  for(size_t i=0;i<2;++i){
   auto& disk=const_cast<ChainStore&>(owners[i]->store());const auto marker=disk.state_pruning();check(marker&&marker->before>1,"historical pruning never removed state");
   const auto now=nodes[i]->current();const auto& live=now.state->blocks.at(now.state->timed.view.anchor).execution_block.header;
   check(marker->anchor.number<=live.number&&marker->before<=marker->anchor.number,"pruning passed authenticated live anchor");
   check(disk.state_at(0).has_value()&&!disk.state_at(marker->before-1).has_value()&&disk.state_at(marker->anchor.number).has_value()&&disk.state_at(head.execution_block.header.number).has_value(),"genesis/historical/live state retention differs");
   Rpc historical_rpc(*owners[i]);
   const auto absent=historical_rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_getBalance"},{"params",Json::array({sender,quantity(marker->before-1)})}});
   check(absent.contains("error")&&absent.at("error").at("code")==-32000&&absent.at("error").at("message")=="State unavailable","historical RPC fell back to latest or hid pruned state");
   const auto available=historical_rpc.handle(Json{{"jsonrpc","2.0"},{"id",2},{"method","eth_getBalance"},{"params",Json::array({sender,"latest"})}});
   check(available.contains("result")&&available.at("result")==quantity(capella_fork_head(*now.state,rules).execution_state.get(sender).balance),"latest balance unavailable/changed after pruning");
   ChainStore audit_disk(node_locations[i],ChainStore::OpenMode::ReadOnly);
   const auto replay=audit_capella_canonical_history(audit_disk,t.chain_id,genesis.header.hash(),rules,limits,initial,now.token,1000,64*1024*1024);
   check(replay.at("canonicalHistoryVerified")==true&&replay.at("prunedExecutionStatesReplayed").get<size_t>()>0&&replay.at("allHistoricalStatesMaterialized")==false,"pruned history did not replay from trusted genesis");
   const auto tip=disk.latest();const auto saved=disk.fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,now.token);check(tip&&saved,"pruning test checkpoint absent");
   auto reject_anchor=[&](ExecutionStateRetentionAnchor wrong){bool refused=false;try{disk.commit_execution_checkpoint(t.chain_id,genesis.header.hash(),now.token,tip->header.hash(),tip->header.number,tip->header.hash(),{},saved->snapshot,limits.envelope_bytes,limits.blocks,{},wrong);}catch(const std::invalid_argument&){refused=true;}check(refused&&disk.fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,now.token).has_value(),"bad pruning anchor changed commitment");check(disk.state_pruning()->before==marker->before&&disk.state_at(marker->anchor.number).has_value(),"rejected retention mutated state boundary");};
   auto wrong=marker->anchor;wrong.hash[0]^=1;reject_anchor(wrong);
   const auto zero=disk.block_by_number(0);check(zero.has_value(),"retention genesis block missing");reject_anchor({0,zero->header.hash()});
   reject_anchor({tip->header.number+1,Bytes(32,0)});
   const bool refused=true;
   std::cout<<Json{{"scenario","capella-historical-state-pruning"},{"before",quantity(marker->before)},{"anchor",quantity(marker->anchor.number)},{"replayed",replay.at("prunedExecutionStatesReplayed")},{"invalidAnchorRefused",refused}}.dump()<<std::endl;
  }
 }
 if(pruning_automatic){check(automatic_moves>=3&&maximum_records<=5*slots&&result.state->blocks.size()<5*slots,"automatic pruning did not bound retained block states");for(size_t i=0;i<2;++i){const auto token=nodes[i]->current().token;nodes[i].reset();owners[i].reset();owners[i]=std::make_unique<Blockchain>(node_locations[i],owner_genesis);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,token);nodes[i]->set_automatic_pruning(true);}equal();std::cout<<Json{{"scenario","capella-automatic-pruning"},{"anchorMoves",automatic_moves},{"maximumRetainedRecords",maximum_records},{"finalRetainedRecords",result.state->blocks.size()},{"depositAncestorPins",result.state->deposit_ancestors.size()},{"nativeProfile",pruning_native},{"finalizedEpoch",result.state->timed.view.finalized.epoch},{"reopenedBothOwners",true}}.dump()<<std::endl;}
 if(slashing_multi){verify_multi_slashing();check(multi_partial&&multi_complete,"multi-offender retirement lifecycle incomplete");std::cout<<Json{{"scenario","capella-slashing-multi"},{"partialFinalityRetained",true},{"completeFinalityRetired",true},{"reopenedBothOwners",true},{"aggregateProofVerified",true},{"networkTransport",false}}.dump()<<std::endl;}
 if(slashing_retirement&&!slashing_multi){check(saw_pending_proof&&saw_retired_proof&&nodes[0]->signed_slashings().empty()&&nodes[1]->signed_slashings().empty(),"slashing archive lifecycle incomplete");std::cout<<Json{{"scenario","capella-slashing-retirement"},{"retainedUntilFinalized",true},{"retiredAfterFinality",true},{"replayDidNotRegrowArchive",true},{"finalizedEpoch",retirement_epoch},{"skippedSlashedProposers",skipped_slashed_proposers},{"reopenedWithoutProof",true}}.dump()<<std::endl;}
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
  if(historical_export){
   const auto saved=nodes[1]->current().token;size_t exported_count=0;auto cursor=old.blocks.at(anchor).header.parent_root;
   std::vector<CheckpointArchiveEntry> archived;std::vector<ExecutionCheckpointBlock> suffix;
   auto chain=consensus_beacon_header_root(head.header);
   for(;;){const auto& b=old.blocks.at(chain);suffix.push_back({b.execution_block,b.execution_state,b.receipts});if(chain==old.timed.view.anchor)break;const auto wire=nodes[0]->signed_block(chain);check(bool(wire),"source historical archive absent");archived.push_back({Bytes(chain.begin(),chain.end()),b.header.slot,*wire});chain=b.header.parent_root;}
   std::reverse(suffix.begin(),suffix.end());
   ConsensusRoot oldest{};
   while(cursor!=old.timed.view.anchor){const auto expected=nodes[0]->signed_block(cursor);check(expected&&nodes[1]->signed_block(cursor)==expected,"pruned historical signed export differs");const auto info=nodes[1]->export_message("block",cursor,limits.envelope_bytes);check(info.at("available")==true&&info.at("historical")==true&&info.at("known")==false&&info.at("envelope").at("ssz")==hex(*expected)&&info.at("parentRoot")==hex(Bytes(old.blocks.at(cursor).header.parent_root.begin(),old.blocks.at(cursor).header.parent_root.end())),"historical export metadata differs");oldest=cursor;cursor=old.blocks.at(cursor).header.parent_root;++exported_count;}
   check(exported_count>0&&nodes[1]->current().token==saved,"historical reads changed durable state");const auto duplicate=*nodes[0]->signed_block(oldest);check(nodes[1]->block(duplicate)&&nodes[1]->current().token==saved,"pruned duplicate lost real admission ACK or changed state");auto wrong_signature=duplicate;wrong_signature[4]^=1;check(!nodes[1]->block(wrong_signature)&&nodes[1]->current().token==saved,"historical duplicate accepted different signature");bool cancelled=false;try{nodes[1]->signed_block(oldest,[]{throw ExecutionCancelled("historical read cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&nodes[1]->current().token==saved,"historical cancellation changed checkpoint");
   const auto source=owners[1]->store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,saved);check(bool(source),"pruned checkpoint missing");
   auto fork_wire=*nodes[0]->signed_block(oldest);auto fork_header=old.blocks.at(oldest).header;fork_header.state_root[0]^=1;std::copy(fork_header.state_root.begin(),fork_header.state_root.end(),fork_wire.begin()+148);
   const auto& historical_state=old.blocks.at(oldest).state.ssz;const auto context=capella_fork_block_context(old.blocks.at(oldest).state,old.blocks.at(oldest).header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
   ConsensusBytes4 version{};std::copy_n(historical_state.begin()+(context.registry.epoch<little(historical_state,56,8)?48:52),4,version.begin());
   const auto signature=Key(fork_header.proposer_index).sign(consensus_signing_root(consensus_beacon_header_root(fork_header),consensus_domain({0,0,0,0},version,genesis_root)));std::copy(signature.begin(),signature.end(),fork_wire.begin()+4);const auto foreign=consensus_beacon_header_root(fork_header);
   for(unsigned mutation=0;mutation<6;++mutation){
    Temp corrupt;ChainStore disk(corrupt/"db");disk.bind_chain(t.chain_id,genesis.header.hash(),"capella-historical-export-negative");disk.commit(genesis,genesis_state,{});auto entries=archived;
    if(mutation<3)for(auto& entry:entries)if(entry.root==Bytes(oldest.begin(),oldest.end())){if(mutation==0)entry.payload[4]^=1;else if(mutation==1)entry.payload.back()^=1;else ++entry.slot;}
    if(mutation==3)entries.push_back({Bytes(foreign.begin(),foreign.end()),fork_header.slot,fork_wire});
    if(mutation==4){const auto parent=old.blocks.at(anchor).header.parent_root;std::erase_if(entries,[&](const auto& entry){return entry.root==Bytes(parent.begin(),parent.end());});}
    const auto token=disk.commit_execution_checkpoint(t.chain_id,genesis.header.hash(),{},genesis.header.hash(),genesis.header.number,genesis.header.hash(),suffix,source->snapshot,limits.envelope_bytes,limits.blocks,entries);
    auto bounded=limits;if(mutation==5)bounded.blocks=pruned.blocks.size()+1;
    CapellaDurableReactor probe(disk,t.chain_id,genesis.header.hash(),rules,bounded,token);
    for(unsigned repeat=0;repeat<2;++repeat){
     if(mutation==5&&repeat==1){check(probe.signed_block(oldest)==nodes[0]->signed_block(oldest),"bounded proof did not resume from authenticated prefix");continue;}
     bool rejected=false;try{const auto wire=probe.signed_block(mutation==3?foreign:oldest);rejected=!wire;}catch(const std::invalid_argument&){rejected=true;}
     if(!rejected)std::cerr<<Json{{"failedHistoricalCase",mutation},{"repeat",repeat}}.dump()<<std::endl;
     check(rejected,"corrupt, foreign, incomplete or over-budget historical archive exported");
    }
    check(probe.current().token==token,"negative historical export changed checkpoint");
   }
   std::cout<<Json{{"scenario","capella-historical-export"},{"historicalBlocks",exported_count},{"negativeCases",5},{"boundedProofResumed",true},{"historicalDuplicateAck",true},{"differentSignatureRejected",true},{"depthBudgetEnforced",true},{"cachedSignatureRechecked",true},{"reopenedPrunedOwner",true},{"cancellationPreservedCheckpoint",true},{"networkTransport",false}}.dump()<<std::endl;
  }
  const auto next_slot=end+1,now=old.timed.clock.genesis_time_ms+next_slot*rules.slot_duration_ms;auto& proposer_node=nodes[(pruning_bls||exit_pending_pruning)?1:0];proposer_node->tick(now);const auto selected=proposer_node->current_proposer(next_slot);check(bool(selected),"next proposer missing after pruning");const auto wire=proposer_node->propose_current(*selected,next_slot,head.execution_block.header.coinbase,16,64,[&]{return now;},journal(*selected),[&](const ConsensusRoot& root){return Key(*selected).sign(root);});check(bool(wire),"next proposal missing");
  const auto before=prepare_capella_store_tick(old,now,rules,limits.replay_slots,limits.sync_trials),after=prepare_capella_store_tick(restored,now,rules,limits.replay_slots,limits.sync_trials);const auto reference=prepare_capella_received_block(before,*wire,rules,limits.replay_slots,limits.sync_trials),candidate=prepare_capella_received_block(after,*wire,rules,limits.replay_slots,limits.sync_trials);check(reference&&candidate,"pruned store rejected next valid block");const auto& a=capella_fork_head(*reference,rules);const auto& b=capella_fork_head(*candidate,rules);check(a.state==b.state&&a.execution_state.root()==b.execution_state.root()&&a.execution_block.header.hash()==b.execution_block.header.hash(),"pruned continuation differs");
  nodes[1]->tick(now);check(nodes[1]->block(*wire),"durably pruned owner rejected next block");check(owners[1]->tip().header.hash()==a.execution_block.header.hash()&&owners[1]->state().root()==a.execution_state.root(),"durably pruned owner continuation differs");
  if(pruning_slashing){const auto context=capella_fork_block_context(b.state,b.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.at(0).slashed,"preserved proof was not applied by next block");check(nodes[1]->signed_slashings()==pruning_proofs,"unfinalized slashing evidence retired after inclusion");}
  if(pruning_native){check(a.deposit_history&&b.deposit_history&&a.deposit_history->count()==1&&a.deposit_history->snapshot()==b.deposit_history->snapshot(),"new deposit after native pruning differs");const auto token=nodes[1]->current().token;nodes[1].reset();owners[1].reset();owners[1]=std::make_unique<Blockchain>(directory/"node-1",owner_genesis);nodes[1]=std::make_unique<CapellaDurableReactor>(*owners[1],rules,limits,token);check(capella_fork_head(*nodes[1]->current().state,rules).deposit_history->snapshot()==a.deposit_history->snapshot(),"new deposit history lost after native restart");}
  if(deposit_consumption){
   check(nodes[0]->block(*wire),"reference did not admit recorded deposit block");for(auto& node:nodes)node->set_automatic_pruning(true);
   const auto period=t.consensus.state.eth1_votes_limit;check(period&&period<=128,"deposit consumption fixture period unbounded");const auto next_period=(next_slot/period+1)*period;const auto stop=next_period+period/2+(deposit_activation?(t.consensus.exit.max_seed_lookahead+6)*slots:slots)+1;size_t consumed=0,new_votes=0;uint64_t consumed_slot=0,activation_epoch=UINT64_MAX;bool observed_pending=false,observed_active=false;const size_t eth1=176+64*t.consensus.state.slots_per_historical_root;const auto fee=head.execution_block.header.coinbase;
   bool pending_restarted=false,active_restarted=false,conflict_refused=false;
#include "support/capella_new_validator_slashing.hpp"
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
   // Delay inclusion of four epochs' genuinely signed duties, not checkpoints.
   // The next boundary admits these still-valid previous-epoch attestations.
   std::vector<Phase0SignedAttestation> delayed_slashing_votes;size_t delayed_slashing_admissions=0;
   const auto delayed_slashing_epoch=(next_period+period/2)/slots-1;
   for(uint64_t slot=next_slot+1;slot<=stop;++slot){const auto clock=old.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;for(auto& node:nodes)node->tick(clock);const auto prior=nodes[0]->current();const auto& parent=capella_fork_head(*prior.state,rules);const auto prior_index=little(parent.state.ssz,eth1+80,8);const auto advanced=prepare_capella_slots(parent.state,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,slot,limits.replay_slots,limits.sync_trials);const auto balances=little(advanced.ssz,eth1+92,4);const auto balance_before=little(advanced.ssz,balances+16,8);
    if(slashing_unknown_finality&&slot%slots==0&&!delayed_slashing_votes.empty()){
     for(const auto& vote:delayed_slashing_votes){check(vote.data.target.epoch+1==slot/slots,"delayed slashing duty not previous epoch");for(auto& node:nodes)check(node->vote(vote),"delayed valid attestation rejected");++delayed_slashing_admissions;}
     std::cout<<Json{{"scenario","new-validator-delayed-inclusion"},{"slot",slot},{"attestations",delayed_slashing_votes.size()}}.dump()<<std::endl;delayed_slashing_votes.clear();
    }
    observe_new_validator_slashing(slot);
    const auto proposer=nodes[0]->current_proposer(slot);if(slashing_new_validator&&(!proposer||(*proposer==64&&new_validator_proof))){++new_validator_skipped_proposals;continue;}check(bool(proposer),"deposit cycle proposer missing");const auto next=nodes[0]->propose_current(*proposer,slot,fee,16,64,[&]{return clock;},journal(*proposer),[&](const ConsensusRoot& root){return Key(*proposer).sign(root);});check(bool(next),"deposit cycle proposal missing");for(auto& node:nodes)check(node->block(*next),"deposit cycle block rejected");const auto left=nodes[0]->current(),right=nodes[1]->current();const auto& left_head=capella_fork_head(*left.state,rules);const auto& right_head=capella_fork_head(*right.state,rules);check(left_head.state==right_head.state&&left_head.execution_block.header.hash()==right_head.execution_block.header.hash()&&owners[0]->state().root()==owners[1]->state().root(),"deposit cycle nodes diverged");const auto index=little(left_head.state.ssz,eth1+80,8);check(index<=1&&index>=prior_index,"deposit cursor regressed or advanced twice");
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
    const auto replay_token=nodes[1]->current().token;check(nodes[1]->block(*next)&&nodes[1]->current().token==replay_token,"deposit block replay changed checkpoint");const auto duties=prepare_capella_attestation_duties(*left.state,rules,slot,limits.replay_slots,limits.sync_trials);for(const auto& [index,duty]:duties){if((automatic_registration||slashing_new_validator)&&index==64)continue;const auto vote=nodes[0]->sign_attestation(index,slot,[&]{return clock;},nodes[0]->current().token,journal(index),[&](const ConsensusRoot& root){return Key(index).sign(root);});check(bool(vote),"deposit cycle vote missing");if(slashing_unknown_finality&&slot/slots>=delayed_slashing_epoch&&slot/slots<delayed_slashing_epoch+4)delayed_slashing_votes.push_back(*vote);else for(auto& node:nodes)check(node->vote(*vote),"deposit cycle vote rejected");if(deposit_activation&&index==64){check(slot/slots>=activation_epoch,"new validator voted before activation");++new_votes;
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
    observe_new_validator_slashing(slot);
    if(slot%slots==0||slot==stop)std::cout<<Json{{"depositProgressSlot",slot},{"depositIndex",index},{"finalizedEpoch",left.state->timed.view.finalized.epoch}}.dump()<<std::endl;
   }
   if(registration_loop){const auto receipt=registration_loop->restart_commitments();check(receipt.size()==1&&receipt[0].at("index").is_null()&&receipt[0].at("publicKey")==hex(Key(64).public_key)&&new_votes>0,"automatic registration receipt or duties missing");std::cout<<Json{{"scenario","capella-automatic-registration"},{"newValidatorVotes",new_votes},{"signatures",registration_signatures},{"publicKeyBound",true}}.dump()<<std::endl;registration_loop.reset();registration_time.reset();}
   check(consumed==1&&consumed_slot>=next_period&&consumed_slot<=next_period+period/2,"deposit not consumed at native majority");for(size_t i=0;i<2;++i){const auto token=nodes[i]->current().token;nodes[i].reset();owners[i].reset();owners[i]=std::make_unique<Blockchain>(node_locations[i],owner_genesis);nodes[i]=std::make_unique<CapellaDurableReactor>(*owners[i],rules,limits,token);check(little(capella_fork_head(*nodes[i]->current().state,rules).state.ssz,eth1+80,8)==1,"consumed cursor lost on restart");}
   if(deposit_activation){check(observed_pending&&observed_active&&(slashing_new_validator||new_votes>0),"new validator never completed activation and voting");for(auto& node:nodes){const auto current=node->current();const auto& h=capella_fork_head(*current.state,rules);const auto context=capella_fork_block_context(h.state,h.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);check(context.registry.validators.size()==65&&context.registry.validators.at(64).activation_epoch==activation_epoch,"activation lost after restart");}}
   if(slashing_unknown_finality)check(delayed_slashing_votes.empty()&&delayed_slashing_admissions>0&&new_validator_unknown_finality_advances>0&&new_validator_unknown_advance_reopened,"missing unknown-member finality advancement/restart coverage");
   if(slashing_new_validator){observe_new_validator_slashing(stop);check(new_validator_unknown_reopened&&new_validator_unknown_observations>0&&new_validator_finalized_slash&&new_validator_forged_checks==16,"new validator slashing lifecycle incomplete");std::cout<<Json{{"scenario","capella-new-validator-slashing"},{"unknownFinalizedObservations",new_validator_unknown_observations},{"unknownFinalityAdvances",new_validator_unknown_finality_advances},{"forgedProofChecks",new_validator_forged_checks},{"proofReopenedBeforeFinalizedRegistration",true},{"finalizedSanctionRetired",true},{"skippedOffenderProposals",new_validator_skipped_proposals},{"networkTransport",false}}.dump()<<std::endl;}
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
static void signer_health_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 const auto validators=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
 for(unsigned mode=0;mode<2;++mode){
  Temp directory;Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
  Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");
  auto reactor=std::make_shared<CapellaDurableReactor>(owner,rules,crash_limits,initial,std::function<void()>{},&witness);
  std::atomic<uint64_t> now{initial.timed.clock.genesis_time_ms+2*rules.slot_duration_ms};auto wall=[&]{return now.load();};
  auto clock=std::make_shared<CapellaLocalClock>(*reactor,2*rules.slot_duration_ms,wall);clock->poll();
  const auto proposer=reactor->current_proposer(2);check(bool(proposer),"health fixture proposer missing");
  std::map<uint64_t,std::unique_ptr<ChainStore>> disks,key_witnesses;
  std::map<uint64_t,std::shared_ptr<BeaconSigningJournal>> journals;std::vector<CapellaValidatorKey> keys;
  uint64_t valid_calls=0,provider_calls=0;
  for(uint64_t i=0;i<64;++i){disks[i]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(i)));key_witnesses[i]=std::make_unique<ChainStore>(directory/("witness-"+std::to_string(i)));journals[i]=std::make_shared<BeaconSigningJournal>(*disks[i],t.chain_id,genesis.header.hash(),validators,Key(i).public_key,slots,std::nullopt,key_witnesses[i].get());keys.push_back({i,journals[i],[&,i](const ConsensusRoot& root){++valid_calls;return Key(i).sign(root);},genesis.header.coinbase,{}});}
  auto faulty_keys=keys;for(auto& key:faulty_keys){const auto i=key.index;key.signer=[&,i](const ConsensusRoot& root)->Bytes{if(++provider_calls==mode+1)throw std::runtime_error("private provider sentinel");return Key(i).sign(root);};}
  auto faulty=std::make_shared<CapellaValidatorLoop>(*reactor,*clock,rules.slot_duration_ms,slots,16,64,faulty_keys,wall);
  const auto before=reactor->current().token,journal_before=journals.at(*proposer)->commitment();
  bool cancelled=false;try{faulty->poll([]{throw ExecutionCancelled("health test cancellation");});}catch(const ExecutionCancelled&){cancelled=true;}
  check(cancelled&&faulty->healthy()&&provider_calls==0&&reactor->current().token==before&&journals.at(*proposer)->commitment()==journal_before,"cancellation poisoned health or reservation");
  auto runtime=std::make_shared<CapellaRuntime>();runtime->reactor=reactor;runtime->clock=clock;runtime->validator=faulty;
  Rpc rpc(owner,"health-test-token",AdmissionBudget::Clock::now,{}, {}, {},runtime);HttpServer server(rpc,"127.0.0.1",0);
  struct RunningHttp{HttpServer& server;std::thread thread;explicit RunningHttp(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningHttp(){server.stop();thread.join();}} http(server);
  PersistentRpc client(server.port());auto status=[&]{return client.call("nano_capellaStatus",Json::array({"health-test-token"}));};
  bool unauthorized=false;try{client.call("nano_capellaStatus",Json::array({"wrong-token"}));}catch(const PersistentRpcError& e){unauthorized=e.code==-32001;}check(unauthorized&&status().at("validatorHealthy")==true,"private health authentication or initial status failed");
  CapellaValidatorFailureAlert alert;check(!alert.poll(nullptr)&&!alert.poll(faulty.get()),"healthy/unconfigured signer generated an alert");
  faulty->start(std::chrono::milliseconds(1));const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(faulty->healthy()&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));faulty->stop();
  check(provider_calls==mode+1&&!faulty->healthy()&&clock->healthy(),"provider fault did not isolate the signer");
  const auto pinned=journals.at(*proposer)->commitment(),after_failure=reactor->current().token;
  check(mode?journals.at(*proposer)->latest_proposal_slot()==2&&pinned.sequence>journal_before.sequence:!journals.at(*proposer)->latest_proposal_slot()&&pinned==journal_before,"proposal reservation boundary differs");
  const auto report=alert.poll(faulty.get());check(report&&*report==Json{{"event","capella-validator-unavailable"},{"nodeShutdownRequested",false}}&&!alert.poll(faulty.get()),"shared daemon alert was missing, repeated or leaked details");
  auto failed_status=status();const auto relay_status=reactor->relay_head();check(failed_status.at("headRoot")==relay_status.at("headRoot")&&failed_status.at("headSlot")==relay_status.at("headSlot")&&failed_status.at("headSlot").is_number_unsigned(),"Private RPC omitted coherent relay head");check(failed_status.at("validatorHealthy")==false&&failed_status.at("clockHealthy")==true&&failed_status.dump().find("private provider sentinel")==std::string::npos,"HTTP status masked failure or leaked provider detail");
  bool origin_refused=false;try{client.call("nano_capellaStatus",Json::array({"health-test-token"}),"https://untrusted.invalid");}catch(const std::runtime_error& e){const std::string message=e.what();origin_refused=message.find("HTTP failure (403)")!=std::string::npos&&message.find("validatorHealthy")==std::string::npos&&message.find("private provider sentinel")==std::string::npos;}check(origin_refused,"Origin request exposed signer status");
  bool refused=false;try{faulty->poll();}catch(const std::runtime_error&){refused=true;}check(refused&&provider_calls==mode+1&&reactor->current().token==after_failure,"failed worker retried or mutated admission");
  {
   CapellaValidatorLoop alternate(*reactor,*clock,rules.slot_duration_ms,slots,16,64,keys,wall);
   if(mode){alternate.poll();check(valid_calls==0&&journals.at(*proposer)->commitment()==pinned,"alternate provider retried a reserved slot");}
   now=initial.timed.clock.genesis_time_ms+3*rules.slot_duration_ms;alternate.poll();
   check(alternate.healthy()&&capella_fork_head(*reactor->current().state,rules).header.slot==3&&owner.tip().header.number>genesis.header.number&&!faulty->healthy(),"provider fault poisoned next-slot admission");
  }
  runtime->validator.reset();check(status().at("validatorHealthy").is_null(),"observer fabricated configured signer health");
  std::map<uint64_t,ForkCheckpointToken> pins;std::map<uint64_t,std::optional<uint64_t>> proposal_slots;
  for(const auto& [i,journal]:journals){pins[i]=journal->commitment();proposal_slots[i]=journal->latest_proposal_slot();}
  faulty.reset();faulty_keys.clear();keys.clear();journals.clear();disks.clear();key_witnesses.clear();
  for(uint64_t i=0;i<64;++i){ChainStore disk(directory/("key-"+std::to_string(i))),key_witness(directory/("witness-"+std::to_string(i)));BeaconSigningJournal reopened(disk,t.chain_id,genesis.header.hash(),validators,Key(i).public_key,slots,pins.at(i),&key_witness);check(reopened.commitment()==pins.at(i)&&reopened.latest_proposal_slot()==proposal_slots.at(i),"retained signer/witness history did not reopen");}
 }
 std::cout<<Json{{"scenario","capella-signer-health"},{"failureCases",2},{"realWorker",true},{"httpTransport",true},{"originRejected",true},{"wrongTokenRejected",true},{"cancelledWorkerHealthy",true},{"reservedSlotNotRetried",true},{"sameJournalsNextSlot",true},{"journalsReopenedPerCase",64},{"sharedDaemonAlertOnce",true},{"privateFailureRedacted",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void duty_timing_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;const auto test_slot=slots+1;
 for(unsigned mode=0;mode<4;++mode){
  Temp directory;Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
  Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");CapellaDurableReactor reactor(owner,rules,crash_limits,initial,std::function<void()>{},&witness);
  uint64_t now=initial.timed.clock.genesis_time_ms+2*rules.slot_duration_ms;auto wall=[&]{return now;};CapellaLocalClock clock(reactor,2*rules.slot_duration_ms,wall);clock.poll();
  const auto validators=root_at(initial.blocks.at(initial.timed.view.anchor).state.ssz,8);
  std::map<uint64_t,std::unique_ptr<ChainStore>> disks;std::map<uint64_t,std::shared_ptr<BeaconSigningJournal>> journals;uint64_t observer_calls=0;
  auto key_for=[&](uint64_t i,bool observer){disks[i]=std::make_unique<ChainStore>(directory/("key-"+std::to_string(i)));journals[i]=std::make_shared<BeaconSigningJournal>(*disks[i],t.chain_id,genesis.header.hash(),validators,Key(i).public_key,slots);return CapellaValidatorKey{i,journals[i],[&,i,observer](const ConsensusRoot& root){if(observer)++observer_calls;return Key(i).sign(root);},genesis.header.coinbase,{}};};
  std::vector<CapellaValidatorKey> all_keys;for(uint64_t i=0;i<64;++i)all_keys.push_back(key_for(i,false));
  {CapellaValidatorLoop warm(reactor,clock,rules.slot_duration_ms,slots,16,64,all_keys,wall);for(uint64_t slot=2;slot<=slots;++slot){now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;warm.poll();check(capella_fork_head(*reactor.current().state,rules).header.slot==slot,"timing warmup block missing");}}
  now=initial.timed.clock.genesis_time_ms+test_slot*rules.slot_duration_ms;clock.poll();const auto proposer=reactor.current_proposer(test_slot);check(bool(proposer),"timing fixture proposer missing");
  std::vector<CapellaValidatorKey> observer_keys;for(auto key:all_keys)if(key.index!=*proposer){const auto i=key.index;key.signer=[&,i](const ConsensusRoot& root){++observer_calls;return Key(i).sign(root);};observer_keys.push_back(std::move(key));}
  const auto producer_key=all_keys.at(*proposer);CapellaValidatorLoop observer(reactor,clock,rules.slot_duration_ms,slots,16,64,observer_keys,wall);
  const auto old_head=consensus_beacon_header_root(capella_fork_head(*reactor.current().state,rules).header);
  observer.poll();check(observer_calls==0&&observer.status().at("attestationsAdmitted")==0&&!reactor.current_duties_ready(test_slot),"observer signed old head at slot start");
  const auto before=reactor.current().token;bool rejected=false;try{rejected=!reactor.block(Bytes(184,0));}catch(const std::invalid_argument&){rejected=true;}check(rejected&&reactor.current().token==before,"invalid block changed timing readiness");
  const auto due=rules.attestation_due_ms;now+=due-1;observer.poll();check(observer_calls==0,"observer signed before block/deadline");
  check(due>0,"timing fixture deadline missing");
  if(mode==0){CapellaValidatorLoop producer(reactor,clock,rules.slot_duration_ms,slots,16,64,{producer_key},wall);producer.poll();check(reactor.current_duties_ready(test_slot),"valid delayed block unavailable");observer.poll();const auto head=consensus_beacon_header_root(capella_fork_head(*reactor.current().state,rules).header);check(head!=old_head&&observer.status().at("attestationsAdmitted").get<uint64_t>()>0,"observer did not vote after valid block");for(const auto& [id,wire]:reactor.signed_votes()){const auto vote=decode_consensus_attestation_ssz(wire,t.consensus.body.validators_per_committee);check(vote&&(vote->data.slot!=test_slot||vote->data.beacon_block_root==head),"delayed observer voted for old head");}}
  else if(mode==1){++now;observer.poll();check(observer.status().at("attestationsAdmitted").get<uint64_t>()>0,"missing block suppressed deadline fallback");for(const auto& [id,wire]:reactor.signed_votes()){const auto vote=decode_consensus_attestation_ssz(wire,t.consensus.body.validators_per_committee);check(vote&&(vote->data.slot!=test_slot||vote->data.beacon_block_root==old_head),"fallback changed selected head");}}
  else if(mode==2){
   // The observer may own the NEXT proposer. New-slot signatures are legitimate;
   // the missed slot must never be backfilled, including its sync messages.
   now=initial.timed.clock.genesis_time_ms+(test_slot+1)*rules.slot_duration_ms;clock.poll();
   const auto next_proposer=reactor.current_proposer(test_slot+1);check(next_proposer&&*next_proposer!=*proposer,"timing fixture needs distinct next proposer");
   observer.poll();check(observer.status().at("proposalsAdmitted")==1&&capella_fork_head(*reactor.current().state,rules).header.slot==test_slot+1,"next-slot proposal was suppressed");
   check(!reactor.current_duties_ready(test_slot),"missed slot became eligible after boundary");
   for(const auto& [index,journal]:journals)check(journal->latest_proposal_slot()!=test_slot,"missed-slot proposal signed late");
   for(const auto& [id,wire]:reactor.signed_votes()){const auto vote=decode_consensus_attestation_ssz(wire,t.consensus.body.validators_per_committee);check(vote&&vote->data.slot!=test_slot,"missed-slot attestation signed late");}
   const auto sync=reactor.capture_inventory("sync_committee_message",4096)->page({},128,32768);
   for(const auto& entry:sync.at("entries")){const auto id=root_at(unhex(entry.get<std::string>()),0);const auto exported=reactor.export_message("sync_committee_message",id,1024);const auto message=decode_capella_sync_message(unhex(exported.at("envelope").at("ssz").get<std::string>()));check(message&&message->slot!=test_slot,"missed-slot sync message signed late");}
  }
  else{++now;const auto began=std::chrono::steady_clock::now();observer.start(std::chrono::milliseconds(rules.slot_duration_ms));const auto deadline=began+std::chrono::milliseconds(3*rules.slot_duration_ms/4);while(observer.status().at("attestationsAdmitted")==0&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(10));observer.stop();check(observer.status().at("attestationsAdmitted").get<uint64_t>()>0&&std::chrono::steady_clock::now()-began<std::chrono::milliseconds(rules.slot_duration_ms),"full-slot worker interval starved deferred duties");}
  const auto calls=observer_calls;observer.poll();check(observer_calls==calls,"duplicate poll repeated provider signatures");
 }
 std::cout<<Json{{"scenario","capella-duty-timing"},{"cases",4},{"earlyOldHeadPrevented",true},{"validBlockBeforeDeadline",true},{"invalidBlockNoRelease",true},{"deadlineFallback",true},{"missedSlotNoLateSignature",true},{"duplicatePollNoResign",true},{"fullSlotWorkerInterval",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void startup_catchup_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 const auto limits=crash_limits;const auto& t=rules.transition;unsigned passed=0;
 for(const std::string mode:{"success","within-phase","downtime","steps","cancel","backward","jump","used","default-refusal"}){
  Temp directory;Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
  Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");CapellaDurableReactor reactor(owner,rules,limits,initial,std::function<void()>{},&witness);
  const auto base=initial.timed.clock.time_ms,bound=2*rules.slot_duration_ms;uint64_t now=base+(mode=="within-phase"?1:5*rules.slot_duration_ms);unsigned reads=0;
  if(mode=="backward")now=base-1;
  CapellaLocalClock clock(reactor,bound,[&]{++reads;if(mode=="jump"&&reads>=3)now+=bound+1;return now;});
  const auto before=reactor.current();std::string failure;Json report;bool cancellation=false;
  if(mode=="used"){now=base;clock.poll();}
  try{
   if(mode=="default-refusal")clock.poll();
   else report=clock.startup_catch_up(mode=="downtime"?5*rules.slot_duration_ms-1:10*rules.slot_duration_ms,mode=="steps"?1:8,[&]{if(mode=="cancel"&&reads>=3){cancellation=true;throw std::runtime_error("cancel startup");}});
  }catch(const std::exception& e){failure=e.what();}
  const auto saved=owner.store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes);check(bool(saved),"startup lost durable checkpoint");
  const auto intent=witness.fork_checkpoint(t.chain_id,genesis.header.hash(),2048+8*limits.envelope_bytes);check(bool(intent),"startup lost independent witness");const auto publication=Json::parse(intent->snapshot);
  check(publication.at("snapshot")==saved->snapshot&&publication.at("next").at("digest")==hex(saved->token.digest),"startup witness differs from primary publication");
  if(mode=="success"||mode=="within-phase"){
   check(failure.empty()&&clock.healthy()&&report.at("steps")==uint64_t(mode=="success"?3:1),"bounded startup did not complete");
   const auto expected=prepare_capella_store_tick(initial,now,rules,limits.replay_slots,limits.sync_trials);
   check(capella_store_snapshot(*reactor.current().state,rules,limits)==capella_store_snapshot(expected,rules,limits),"chunked startup differs from direct transition oracle");
   check(reactor.signed_votes().empty(),"startup invented signed votes");
   bool reused=false;try{clock.startup_catch_up(10*rules.slot_duration_ms,8);}catch(const std::invalid_argument&){reused=true;}check(reused&&clock.healthy(),"startup allowance reused after completion");
   now+=bound+1;bool refused=false;try{clock.poll();}catch(const std::runtime_error&){refused=true;}check(refused&&!clock.healthy(),"startup weakened ordinary forward-jump refusal");
  }else{
   check(!failure.empty(),"invalid startup accepted");
   const bool partial=mode=="steps"||mode=="cancel"||mode=="jump";
   check((saved->token.sequence>before.token.sequence)==partial,"startup durable prefix changed unexpectedly");
   check(clock.healthy()==(mode=="cancel"||mode=="used"),"startup cancellation/failure latch incorrect");
   if(mode=="cancel")check(cancellation&&reactor.current().state->timed.clock.time_ms==base+bound,"cancelled startup lost committed prefix");
   if(mode=="steps")check(failure.find("step budget exceeded")!=std::string::npos,"missing step bound failure");
   if(mode=="downtime")check(failure.find("downtime budget exceeded")!=std::string::npos,"missing downtime bound failure");
  }
  ++passed;
 }
 std::cout<<Json{{"scenario","capella-startup-catchup"},{"scenarios",passed},{"directTransitionOracle",true},{"witnessedDurablePrefixes",true},{"runtimeJumpRefusalPreserved",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void clock_diagnostics_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 const auto limits=crash_limits;const auto& t=rules.transition;unsigned passed=0;
 for(unsigned mode=0;mode<4;++mode){
  Temp directory;Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
  Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");
  CapellaDurableReactor reactor(owner,rules,limits,initial,std::function<void()>{},&witness);
  const auto base=initial.timed.clock.time_ms;uint64_t now=base;const uint64_t bound=mode==3?1:rules.slot_duration_ms;
  CapellaLocalClock clock(reactor,bound,[&]{return now;});clock.poll();
  // Polls within a phase update the observation without publishing a checkpoint.
  if(mode!=3){now=base+2;check(!clock.poll(),"diagnostic fixture did not coalesce");}
  const auto saved=reactor.current();const auto record=owner.store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,saved.token);
  check(bool(record),"missing diagnostic checkpoint");const auto observed=now;
  now=mode==0?base-1:mode==1?base+1:mode==2?observed+bound+1:base+1;
  std::string failure;try{clock.poll();}catch(const std::runtime_error& error){failure=error.what();}
  if(mode==3){check(failure.empty()&&clock.healthy()&&reactor.current().token==saved.token,"inclusive forward bound rejected or published unchanged phase");}
  else{
   const bool before_durable=now<saved.state->timed.clock.time_ms,before_observed=now<observed;
   const bool exceeded=now>=observed&&now-observed>bound;
   for(const auto& value:{std::string("Capella local clock discontinuity"),"nowMs="+std::to_string(now),"durableMs="+std::to_string(saved.state->timed.clock.time_ms),"observedMs="+std::to_string(observed),"maximumForwardStepMs="+std::to_string(bound),"beforeDurable="+std::to_string(before_durable),"beforeObserved="+std::to_string(before_observed),"forwardStepExceeded="+std::to_string(exceeded)})
    check(failure.find(value)!=std::string::npos,"clock failure lost measured discriminator");
   check(!clock.healthy(),"clock fault did not latch");bool closed=false;
   try{owner.tip();}catch(const std::runtime_error&){closed=true;}check(closed,"clock fault left execution owner available");
   closed=false;try{reactor.beacon_read("/eth/v1/beacon/headers/head");}catch(const std::runtime_error& error){closed=std::string(error.what()).find(failure)!=std::string::npos;}check(closed,"Beacon did not preserve exact initial clock failure");
   reactor.invalidate();closed=false;try{reactor.current();}catch(const std::runtime_error& error){closed=std::string(error.what()).find(failure)!=std::string::npos;}check(closed,"subsequent invalidation replaced initial clock diagnosis");
  }
  const auto after=owner.store().fork_checkpoint(t.chain_id,genesis.header.hash(),limits.envelope_bytes,saved.token);
  check(after&&after->snapshot==record->snapshot&&after->token==record->token,"clock fault changed durable checkpoint");++passed;
 }
 std::cout<<Json{{"scenario","capella-clock-diagnostics"},{"scenarios",passed},{"failClosedPreserved",true},{"checkpointUnchanged",true},{"networkTransport",false}}.dump()<<std::endl;
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
 const Int sender_start_nonce=owner->state().get(sender).nonce;uint64_t admitted_transactions=0;
 uint64_t proposals=0,votes=0,sync_messages=0;
 const auto period=t.consensus.epoch.sync.epochs_per_sync_committee_period;check(period&&period<=16,"rotation fixture period must remain bounded");const auto boundary=period*slots;
 const uint64_t stop_slot=sync_rotation?(3*period+3)*slots+1:5*slots+1;
 Bytes previous_committee,previous_next;unsigned rotations=0,committee_changes=0,verified_aggregates=0;size_t maximum_records=0;
 for(uint64_t slot=2;slot<=stop_slot;++slot){
  now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;const auto initial_nonce=owner->state().get(sender).nonce;
  if(slot==2){
   // Exceeds the 16-trial budget; network/reorg arrival order is not nonce order.
   for(unsigned i=32;i>0;--i){tx.nonce=initial_nonce+i-1;check(owner->submit(SignedTransaction::sign(tx,key)),"out-of-order scheduler submission");}
   admitted_transactions+=32;
  }else if(!owner->pending_count()){tx.nonce=initial_nonce;check(owner->submit(SignedTransaction::sign(tx,key)),"scheduler mempool submit");++admitted_transactions;}
  const auto pending_before=owner->pending_count(),expected_transactions=std::min<size_t>(16,pending_before);
  loop->poll();const auto current=reactor->current();const auto& head=capella_fork_head(*current.state,rules);check(head.header.slot==slot&&head.execution_block.transactions.size()==expected_transactions&&owner->pending_count()==pending_before-expected_transactions,"scheduler failed to drain nonce-ready mempool prefix");
  for(size_t i=0;i<expected_transactions;++i)check(head.execution_block.transactions[i].tx.nonce==initial_nonce+Int(std::to_string(i)),"scheduler included transactions out of nonce order");
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
 const auto current=reactor->current();check(current.state->timed.view.finalized.epoch>=3&&owner->state().get(sender).nonce==sender_start_nonce+Int(std::to_string(admitted_transactions)),"scheduled canonical nonce or finality differs");
 check(current.state->blocks.at(current.state->timed.view.finalized.root).execution_state.get(sender).nonce>=sender_start_nonce+32,"out-of-order nonce batch not finalized");
 const auto finalized_header=reactor->beacon_read("/eth/v1/beacon/headers/finalized");const auto finalized_root=current.state->timed.view.finalized.root;check(finalized_header.status==200&&finalized_header.body.at("finalized")==true&&finalized_header.body.at("data").at("root")==hex(Bytes(finalized_root.begin(),finalized_root.end())),"scheduled finalized Beacon header mismatch");
 proposals+=loop->status().at("proposalsAdmitted").get<uint64_t>();votes+=loop->status().at("attestationsAdmitted").get<uint64_t>();sync_messages+=loop->status().at("syncMessagesAdmitted").get<uint64_t>();check(proposals==stop_slot-1&&votes>0&&sync_messages>0,"scheduler duty counters");
 if(sync_rotation)check(rotations==3&&committee_changes>=2&&verified_aggregates==stop_slot-2&&maximum_records<=5*slots&&current.state->timed.view.finalized.epoch>=3*period,"rotation/finality/pruning qualification incomplete");
 {ChainStore audit_disk(directory/"owner",ChainStore::OpenMode::ReadOnly);
 const auto audited=audit_capella_canonical_history(audit_disk,t.chain_id,genesis.header.hash(),rules,limits,initial,current.token,stop_slot,512*1024*1024);
 check(audited.at("canonicalHistoryVerified")==true&&audited.at("historicalForkChoiceVerified")==false&&audited.at("blocksIncludingTrustedAnchor")==stop_slot,"canonical history audit scope/count");
 bool audit_rejected=false;try{audit_capella_canonical_history(audit_disk,t.chain_id,genesis.header.hash(),rules,limits,initial,current.token,1,512*1024*1024);}catch(const std::exception&){audit_rejected=true;}check(audit_rejected,"audit ignored block budget");
 auto wrong_commitment=current.token;wrong_commitment.digest[0]^=1;audit_rejected=false;try{audit_capella_canonical_history(audit_disk,t.chain_id,genesis.header.hash(),rules,limits,initial,wrong_commitment,stop_slot,512*1024*1024);}catch(const std::exception&){audit_rejected=true;}check(audit_rejected,"audit accepted wrong external checkpoint");
 std::cout<<audited.dump()<<std::endl;}
 bool cancelled=false;try{loop->poll([]{throw ExecutionCancelled("scheduler cancellation");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&loop->status().at("healthy")==true&&clock->healthy(),"cancellation poisoned scheduler");
 // Exercise the real worker lifecycle with a stable clock and already completed duties.
 const auto prior=signatures;const auto polls_before=loop->status().at("timing").at("polls").get<uint64_t>();loop->start(std::chrono::milliseconds(1));std::this_thread::sleep_for(std::chrono::milliseconds(20));loop->stop();check(signatures==prior&&loop->status().at("healthy")==true&&loop->status().at("timing").at("polls").get<uint64_t>()>polls_before,"worker failed to poll or duplicated completed duty");
 now-=1;bool rejected=false;try{clock->poll();}catch(const std::runtime_error&){rejected=true;}check(rejected&&!clock->healthy(),"backward clock accepted");bool closed=false;try{owner->tip();}catch(const std::runtime_error&){closed=true;}check(closed,"clock failure left execution readable");bool beacon_closed=false;try{reactor->beacon_read("/eth/v1/beacon/headers/head");}catch(const std::runtime_error&){beacon_closed=true;}check(beacon_closed,"clock failure left Beacon readable");
 bool diagnosed=false;try{reactor->current();}catch(const std::runtime_error& error){diagnosed=std::string(error.what()).find("initial failure: Capella local clock discontinuity")!=std::string::npos;}check(diagnosed,"reactor lost initial clock failure");
 reactor->invalidate();diagnosed=false;try{reactor->current();}catch(const std::runtime_error& error){diagnosed=std::string(error.what()).find("initial failure: Capella local clock discontinuity")!=std::string::npos;}check(diagnosed,"repeated invalidation lost first failure");
 std::cout<<Json{{"scenario",sync_rotation?"capella-scheduled-sync-rotation":"capella-scheduled-finality"},{"rotations",rotations},{"committeeChanges",committee_changes},{"restartSlot",sync_rotation?2*boundary-1:3*slots},{"verifiedSyncAggregates",verified_aggregates},{"maximumRetainedRecords",maximum_records},{"syncMessages",sync_messages},{"blocks",proposals},{"votes",votes},{"finalizedEpoch",current.state->timed.view.finalized.epoch},{"witnessRestart",true},{"signingJournalsReopened",64},{"duplicatePollNoSignature",true},{"backwardsClockClosedOwner",true},{"initialFailurePreserved",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void runtime_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 Temp directory;const auto limits=crash_limits;const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 Genesis g;g.chain_id=t.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;auto owner=std::make_unique<Blockchain>(directory/"owner",g);
 const auto profile=capella_fork_profile(rules).dump(),snapshot=capella_store_snapshot(initial,rules,limits);{std::ofstream f(directory/"profile.json");f<<profile;}{std::ofstream f(directory/"initial.json");f<<snapshot;}
 Json config{{"profile",{{"file",(directory/"profile.json").string()},{"sha256",hex(sha256(Bytes(profile.begin(),profile.end())))},{"maximumBytes","0x100000"}}},{"limits",{{"envelopeBytes",quantity(Int(std::to_string(limits.envelope_bytes)))},{"blocks",quantity(Int(std::to_string(limits.blocks)))},{"messages",quantity(Int(std::to_string(limits.messages)))},{"equivocations",quantity(Int(std::to_string(limits.equivocations)))},{"stateBytes",quantity(Int(std::to_string(limits.state_bytes)))},{"totalStateBytes",quantity(Int(std::to_string(limits.total_state_bytes)))},{"replaySlots",quantity(Int(std::to_string(limits.replay_slots)))},{"syncTrials",quantity(Int(std::to_string(limits.sync_trials)))}}},{"initial",{{"file",(directory/"initial.json").string()},{"sha256",hex(sha256(Bytes(snapshot.begin(),snapshot.end())))}}},{"restart",nullptr},{"recoveryWitness",(directory/"witness").string()},{"clock",{{"intervalMs","0x3e8"},{"maximumForwardStepMs",quantity(Int(std::to_string(2*rules.slot_duration_ms)))}}}};
 // Invalid tolerance is rejected before any checkpoint publication.
 for(const auto& drift:std::vector<Json>{"0x0","0x01",Json(1),quantity(Int(std::to_string(rules.attestation_due_ms))),quantity(Int(std::to_string(2*rules.slot_duration_ms)))}){
  auto invalid=config;invalid["clock"]["maximumWallClockDriftMs"]=drift;
  bool refused=false;try{auto rejected=start_capella_runtime(*owner,invalid,{},[]{return uint64_t(0);});}catch(const std::invalid_argument&){refused=true;}
  check(refused&&!owner->store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes),"Invalid wall drift published a checkpoint");
 }
 {Temp time_directory;Blockchain time_owner(time_directory/"chain",g);auto selected=config;
  selected["recoveryWitness"]=(time_directory/"witness").string();
  selected["clock"]["maximumWallClockDriftMs"]=quantity(Int(std::to_string(rules.attestation_due_ms/2)));
  const uint64_t initial_time=initial.timed.clock.time_ms;
  auto time_runtime=start_capella_runtime(time_owner,selected,{},[initial_time]{return initial_time;});
  check(time_runtime.clock->healthy(),"Monotonic runtime did not start");
  const auto first=time_runtime.reactor->current().state->timed.clock.time_ms;
  time_runtime.clock->poll();
  check(time_runtime.reactor->current().state->timed.clock.time_ms>=first,"Monotonic runtime time regressed");
  stop_capella_runtime(time_runtime);
 }
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
     if(deposit_network){
      check(rules.native_deposit_votes&&rules.deposit_contract_code_hash==keccak(genesis_state.get(ethereum_deposit_contract).code),"native deposit fixture code binding");
      const auto& head=capella_fork_head(initial,rules);const auto context=capella_fork_block_context(head.state,head.header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state);
      check(context.registry.validators.size()==64&&!context.registry.validators.contains(64),"new validator already registered");
      Phase0Deposit deposit{};Key depositor(64);copy_to(deposit.message.public_key,depositor.public_key);deposit.message.withdrawal_credentials[0]=1;deposit.message.withdrawal_credentials[31]=44;deposit.message.amount_gwei=t.consensus.deposit.maximum_effective_balance;
      copy_to(deposit.signature,depositor.sign(consensus_signing_root(consensus_deposit_root(deposit.message),consensus_domain({3,0,0,0},t.genesis_version,{}))));check(verify_consensus_deposit_signature(deposit.message,deposit.signature,t.genesis_version),"generated deposit signature invalid");
      const std::string method="deposit(bytes,bytes,bytes,bytes32)";auto data=keccak(Bytes(method.begin(),method.end()));data.resize(4);Bytes args(416);auto word=[&](size_t at,size_t n){const auto encoded=bytes(Int(n),32);std::copy(encoded.begin(),encoded.end(),args.begin()+at);};
      word(0,128);word(32,224);word(64,288);const auto root=consensus_deposit_data_root(deposit);std::copy(root.begin(),root.end(),args.begin()+96);word(128,48);std::copy(deposit.message.public_key.begin(),deposit.message.public_key.end(),args.begin()+160);word(224,32);std::copy(deposit.message.withdrawal_credentials.begin(),deposit.message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(deposit.signature.begin(),deposit.signature.end(),args.begin()+320);data.insert(data.end(),args.begin(),args.end());
      Transaction request;request.type=2;request.chain_id=t.chain_id;request.to=ethereum_deposit_contract;request.value=Int(std::to_string(deposit.message.amount_gwei))*1000000000;request.gas=1000000;request.max_fee=2000000000;request.priority_fee=100000000;request.data=std::move(data);
      const auto sender_key=bytes(Int(777),32);request.nonce=head.execution_state.get(SignedTransaction::sign(request,sender_key).sender()).nonce;const auto signed_tx=SignedTransaction::sign(request,sender_key);
      check(head.execution_state.get(signed_tx.sender()).balance>=request.value+request.gas*request.max_fee,"deposit sender fixture insufficient balance");
      save(network_export/"deposit.json",Json{{"kind","native_deposit_transaction"},{"raw",hex(signed_tx.raw())},{"hash",hex(signed_tx.hash())},{"validatorIndex",64},{"publicKey",hex(depositor.public_key)},{"amountGwei",deposit.message.amount_gwei},{"withdrawalRecipient","0x000000000000000000000000000000000000002c"},{"sender",signed_tx.sender()},{"nonce",quantity(request.nonce)},{"productionQualified",false}});
      auto remote=load(network_export/"node-1.json"),variables=load(network_export/"env-1.json");const std::string name="NANO_TEST_CAPELLA_NETWORK_KEY_64";variables[name]=hex(bytes(Int(65),32));
      remote["capella"]["validator"]["keys"].push_back({{"index",nullptr},{"feeRecipient",genesis.header.coinbase},{"keyEnv",name},{"journal","/var/lib/nano/key-64"},{"restart",nullptr},{"recoveryWitness","/var/lib/nano/key-witness-64"}});save(network_export/"node-1.json",remote);save(network_export/"env-1.json",variables);
      meta["nativeDepositTest"]=true;meta["pendingValidators"]=1;meta["initialHead"]=hex(Bytes(initial.timed.view.anchor.begin(),initial.timed.view.anchor.end()));meta["productionQualified"]=false;
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
     // Admit with no peer, close the owner, then reconnect without re-submission.
     // This distinguishes durable operation delivery from learning an exit in a block.
     auto offline=follower_config;offline["bootnodes"]=Json::array();save(follower_file,offline);
     Process disconnected({cli_binary,"run",(directory/"genesis.json").string(),(directory/"follower-db").string(),follower_file.string()},follower_env);
     const auto offline_port=disconnected.port();check(disconnected.event("altair-ipc-ready").at("networkProtocolActive")==true,"offline exit admission did not start real daemon");
     check(quantity(rpc(offline_port,"net_peerCount"))==0,"offline exit unexpectedly had peers");
     const auto stopped=disconnected.stop();std::cout<<stopped.out<<stopped.err;check(stopped.code==0,"offline exit owner did not stop cleanly");
     {Blockchain reopened(directory/"follower-db",g);ChainStore witness(directory/"follower-witness");CapellaDurableReactor recovered(reopened,rules,limits,witness);const auto pending=recovered.pending_exits();check(pending.size()==1&&encode_consensus_voluntary_exit_ssz(pending.front())==encode_consensus_voluntary_exit_ssz(operation),"offline exit not preserved exactly");}
     follower_env.erase("NANO_TEST_EXIT_BOOTSTRAP");follower_env.erase("NANO_TEST_BLS_DAEMON");
     follower_config["p2p_binary"]=daemon_binary;follower_config["capella"]["automaticRecovery"]=true;save(follower_file,follower_config);
     std::cout<<Json{{"event","capella-exit-offline-reopened"},{"pendingBytesVerified",true},{"resubmitted",false},{"peersBeforeStop",0}}.dump()<<std::endl;
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
     bool delivered=false;std::istringstream relay_events(b.out);std::string relay_line;
     while(std::getline(relay_events,relay_line)){const auto event=Json::parse(relay_line,nullptr,false);if(event.is_object()&&event.value("event","")=="test-capella-catalog-relay"&&event.value("kind","")=="voluntary_exit"&&event.value("delivered",uint64_t(0))>0)delivered=true;}
     check(delivered,"reconnected exit was not delivered as a standalone libp2p operation");
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
 // Peer observations describe monitoring only; they must never mutate consensus.
 runtime.validator->stop();runtime.clock->stop();
 const auto sync_before=runtime.reactor->current();const auto sync_time=now.load();
 check(runtime.beacon_read("/eth/v1/node/syncing",{}).status==503,"Capella missing peers declared synced");
 Rpc sync_rpc(*owner,"capella-sync-test");sync_rpc.set_beacon_sync_tracker(runtime.sync_tracker);
 auto report_sync=[&](Json slot,std::string token="capella-sync-test"){
  return sync_rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_networkReport"},{"params",Json::array({token,Json{{"peerId","self"},{"addresses",Json::array()},{"peers",Json::array({Json{{"peerId","peer"},{"bestBlock",uint64_t(999999)},{"beaconHeadSlot",slot}}})}}})}});
 };
 const auto head_slot=capella_fork_head(*sync_before.state,rules).header.slot;
 check(report_sync(head_slot,"wrong-token").contains("error")&&runtime.beacon_read("/eth/v1/node/syncing",{}).status==503,"Capella unauthorized sync observation");
 check(report_sync(head_slot).value("result",false),"Capella peer report rejected");
 auto sync=runtime.beacon_read("/eth/v1/node/syncing",{});
 check(sync.status==200&&sync.body.at("data")==Json{{"head_slot",std::to_string(head_slot)},{"sync_distance","0"},{"is_syncing",false},{"is_optimistic",false},{"el_offline",false}},"Capella sync projection");
 now=sync_time+2*rules.slot_duration_ms;check(report_sync(head_slot+2).value("result",false),"Capella ahead report rejected");
 sync=runtime.beacon_read("/eth/v1/node/syncing",{});check(sync.body.at("data").at("sync_distance")=="2"&&sync.body.at("data").at("is_syncing")==true,"Capella lag concealed");
 check(report_sync("0xffff").contains("error"),"Capella malformed peer slot accepted");
 check(report_sync(UINT64_MAX).value("result",false),"Capella future fixture rejected at wrong layer");
 bool future_refused=false;try{runtime.beacon_read("/eth/v1/node/syncing",{});}catch(const std::runtime_error&){future_refused=true;}check(future_refused,"Capella future peer claimed synced");
 runtime.sync_tracker->report({head_slot});check(sync_before.state->timed.clock.time_ms>0,"Capella clock regression fixture lacks positive time");now=sync_before.state->timed.clock.time_ms-1;
 bool clock_refused=false;try{runtime.beacon_read("/eth/v1/node/syncing",{});}catch(const std::runtime_error&){clock_refused=true;}check(clock_refused,"Capella backwards wall clock accepted");
 now=sync_time;bool sync_cancelled=false;try{runtime.beacon_read("/eth/v1/node/syncing",[]{throw std::runtime_error("cancel");});}catch(const std::runtime_error&){sync_cancelled=true;}check(sync_cancelled,"Capella sync cancellation ignored");
 runtime.sync_tracker->report({});check(runtime.beacon_read("/eth/v1/node/syncing",{}).status==503,"Capella isolated node declared synced");

 if(capella_http){
  HttpServer sync_server(sync_rpc,"127.0.0.1",0,std::chrono::seconds(4),runtime.beacon_read);
  std::thread sync_thread([&]{sync_server.run();});
  struct SyncHttpStop{HttpServer& server;std::thread& thread;~SyncHttpStop(){server.stop();thread.join();}} sync_stop{sync_server,sync_thread};
  auto get_sync=[&]{
   namespace asio=boost::asio;namespace http=boost::beast::http;
   asio::io_context context;boost::beast::tcp_stream socket(context);socket.expires_after(std::chrono::seconds(5));socket.connect({asio::ip::make_address("127.0.0.1"),sync_server.port()});
   http::request<http::empty_body> request(http::verb::get,"/eth/v1/node/syncing",11);request.set(http::field::host,"localhost");request.set(http::field::accept,"application/json");http::write(socket,request);
   boost::beast::flat_buffer buffer;http::response<http::string_body> response;http::read(socket,buffer,response);
   return BeaconReadResponse{unsigned(response.result_int()),Json::parse(response.body()),{}, {}};
  };
  PersistentRpc sync_client(sync_server.port());
  auto http_report=[&](uint64_t slot,std::string token="capella-sync-test"){
   return sync_client.call("nano_networkReport",Json::array({token,Json{{"peerId","self"},{"addresses",Json::array()},{"peers",Json::array({Json{{"peerId","peer"},{"bestBlock",uint64_t(999999)},{"beaconHeadSlot",slot}}})}}}));
  };
  check(get_sync().status==503,"Capella HTTP isolated node declared synced");
  bool unauthorized=false;try{http_report(head_slot,"wrong-token");}catch(const PersistentRpcError& e){unauthorized=e.code==-32001;}
  check(unauthorized&&get_sync().status==503,"Capella HTTP unauthorized observation changed syncing");
  check(http_report(head_slot)==true,"Capella HTTP peer report refused");sync=get_sync();
  check(sync.status==200&&sync.body.at("data").at("sync_distance")=="0"&&sync.body.at("data").at("head_slot")==std::to_string(head_slot),"Capella HTTP sync projection");
  now=sync_time+2*rules.slot_duration_ms;check(http_report(head_slot+2)==true,"Capella HTTP ahead observation refused");sync=get_sync();
  check(sync.status==200&&sync.body.at("data").at("sync_distance")=="2"&&sync.body.at("data").at("is_syncing")==true,"Capella HTTP lag concealed");
  check(http_report(UINT64_MAX)==true,"Capella HTTP future fixture report refused at wrong layer");sync=get_sync();
  check(sync.status==503&&sync.body.dump().find("Beacon peer claims future slot")==std::string::npos,"Capella HTTP future slot accepted or private failure leaked");
  now=sync_time;runtime.sync_tracker->report({});
 }
 check(runtime.reactor->current().token==sync_before.token,"Capella peer observation changed checkpoint");
 stop_capella_runtime(runtime);runtime={};runtime=start_capella_runtime(*owner,config,{},wall);if(automatic_registration)runtime.validator->set_duty_trace_limit(4);
 std::optional<ForkCheckpointToken> stale_token;uint64_t stale_time=0;
 const auto secret=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;const auto sender=SignedTransaction::sign(tx,secret).sender();uint64_t proposals=0;
 for(uint64_t slot=2;slot<=2*slots+1;++slot){
  tx.nonce=owner->state().get(sender).nonce;check(owner->submit(SignedTransaction::sign(tx,secret)),"runtime transaction refused");now=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;runtime.validator->poll();check(owner->state().get(sender).nonce==Int(std::to_string(slot)),"runtime block failed to execute");check(runtime.beacon_read("/eth/v2/beacon/blocks/head",{}).body.at("data").at("message").at("body").at("execution_payload").at("block_hash")==hex(owner->tip().header.hash()),"runtime Beacon handler diverged");now=now.load()+rules.attestation_due_ms;runtime.validator->poll();if(automatic_registration){const auto status=runtime.validator->status(true);check(status.at("healthy")==true&&status.at("awaitingRegistration")==1,"configured pending key did not wait");for(const auto& entry:status.at("dutyTrace"))for(const auto& message:entry.at("latestMessages"))check(message.at("local")==true,"automatic key trace used unresolved index");const auto commitments=runtime.validator->restart_commitments();check(commitments.back().at("index").is_null()&&commitments.back().at("publicKey")==hex(Key(64).public_key),"pending key restart identity lost");}
  if(slot==slots+1){const auto receipt=stop_capella_runtime(runtime);check(receipt.at("clockHealthy")==true&&receipt.at("validatorKeys").size()==(automatic_registration?65:64),"runtime stop receipt incomplete");proposals+=receipt.at("validator").at("proposalsAdmitted").get<uint64_t>();runtime={};if(automatic_recovery){stale_token=owner->store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes)->token;stale_time=now.load();}owner.reset();if(automatic_recovery){fs::copy(directory/"owner",directory/"stale-owner",fs::copy_options::recursive);fs::copy(directory/"witness",directory/"stale-witness",fs::copy_options::recursive);}owner=std::make_unique<Blockchain>(directory/"owner",g);runtime=start_capella_runtime(*owner,config,{},wall);}
 }
 check(runtime.reactor->network_identity()==runtime_identity,"runtime identity changed across restart");const auto saved=runtime.reactor->current();const auto root=consensus_beacon_header_root(capella_fork_head(*saved.state,rules).header);Json envelope=runtime_identity;envelope["kind"]="block";envelope["ssz"]=hex(*runtime.reactor->signed_block(root));auto done=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto result=done->get_future();check(runtime.admission->submit(envelope.dump(),std::make_shared<std::atomic<bool>>(false),[done](auto d){done->set_value(d);}),"runtime admission refused duplicate");check(result.wait_for(std::chrono::seconds(10))==std::future_status::ready&&result.get()==CapellaConsensusService::Decision::accepted&&runtime.reactor->current().token==saved.token,"runtime admission changed duplicate checkpoint");
 {
  Rpc rpc(*owner,"capella-test-token",AdmissionBudget::Clock::now,{}, {}, {},std::make_shared<CapellaRuntime>(runtime));check(rpc.requires_private_bind()&&rpc.local_only(),"Capella RPC exposed public bind");
  struct RunningHttp {HttpServer& server;std::thread thread;explicit RunningHttp(HttpServer& s):server(s),thread([&s]{s.run();}){}~RunningHttp(){server.stop();thread.join();}};
  std::unique_ptr<HttpServer> http_server;std::unique_ptr<RunningHttp> http_running;std::unique_ptr<PersistentRpc> http_client;
  if(capella_http){bool public_refused=false;try{HttpServer forbidden(rpc,"0.0.0.0",0);}catch(const std::invalid_argument&){public_refused=true;}check(public_refused,"Capella HTTP accepted public bind");http_server=std::make_unique<HttpServer>(rpc,"127.0.0.1",0,std::chrono::seconds(4),runtime.beacon_read);http_running=std::make_unique<RunningHttp>(*http_server);http_client=std::make_unique<PersistentRpc>(http_server->port());}
  auto call=[&](const char* method,Json params){if(http_client){try{return Json{{"result",http_client->call(method,std::move(params))}};}catch(const PersistentRpcError& e){return Json{{"error",{{"code",e.code}}}};}}return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});};

  if(capella_http_queue){
   // A real private RPC waits for admission while public identity and Beacon
   // reads must still run on the HTTP loop / another bounded worker.
   Json private_batch=Json::array();
   for(const auto* method:{"nano_capellaStatus","nano_capellaSubmit","nano_capellaJob","nano_capellaCancel","nano_capellaForget","nano_capellaJobs","nano_capellaRotateSession","nano_capellaExport","nano_capellaCatalogGeneration","nano_capellaCatalogAcquire","nano_capellaCatalogRelease","nano_capellaCatalogOpen","nano_capellaCatalogSeek","nano_capellaCatalogPage","nano_capellaCatalogClose"}){
    check(simulation_count(Json{{"method",method}})==1,"Capella notification bypasses worker reservation");private_batch.push_back(Json{{"method",method}});
   }
   check(simulation_count(private_batch)==SimulationQueue::limit+1,"oversized private batch bypasses capacity");
   runtime.clock->stop(); // Drive the same clock explicitly for deterministic contention.
   const auto before=runtime.reactor->current().token;
   std::promise<void> held,release;auto ready=held.get_future();auto gate=release.get_future().share();std::atomic<bool> paused=false;
   auto holder=std::async(std::launch::async,[&]{return runtime.reactor->signed_block(root,[&]{if(!paused.exchange(true)){held.set_value();gate.wait();}});});
   const bool acquired=ready.wait_for(std::chrono::seconds(2))==std::future_status::ready;
   if(!acquired){release.set_value();holder.get();check(false,"queue fixture did not hold real admission mutex");}
   std::promise<void> clock_entered;auto clock_ready=clock_entered.get_future();std::atomic<bool> clock_signaled=false;
   auto clock_waiter=std::async(std::launch::async,[&]{return runtime.clock->poll([&]{if(!clock_signaled.exchange(true))clock_entered.set_value();});});
   if(clock_ready.wait_for(std::chrono::seconds(2))!=std::future_status::ready){release.set_value();holder.get();clock_waiter.get();check(false,"clock poll did not enter while admission held");}
   auto blocked=std::async(std::launch::async,[&]{PersistentRpc client(http_server->port());return client.call("nano_capellaStatus",Json::array({"capella-test-token"}));});
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
   auto identity=std::async(std::launch::async,[&]{PersistentRpc client(http_server->port());return client.call("eth_chainId");});
   auto beacon=std::async(std::launch::async,[&]{namespace h=boost::beast::http;boost::asio::io_context io;boost::beast::tcp_stream socket(io);socket.connect({boost::asio::ip::make_address("127.0.0.1"),http_server->port()});h::request<h::empty_body> request(h::verb::get,"/eth/v1/beacon/states/head/finality_checkpoints",11);request.set(h::field::host,"localhost");request.set(h::field::accept,"application/json");h::write(socket,request);boost::beast::flat_buffer buffer;h::response<h::string_body> response;h::read(socket,buffer,response);return Json{{"status",response.result_int()},{"body",Json::parse(response.body())}};});
   const bool identity_ready=identity.wait_for(std::chrono::milliseconds(500))==std::future_status::ready;
   const bool beacon_ready=beacon.wait_for(std::chrono::milliseconds(500))==std::future_status::ready;
   const bool private_pending=blocked.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready;
   const bool clock_pending=clock_waiter.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready;
   release.set_value();clock_waiter.get();const auto archive=holder.get();const auto status=blocked.get(),chain_id=identity.get(),read=beacon.get();
   check(archive&&status.at("executionBound")==true&&chain_id==quantity(t.chain_id)&&read.at("status")==200,"queue fixture did not finish valid authenticated work");
   std::cout<<Json{{"event","capella-http-queue-observation"},{"publicReady",identity_ready},{"beaconReady",beacon_ready},{"privatePending",private_pending},{"checkpointUnchanged",runtime.reactor->current().token==before}}.dump()<<std::endl;
   check(identity_ready&&beacon_ready&&private_pending&&clock_pending&&runtime.clock->healthy()&&runtime.reactor->current().token==before,"blocked Capella private RPC stalled public HTTP or changed checkpoint");
   std::cout<<Json{{"scenario","capella-http-admission-queue"},{"realAdmissionMutexHeld",true},{"clockPollPending",clock_pending},{"privatePending",true},{"publicIdentityBeforeRelease",identity_ready},{"beaconBeforeRelease",beacon_ready},{"checkpointUnchanged",true},{"networkTransport",false}}.dump()<<std::endl;
  }

  for(const auto* method:{"nano_capellaStatus","nano_capellaJobs","nano_capellaCatalogGeneration"})check(call(method,Json::array({"wrong-token"})).at("error").at("code")==-32001,"unauthorized Capella read accepted");
  check(call("nano_capellaCatalogOpen",Json::array({"wrong-token","block"})).at("error").at("code")==-32001,"unauthorized catalog opened");
  check(call("nano_capellaSubmit",Json::array({"wrong-token","invalid","0x1",envelope.dump()})).at("error").at("code")==-32001,"unauthorized submit accepted");
  const auto status=call("nano_capellaStatus",Json::array({"capella-test-token"})).at("result");check(status.at("consensusIdentity")==runtime_identity.at("consensusIdentity")&&status.at("networkProtocolActive")==false,"Capella RPC status differs");
  const auto rpc_session=status.at("admissionSession");check(!call("nano_capellaSubmit",Json::array({"capella-test-token",rpc_session,"0x1",envelope.dump()})).contains("error"),"Capella RPC submit failed");
  const auto rpc_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);Json rpc_job;do{rpc_job=call("nano_capellaJob",Json::array({"capella-test-token",rpc_session,"0x1"})).at("result");if(rpc_job.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<rpc_deadline);check(rpc_job.at("state")=="completed"&&rpc_job.at("decision")=="accepted"&&runtime.reactor->current().token==saved.token,"Capella RPC result changed duplicate checkpoint");
  const auto exported=call("nano_capellaExport",Json::array({"capella-test-token","block",hex(Bytes(root.begin(),root.end()))})).at("result");check(exported.at("envelope")==envelope,"Capella RPC export differs");
  const auto handle=call("nano_capellaCatalogOpen",Json::array({"capella-test-token","block"})).at("result").at("handle");check(call("nano_capellaCatalogPage",Json::array({"capella-test-token",handle,nullptr,"0x2"})).at("result").at("entries").size()==2,"Capella RPC page failed");check(call("nano_capellaCatalogSeek",Json::array({"capella-test-token",handle,hex(Bytes(32,0)),"0x2"})).at("result").at("entries")==call("nano_capellaCatalogPage",Json::array({"capella-test-token",handle,nullptr,"0x2"})).at("result").at("entries"),"RPC seek below first root differs");check(call("nano_capellaCatalogSeek",Json::array({"wrong-token",handle,hex(Bytes(32,0)),"0x2"})).at("error").at("code")==-32001,"unauthorized seek accepted");check(call("nano_capellaCatalogSeek",Json::array({"capella-test-token",handle,hex(Bytes(32,0)),"0x0"})).contains("error"),"zero seek page count accepted");check(call("nano_capellaCatalogClose",Json::array({"capella-test-token",handle})).at("result")==true,"Capella RPC close failed");check(call("nano_capellaCatalogClose",Json::array({"capella-test-token",handle})).at("result")==true,"Capella RPC repeated close failed");
  check(call("nano_capellaCatalogPage",Json::array({"capella-test-token",handle,nullptr,"0x2"})).contains("error"),"closed RPC catalog readable");
  check(call("nano_capellaExport",Json::array({"capella-test-token","block","0x01"})).at("error").at("code")==-32602,"invalid RPC root accepted");
  if(capella_http){namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;asio::io_context context;beast::tcp_stream stream(context);stream.connect({asio::ip::make_address("127.0.0.1"),http_server->port()});http::request<http::string_body> request(http::verb::post,"/",11);request.set(http::field::host,"localhost");request.set(http::field::origin,"https://example.invalid");request.body()=Json{{"jsonrpc","2.0"},{"id",1},{"method","nano_capellaStatus"},{"params",Json::array({"capella-test-token"})}}.dump();request.prepare_payload();http::write(stream,request);beast::flat_buffer buffer;http::response<http::string_body> response;http::read(stream,buffer,response);check(response.result()==http::status::forbidden,"browser-origin Capella request accepted");}

 }
 const auto runtime_catalog=runtime.catalogs->open("authenticated-test-peer","block");check(!runtime.catalogs->page("authenticated-test-peer",runtime_catalog,{},2,512).at("entries").empty(),"configured catalog empty");
 {
  const auto snapshot=runtime.reactor->capture_inventory("block",limits.blocks*32);
  const auto all=snapshot->page({},limits.blocks,1024*1024).at("entries");check(!all.empty(),"seek fixture empty");
  std::vector<ConsensusRoot> boundaries{ConsensusRoot{},root_at(Bytes(32,255),0),root_at(unhex(all.front().get<std::string>()),0)};
  auto absent=boundaries.back();absent.back()^=1;boundaries.push_back(absent);
  for(const auto& boundary:boundaries){Json expected=Json::array();const auto text=hex(Bytes(boundary.begin(),boundary.end()));for(const auto& id:all)if(id.get<std::string>()>text&&expected.size()<2)expected.push_back(id);
   check(snapshot->seek(boundary,2,512).at("entries")==expected,"snapshot seek changed sorted selection");
   check(runtime.catalogs->seek("authenticated-test-peer",runtime_catalog,boundary,2,512).at("entries")==expected,"session seek differs from frozen snapshot");
  }
  bool rejected=false;try{runtime.catalogs->seek("foreign-peer",runtime_catalog,{},2,512);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"foreign seek accessed snapshot");
  rejected=false;try{snapshot->seek({},0,512);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"snapshot seek ignored count budget");
  rejected=false;try{snapshot->page(root_at(Bytes(32,255),0),2,512);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"strict page cursor validation weakened");
 }
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
 if(automatic_recovery){auto missing=config;missing["recoveryWitness"]=(directory/"missing-recovery-witness").string();bool rejected=false;try{auto invalid=start_capella_runtime(*owner,missing,{},wall);}catch(const std::exception&){rejected=true;}check(rejected,"automatic recovery bootstrapped over primary with missing witness");auto commitment=[](const ForkCheckpointToken& token){return Json{{"sequence",quantity(Int(std::to_string(token.sequence)))},{"digest",hex(token.digest)}};};
  auto pinned=config;pinned["recoveryCommitment"]=commitment(saved.token);
  auto malformed=pinned;malformed["recoveryCommitment"]=nullptr;refused(malformed);malformed=pinned;malformed.erase("recoveryWitness");refused(malformed);malformed=pinned;malformed["recoveryCommitment"]["sequence"]="0x0";refused(malformed);malformed=pinned;malformed["recoveryCommitment"]["digest"]="0x01";refused(malformed);
  malformed=pinned;malformed["recoveryWitness"]=(directory/"absent-pinned-witness").string();refused(malformed);check(!fs::exists(directory/"absent-pinned-witness"),"pinned missing witness was created");
  unsigned clock_reads=0;auto counted_wall=[&]{++clock_reads;return now.load();};malformed=pinned;malformed["recoveryCommitment"]["digest"]=hex(Bytes(32,0));bool mismatch=false;try{auto invalid=start_capella_runtime(*owner,malformed,{},counted_wall);}catch(const std::invalid_argument& e){mismatch=std::string(e.what()).find("commitment differs")!=std::string::npos;}check(mismatch&&clock_reads==0,"mismatched commitment reached clock or passed");
  check(stale_token&&stale_token->sequence<saved.token.sequence,"rollback fixture did not retain an older checkpoint");
  {auto stale_config=pinned;stale_config.erase("validator");stale_config["recoveryWitness"]=(directory/"stale-witness").string();Blockchain stale_owner(directory/"stale-owner",g);mismatch=false;try{auto invalid=start_capella_runtime(stale_owner,stale_config,{},counted_wall);}catch(const std::invalid_argument& e){mismatch=std::string(e.what()).find("commitment differs")!=std::string::npos;}check(mismatch&&clock_reads==0,"older coherent primary/witness accepted newer commitment");check(stale_owner.store().fork_checkpoint(g.chain_id,g.block.header.hash(),limits.envelope_bytes)->token==*stale_token,"rollback rejection changed primary checkpoint");
  stale_config["recoveryCommitment"]=commitment(*stale_token);auto stale_runtime=start_capella_runtime(stale_owner,stale_config,{},[&]{return stale_time;});check(stale_runtime.reactor->current().token==*stale_token,"matching old commitment failed coherent recovery");stop_capella_runtime(stale_runtime);stale_runtime={};}
  auto resumed=start_capella_runtime(*owner,pinned,{},wall);check(resumed.reactor->current().token==saved.token,"pinned recovery changed retained checkpoint");stop_capella_runtime(resumed);}
 std::cout<<Json{{"externalRecoveryCommitmentChecks",automatic_recovery?9:0},{"automaticRecovery",automatic_recovery},{"scenario","capella-configured-runtime"},{"blocks",proposals},{"keysLoaded",automatic_registration?65:64},{"automaticRegistration",automatic_registration},{"witnessRestart",true},{"signerWitnessRestart",true},{"admissionReceiptChecks",6},{"httpTransport",capella_http},{"runtimeCatalogChecks",3},{"catalogSeekBoundaryChecks",8},{"catalogSeekOwnershipAndBoundsChecks",3},{"invalidConfigurationsRejected",6},{"workersStopped",true},{"networkTransport",false}}.dump()<<std::endl;
}
static void verify_beacon_read_admission(CapellaDurableReactor& reactor,const Bytes& wire,const ConsensusRoot& parent){
 if(std::getenv("NANO_TEST_CATALOG_WORKER_STARVATION")){
  CapellaInventorySessions catalogs(reactor,2,65536,std::chrono::seconds(60));const auto gen=catalogs.generation();const auto primary=reactor.current().token;
  SimulationQueue queue(std::chrono::milliseconds(150));boost::asio::io_context io;auto keep=boost::asio::make_work_guard(io);
  std::promise<void> held,waiter,unblock;auto held_ready=held.get_future(),waiter_ready=waiter.get_future();auto gate=unblock.get_future().share();std::atomic<bool> cancel{false},waiter_busy{false};std::atomic<unsigned> drained{0};
  check(queue.reserve(4),"starvation probe reservation failed");auto cleanup=[&]{queue.release(1);if(++drained==4)keep.reset();};
  queue.dispatch(io.get_executor(),[&]()->Json{bool seen=false;try{catalogs.acquire("probe",gen,"0x"+std::string(64,'a'),"block",[&]{if(!seen){seen=true;held.set_value();gate.wait();}throw ExecutionCancelled("held acquisition cancelled");});}catch(const ExecutionCancelled&){}return false;},[](auto){},cleanup);
  const bool capture_started=held_ready.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  queue.dispatch(io.get_executor(),[&]()->Json{bool seen=false;try{catalogs.acquire("probe",gen,"0x"+std::string(64,'b'),"block",[&]{if(!seen){seen=true;waiter.set_value();}if(cancel)throw ExecutionCancelled("waiting acquisition cancelled");});}catch(const ExecutionCancelled&){}catch(const std::length_error&){waiter_busy=true;if(!seen)waiter.set_value();}return false;},[](auto){},cleanup);
  const bool waiter_started=waiter_ready.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  std::promise<bool> generation_result,release_result;auto generation_ready=generation_result.get_future(),release_ready=release_result.get_future();
  queue.dispatch(io.get_executor(),[&]()->Json{return catalogs.generation();},[&](auto result){generation_result.set_value(!result);},cleanup);
  queue.dispatch(io.get_executor(),[&]()->Json{catalogs.release("probe",gen,"0x"+std::string(64,'a'));return true;},[&](auto result){release_result.set_value(!result);},cleanup);
  const bool direct_control=catalogs.generation()==gen;std::thread loop([&]{io.run();});
  const bool gen_delivered=generation_ready.wait_for(std::chrono::seconds(2))==std::future_status::ready;
  const bool rel_delivered=release_ready.wait_for(std::chrono::seconds(2))==std::future_status::ready;
  cancel=true;unblock.set_value();queue.workers.join();loop.join();
  const bool gen_expired=generation_ready.get(),rel_expired=release_ready.get();
  check(capture_started&&waiter_started&&direct_control&&gen_delivered&&rel_delivered&&!gen_expired&&!rel_expired&&waiter_busy&&queue.pending==0&&drained==4&&reactor.current().token==primary,"worker starvation hypothesis not reproduced");
  std::cout<<Json{{"scenario","catalog-worker-failfast-draft"},{"workers",2},{"jobs",4},{"captureHeld",capture_started},{"acquirerRejectedBusy",waiter_busy.load()},{"directRegistryControlResponsive",direct_control},{"queuedGenerationExpired",gen_expired},{"queuedReleaseExpired",rel_expired},{"allReservationsDrained",queue.pending==0},{"primaryUnchanged",true},{"realHttpTransport",false},{"wanCauseProven",false}}.dump()<<std::endl;
 }
 if(std::getenv("NANO_TEST_CATALOG_CAPTURE_UNLOCKED")){
  const auto primary=reactor.current().token;Json cases=Json::array();
  for(const std::string mode:{"release","stop","expiry","cancel"}){
   std::atomic<int> elapsed{0};CapellaInventorySessions catalogs(reactor,2,65536,std::chrono::seconds(1),[&]{return std::chrono::steady_clock::time_point{}+std::chrono::milliseconds(elapsed.load());});
   const auto gen=catalogs.generation(),request="0x"+std::string(64,'a');std::promise<void> held,unblock;auto ready=held.get_future();auto gate=unblock.get_future().share();
   auto acquisition=std::async(std::launch::async,[&]{bool paused=false;try{catalogs.acquire("probe",gen,request,"block",[&]{if(!paused){paused=true;held.set_value();gate.wait();}if(mode=="cancel")throw ExecutionCancelled("capture probe cancelled");});return false;}catch(const ExecutionCancelled&){return mode=="cancel";}catch(const std::invalid_argument&){return mode=="release"||mode=="expiry";}catch(const std::runtime_error&){return mode=="stop";}});
   if(ready.wait_for(std::chrono::seconds(5))!=std::future_status::ready){unblock.set_value();acquisition.get();check(false,"capture not held");}
   auto generation=std::async(std::launch::async,[&]{return catalogs.generation();});const bool readable=generation.wait_for(std::chrono::milliseconds(150))==std::future_status::ready;
   auto control=std::async(std::launch::async,[&]{if(mode=="stop")catalogs.stop();else if(mode=="release")catalogs.release("probe",gen,request);else if(mode=="expiry")elapsed=1000;return true;});
   const bool responsive=control.wait_for(std::chrono::milliseconds(150))==std::future_status::ready;
   unblock.set_value();const bool refused=acquisition.get();control.get();bool generation_same=false;try{generation_same=generation.get()==gen;}catch(const std::runtime_error&){}
   check(readable&&responsive&&generation_same&&refused&&reactor.current().token==primary,"capture control/fencing invariant failed");
   if(mode!="stop"){const auto h=catalogs.open("recovery","block");catalogs.close("recovery",h);}
   cases.push_back({{"mode",mode},{"generationResponsive",readable},{"controlResponsive",responsive},{"lateCaptureRefused",refused},{"primaryUnchanged",true},{"capacityRecovered",mode!="stop"}});
  }
  std::cout<<Json{{"scenario","catalog-capture-outside-control-lock"},{"cases",cases},{"wanCauseProven",false},{"networkActivated",false}}.dump()<<std::endl;
 }
 if(const char* field=std::getenv("NANO_TEST_NETWORK_STATUS_FIELD")){
  const std::string name=field;check(name=="identity"||name=="sync","Invalid status probe field");
  auto read_status=[&]{return name=="identity"?reactor.network_identity():reactor.sync_admission_stats();};
  const auto before=read_status();const auto token=reactor.current().token;
  std::promise<void> held,unblock;auto held_future=held.get_future();auto gate=unblock.get_future().share();
  auto writer=std::async(std::launch::async,[&]{bool paused=false;try{return reactor.block(wire,[&]{if(!paused){paused=true;held.set_value();gate.wait();throw ExecutionCancelled("status probe writer cancelled");}});}catch(const ExecutionCancelled&){return false;}});
  const bool locked=held_future.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  if(!locked){unblock.set_value();writer.get();check(false,"Status probe admission lock not held");}
  auto reader=std::async(std::launch::async,read_status);
  const bool completed=reader.wait_for(std::chrono::seconds(1))==std::future_status::ready;
  unblock.set_value();const auto actual=reader.get();const auto admitted=writer.get();
  std::cout<<Json{{"scenario","network-status-during-held-admission"},{"field",name},{"completedWhileAdmissionHeld",completed},{"cancelledWriterUnpublished",!admitted&&reactor.current().token==token},{"sameStatus",actual==before},{"wanCauseProven",false}}.dump()<<std::endl;
  check(completed&&!admitted&&actual==before&&reactor.current().token==token,"Network status "+name+" waited on held admission");
 }

 // The current head can still be an unsigned bootstrap anchor before this
 // proposal. Select its already archived parent explicitly, preserving the
 // captured publication's canonical/finalized flags during admission.
 const auto path="/eth/v1/beacon/headers/"+hex(Bytes(parent.begin(),parent.end()));
 const auto before=reactor.beacon_read(path);check(before.status==200,"Concurrent Beacon signed parent missing");
 {
  const auto before_state=reactor.beacon_read("/eth/v1/beacon/states/head/finality_checkpoints");const auto token=reactor.current().token;const auto before_head=reactor.relay_head();const auto before_export=reactor.export_message("block",parent,16*1024*1024,token);
  std::promise<void> held,unblock;auto held_future=held.get_future();auto gate=unblock.get_future().share();
  auto blocked_writer=std::async(std::launch::async,[&]{bool paused=false;try{return reactor.block(wire,[&]{if(!paused){paused=true;held.set_value();gate.wait();throw ExecutionCancelled("held writer cancelled");}});}catch(const ExecutionCancelled&){return false;}});
  const bool locked=held_future.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  if(!locked){unblock.set_value();blocked_writer.get();check(false,"Admission lock fixture not reached");}
  auto concurrent=std::async(std::launch::async,[&]{return Json::array({reactor.beacon_read(path).body,reactor.beacon_read("/eth/v1/beacon/states/head/finality_checkpoints").body,reactor.relay_head(),reactor.export_message("block",parent,16*1024*1024,token)});});
  const bool completed=concurrent.wait_for(std::chrono::seconds(5))==std::future_status::ready;
  // Release before all assertions so the old locking path fails without hanging.
  unblock.set_value();const auto bodies=concurrent.get();const auto admitted=blocked_writer.get();
  check(completed&&!admitted&&bodies==Json::array({before.body,before_state.body,before_head,before_export})&&reactor.current().token==token,"Beacon read or retained block export waited on admission or mixed unpublished state");
  std::cout<<Json{{"scenario","beacon-read-during-held-admission"},{"publicationReadCompleted",true},{"archiveReadCompleted",true},{"retainedBlockExportCompleted",true},{"oldPublicationPreserved",true},{"cancelledWriterUnpublished",true}}.dump()<<std::endl;
 }

 std::promise<void> entered,release;auto entered_future=entered.get_future();auto gate=release.get_future().share();
 auto reader=std::async(std::launch::async,[&]{bool paused=false;return reactor.beacon_read(path,[&]{if(!paused){paused=true;entered.set_value();gate.wait();}});});
 const bool captured=entered_future.wait_for(std::chrono::seconds(5))==std::future_status::ready;
 if(!captured){release.set_value();reader.get();check(false,"Concurrent Beacon reader never captured publication");}
 const auto export_token=reactor.current().token;const auto before_export=reactor.export_message("block",parent,16*1024*1024,export_token);
 std::promise<void> export_entered,export_release;auto export_ready=export_entered.get_future();auto export_gate=export_release.get_future().share();
 auto exporter=std::async(std::launch::async,[&]{unsigned checks=0;return reactor.export_message("block",parent,16*1024*1024,export_token,[&]{if(++checks==2){export_entered.set_value();export_gate.wait();}});});
 const bool export_captured=export_ready.wait_for(std::chrono::seconds(5))==std::future_status::ready;
 if(!export_captured){release.set_value();export_release.set_value();reader.get();exporter.get();check(false,"Concurrent block exporter never captured publication");}
 auto writer=std::async(std::launch::async,[&]{return reactor.block(wire);});
 const bool progressed=writer.wait_for(std::chrono::seconds(5))==std::future_status::ready;
 // Always release before checking: a regression must fail, not deadlock cleanup.
 release.set_value();export_release.set_value();const auto response=reader.get();const auto exported=exporter.get();const bool admitted=writer.get();
 check(progressed&&admitted,"Paused Beacon projection or block export blocked block admission");
 check(exported==before_export&&reactor.current().token!=export_token,"Concurrent retained export mixed publications or prevented commit");
 check(response.status==200&&response.body==before.body,"Concurrent Beacon read mixed publications");
 const auto after=reactor.beacon_read("/eth/v1/beacon/headers/head");
 check(after.status==200&&after.body.at("data").at("root")!=before.body.at("data").at("root"),"Concurrent Beacon test did not advance head");
 std::cout<<Json{{"scenario","beacon-read-concurrent-admission"},{"retainedExportCoherentDuringCommit",true},{"admissionWhileExporterPaused",true},{"admissionWhileReaderPaused",true},{"oldPublicationPreserved",true},{"headAdvanced",true}}.dump()<<std::endl;
}
static void verify_beacon_reads(CapellaDurableReactor& reactor,const CapellaForkBlockRules& rules){
 const auto snapshot=reactor.current();const auto& head=capella_fork_head(*snapshot.state,rules);const auto id=consensus_beacon_header_root(head.header);auto text=[](const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));};
 if(beacon_read_checks==0){
  BeaconReadPhase phase=BeaconReadPhase::publication;bool stopped=false;
  try{capella_beacon_read(*snapshot.state,rules,"/eth/v1/beacon/states/head/finality_checkpoints",[&]{if(phase==BeaconReadPhase::state_commitment)throw ExecutionCancelled("private checkpoint detail");},[&](auto next){phase=next;});}catch(const ExecutionCancelled&){stopped=true;}
  check(stopped&&phase==BeaconReadPhase::state_commitment&&reactor.current().token==snapshot.token,"Beacon commitment phase/cancellation lost");
  bool pin=false;try{reactor.beacon_read("/eth/v1/beacon/states/head/finality_checkpoints",[]{throw ExecutionCancelled("private checkpoint detail");});}catch(const BeaconReadCancelled& error){pin=error.phase==BeaconReadPhase::publication&&std::string(error.what()).find("private")==std::string::npos;}
  check(pin&&reactor.current().token==snapshot.token,"Beacon publication phase diagnostic leaked or changed state");
  std::cout<<Json{{"scenario","beacon-read-phase-diagnostics"},{"publicationCancellationQualified",true},{"commitmentCancellationQualified",true},{"checkpointUnchanged",true},{"privateExceptionRedacted",true}}.dump()<<std::endl;
 }
 const auto header=reactor.beacon_read("/eth/v1/beacon/headers/head");const auto hint=reactor.relay_head();check(hint.at("headRoot")==text(id)&&hint.at("headSlot")==head.header.slot,"Relay hint differs from authenticated head");bool hint_cancelled=false;try{reactor.relay_head([]{throw ExecutionCancelled("relay hint cancelled");});}catch(const ExecutionCancelled&){hint_cancelled=true;}check(hint_cancelled&&reactor.current().token==snapshot.token,"Cancelled relay hint changed checkpoint");check(header.status==200&&header.body.at("data").at("root")==text(id)&&header.body.at("data").at("canonical")==true,"Capella Beacon head header differs");
 check(reactor.beacon_read("/eth/v1/beacon/headers/"+std::to_string(head.header.slot)).body==header.body&&reactor.beacon_read("/eth/v1/beacon/headers/"+text(id)).body==header.body,"Capella Beacon header selectors differ");
 const auto full=reactor.beacon_read("/eth/v2/beacon/blocks/head");check(full.status==200&&full.version=="capella"&&full.body.at("version")=="capella"&&full.ssz==*reactor.signed_block(id),"Capella Beacon signed bytes differ");const auto& body=full.body.at("data").at("message").at("body");const auto& payload=body.at("execution_payload");
 check(payload.at("block_hash")==hex(head.execution_block.header.hash())&&payload.at("state_root")==hex(head.execution_state.root())&&payload.at("transactions").size()==head.execution_block.transactions.size(),"Capella Beacon execution projection differs");
 const auto& withdrawals=payload.at("withdrawals");check(withdrawals.size()==head.execution_block.withdrawals.size(),"Beacon withdrawal count differs");for(size_t i=0;i<withdrawals.size();++i){const auto& expected=head.execution_block.withdrawals[i];check(withdrawals[i].at("index")==quantity(expected.at("index")).get_str()&&withdrawals[i].at("validator_index")==quantity(expected.at("validatorIndex")).get_str()&&withdrawals[i].at("amount")==quantity(expected.at("amount")).get_str()&&withdrawals[i].at("address")==expected.at("address"),"Beacon withdrawal fields differ");}
 check(reactor.beacon_read("/eth/v1/beacon/states/head/root").body.at("data").at("root")==text(head.header.state_root),"Beacon state root differs");
 const auto checkpoints=capella_fork_checkpoints(head.state,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state);const auto finality=reactor.beacon_read("/eth/v1/beacon/states/head/finality_checkpoints");check(finality.body.at("data").at("finalized").at("epoch")==std::to_string(checkpoints.realized.finalized.epoch)&&finality.body.at("data").at("finalized").at("root")==text(checkpoints.realized.finalized.root),"Beacon finalized state checkpoint differs");
 const auto state_path="/eth/v1/beacon/states/"+text(head.header.state_root);
 check(reactor.beacon_read(state_path+"/finality_checkpoints").body==finality.body,"Pinned head finality differs");
 check(reactor.beacon_read(state_path+"/root").body.at("data").at("root")==text(head.header.state_root),"Pinned state root differs");
 check(reactor.beacon_read(state_path+"/fork").body==reactor.beacon_read("/eth/v1/beacon/states/head/fork").body,"Pinned fork differs");
 check(reactor.beacon_read("/eth/v1/beacon/states/0x12/finality_checkpoints").status==400,"Short state root accepted");
 check(reactor.beacon_read("/eth/v1/beacon/states/0x"+std::string(64,'g')+"/finality_checkpoints").status==400,"Non-hex state root accepted");
 check(reactor.beacon_read("/eth/v1/beacon/states/0x"+std::string(64,'f')+"/finality_checkpoints").status==404,"Unknown state fell back to head");
 check(reactor.beacon_read("/eth/v1/beacon/states/"+text(id)+"/finality_checkpoints").status==404,"Block root accepted as state root");
 for(const auto& [older_id,older]:snapshot.state->blocks)if(older.header.slot<head.header.slot){
  const auto path="/eth/v1/beacon/states/"+text(older.header.state_root)+"/finality_checkpoints";
  const auto expected=capella_fork_checkpoints(older.state,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state).realized;
  const auto pinned=reactor.beacon_read(path);check(pinned.status==200&&pinned.body.at("data").at("finalized").at("epoch")==std::to_string(expected.finalized.epoch)&&pinned.body.at("data").at("finalized").at("root")==text(expected.finalized.root),"Old state selector followed newer head");
  auto pruned=*snapshot.state;pruned.blocks.erase(older_id);check(capella_beacon_read(pruned,rules,path).status==404,"Pruned state fell back to head");
  auto corrupt=*snapshot.state;corrupt.blocks.at(older_id).state.ssz[0]^=1;bool rejected=false;try{capella_beacon_read(corrupt,rules,path);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Pinned state commitment mismatch accepted");break;
 }
 check(reactor.current().token==snapshot.token,"Pinned reads mutated checkpoint");
 const auto spec=reactor.beacon_read("/eth/v1/config/spec");check(spec.body.at("data").at("MAX_WITHDRAWALS_PER_PAYLOAD")==std::to_string(rules.transition.withdrawals.maximum_per_payload),"Beacon Capella spec differs");
 check(reactor.beacon_read("/eth/v1/beacon/headers/0x12").status==400&&reactor.beacon_read("/eth/v1/beacon/headers/genesis").status==404&&reactor.beacon_read("/eth/v1/node/syncing").status==404,"Beacon unsupported selector behavior");
 bool cancelled=false;try{reactor.beacon_read("/eth/v2/beacon/blocks/head",[]{throw ExecutionCancelled("Beacon read cancelled");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&reactor.current().token==snapshot.token,"Beacon read cancellation changed checkpoint");++beacon_read_checks;
}
#include "support/capella_slashing_reorg.hpp"
static void slot_continuation_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules){
 const auto& parent=capella_fork_head(initial,rules);const auto original=parent.state;
 const auto& t=rules.transition;const CapellaEpochRules epoch{t.consensus.epoch,t.execution,t.maximum_state_bytes};
 const auto& limits=t.consensus.state;const auto target=parent.header.slot+2*limits.slots_per_historical_root+3;
 const auto expected=prepare_capella_slots(original,epoch,limits,target,target-parent.header.slot,100000);
 size_t passed=0;
 for(uint64_t budget:{uint64_t(1),uint64_t(3),t.consensus.epoch.ffg.slots_per_epoch+1}){
  CapellaSlotPreparation progress(original,parent.header,epoch,limits);size_t steps=0;
  while(progress.slot()<target){const auto before=progress.slot();const auto complete=progress.advance(target,budget,100000);check(progress.slot()>before&&progress.slot()-before<=budget,"continuation chunk exceeded bound");check(complete==(progress.slot()==target),"continuation reported premature readiness");++steps;}
  check(progress.state()==expected&&progress.parent()==parent.header,"chunked slot transition differs from monolithic oracle");
  check(steps==(target-parent.header.slot+budget-1)/budget,"continuation chunk count mismatch");
  const auto done=progress.state();check(progress.advance(target,budget,100000)&&progress.state()==done,"completed continuation replayed state");++passed;
 }
 CapellaSlotPreparation progress(original,parent.header,epoch,limits);progress.advance(target,3,100000);const auto prefix=progress.state();
 bool cancelled=false;try{size_t calls=0;progress.advance(target,3,100000,[&]{if(++calls==12)throw std::runtime_error("injected preparation cancellation");});}catch(const std::runtime_error&){cancelled=true;}
 check(cancelled&&progress.state()==prefix,"cancelled chunk changed committed prefix");++passed;
 while(!progress.advance(target,3,100000)){}check(progress.state()==expected,"cancelled continuation did not resume exactly");++passed;
 for(unsigned mode=0;mode<3;++mode){bool refused=false;const auto before=progress.state();try{progress.advance(mode==0?target-1:target,mode==1?0:3,mode==2?0:100000);}catch(const std::invalid_argument&){refused=true;}check(refused&&progress.state()==before,"invalid continuation bounds mutated state");++passed;}
 auto bad=parent.header;bad.state_root[0]^=1;bool refused=false;try{CapellaSlotPreparation forged(original,bad,epoch,limits);}catch(const std::invalid_argument&){refused=true;}check(refused,"continuation accepted unauthenticated parent");++passed;
 refused=false;try{CapellaSlotPreparation forged(expected,parent.header,epoch,limits);}catch(const std::invalid_argument&){refused=true;}check(refused,"advanced state masqueraded as original signed block state");++passed;
 check(parent.state==original&&capella_fork_block_context(parent.state,parent.header,epoch,limits).block_root==consensus_beacon_header_root(parent.header),"continuation mutated admitted parent");++passed;
 const auto oracle_proposer=prepare_capella_proposer_identity(original,parent.header,target,epoch,limits,t.consensus.attestation.duties,target-parent.header.slot,100000);
 check(progress.proposer_identity(t.consensus.attestation.duties)==oracle_proposer,"prepared proposer/domain differs from original-parent oracle");++passed;
 const auto before_duties=progress.state();auto wrong_duties=t.consensus.attestation.duties;++wrong_duties.slots_per_epoch;
 refused=false;try{progress.proposer_identity(wrong_duties);}catch(const std::invalid_argument&){refused=true;}check(refused&&progress.state()==before_duties,"prepared proposer accepted incompatible epoch size");++passed;
 refused=false;try{progress.proposer_identity(t.consensus.attestation.duties,[]{throw std::runtime_error("cancel duty projection");});}catch(const std::runtime_error&){refused=true;}check(refused&&progress.state()==before_duties,"cancelled duty projection changed state");++passed;
 const auto boundary_slot=((target/t.consensus.epoch.ffg.slots_per_epoch)+1)*t.consensus.epoch.ffg.slots_per_epoch;
 const ConsensusCheckpoint checkpoint{boundary_slot/t.consensus.epoch.ffg.slots_per_epoch,consensus_beacon_header_root(parent.header)};
 while(!progress.advance(boundary_slot,3,100000)){}
 const auto context=progress.vote_context(checkpoint);
 const auto context_oracle=prepare_capella_fork_state_contexts(original,parent.header,checkpoint,epoch,limits,boundary_slot-parent.header.slot,100000).target;
 check(context.checkpoint==context_oracle.checkpoint&&context.registry==context_oracle.registry&&context.randao==context_oracle.randao&&context.fork.previous_version==context_oracle.fork.previous_version&&context.fork.current_version==context_oracle.fork.current_version&&context.fork.epoch==context_oracle.fork.epoch&&context.genesis_root==context_oracle.genesis_root,"prepared vote context differs from original-parent oracle");++passed;
 for(unsigned mode=0;mode<3;++mode){auto invalid=checkpoint;if(mode==0)invalid.root[0]^=1;else if(mode==1)--invalid.epoch;else invalid.epoch=UINT64_MAX;const auto before=progress.state();refused=false;try{progress.vote_context(invalid);}catch(const std::invalid_argument&){refused=true;}check(refused&&progress.state()==before,"prepared vote context accepted wrong root/boundary or overflow");++passed;}
 CapellaSlotPreparation at_parent(original,parent.header,epoch,limits);check(!at_parent.proposer_identity(t.consensus.attestation.duties),"prepared parent itself produced a new proposal identity");++passed;
 std::cout<<Json{{"scenario","capella-authenticated-slot-continuation"},{"scenarios",passed},{"crossedHistoryWindows",2},{"canonicalOracleEqual",true},{"parentPreserved",true},{"runtimeIntegrated",false},{"networkQualified",false}}.dump()<<std::endl;
}
static void prepared_block_loop(const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const Block& genesis,const State& genesis_state){
 const auto& parent=capella_fork_head(initial,rules);const auto& r=rules.transition;const auto& l=r.consensus.state;
 const CapellaEpochRules epoch_rules{r.consensus.epoch,r.execution,r.maximum_state_bytes};const auto slots=r.consensus.epoch.ffg.slots_per_epoch;
 const size_t hist=176+64*l.slots_per_historical_root,randao=hist+96,field=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;
 const auto base=parent.header.slot+2*l.slots_per_historical_root;size_t checks=0;
 auto history=[&](const Int& n){return capella_execution_history(initial,consensus_beacon_header_root(parent.header),n);};
 for(uint64_t slot:{base,((base/slots)+1)*slots}){
  CapellaBlockPreparation preparation(parent.state,parent.header,r);const auto original=parent.state;bool refused=false;
  try{preparation.proposer_for_block(slot,100000);}catch(const std::invalid_argument&){refused=true;}check(refused,"unfinished block preparation produced proposer");++checks;
  while(!preparation.advance_for_block(slot,3,100000)){}
  check(preparation.prepared_slot()==slot-1,"block preparation skipped final slot");++checks;
  const auto before=preparation.prepared_state();const auto identity=preparation.proposer_for_block(slot,100000);check(bool(identity),"prepared block proposer missing");
  check(identity==prepare_capella_proposer_identity(original,parent.header,slot,epoch_rules,l,r.consensus.attestation.duties,slot-parent.header.slot,100000),"prepared block proposer differs from original-parent oracle");++checks;
  const auto advanced=prepare_capella_slots(before,epoch_rules,l,slot,1,100000);const auto& b=advanced.ssz;const auto epoch=slot/slots;
  ConsensusRoot epoch_root{};for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
  Key key(identity->index);const auto reveal=key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},identity->fork_version,identity->genesis_root)));
  Block execution;execution.header.parent_hash=parent.execution_block.header.hash();execution.header.number=parent.execution_block.header.number+1;execution.header.timestamp=Int(std::to_string(little(b,0,8)+slot*r.seconds_per_slot));execution.header.difficulty=0;execution.header.base_fee=next_base_fee(parent.execution_block.header);const auto mix=root_at(b,randao+32*(epoch%l.epochs_per_historical_vector));execution.header.mix_hash=Bytes(mix.begin(),mix.end());
  const auto user_key=bytes(Int(777),32);Transaction tx;tx.type=2;tx.to="0x0000000000000000000000000000000000001234";tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;tx.nonce=parent.execution_state.get(SignedTransaction::sign(tx,user_key).sender()).nonce;execution.transactions.push_back(SignedTransaction::sign(tx,user_key));
  CapellaWithdrawalState withdrawals;withdrawals.registry.epoch=epoch;withdrawals.next_withdrawal_index=little(b,field+4,8);withdrawals.next_validator_index=little(b,field+12,8);const auto validators=little(b,hist+88,4),balances=little(b,hist+92,4);
  for(size_t i=0;i<(balances-validators)/121;++i){ConsensusSlashingValidator v;const auto at=validators+121*i;v.balance=little(b,balances+8*i,8);v.effective_balance=little(b,at+80,8);v.withdrawable_epoch=little(b,at+113,8);std::copy_n(b.begin()+at+48,32,v.withdrawal_credentials.begin());withdrawals.registry.validators.emplace(i,v);}
  const auto payments=expected_capella_withdrawals(withdrawals,r.withdrawals);for(const auto& w:payments)execution.withdrawals.push_back(w.to_json());
  State expected=parent.execution_state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(expected,execution,false);
  auto ep=payload(execution);ep.withdrawals=payments;const auto encoded=encode_capella_execution_payload(ep,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);
  const size_t sync_bytes=(l.sync_committee_size+7)/8+96,fixed=228+sync_bytes;Bytes body(fixed,0);std::copy(reveal.begin(),reveal.end(),body.begin());std::copy_n(b.begin()+hist+4,72,body.begin()+96);body[220+(l.sync_committee_size+7)/8]=0xc0;
  for(size_t at:std::array<size_t,6>{200,204,208,212,216,220+sync_bytes})put(body,at,fixed,4);put(body,224+sync_bytes,fixed+encoded.size(),4);body.insert(body.end(),encoded.begin(),encoded.end());
  const auto constructed=preparation.construct(slot,identity->index,body,parent.execution_block.header,parent.execution_state,history,100000);check(bool(constructed),"prepared unsigned block rejected");
  const auto oracle=prepare_capella_unsigned_block(original,slot,identity->index,consensus_beacon_header_root(parent.header),body,r,parent.execution_block.header,parent.execution_state,history,slot-parent.header.slot,100000);
  check(oracle&&constructed->message_ssz==oracle->message_ssz&&constructed->post.consensus==oracle->post.consensus&&constructed->post.execution.block.header.hash()==oracle->post.execution.block.header.hash()&&constructed->post.execution.state.root()==expected.root(),"prepared block differs from full-gap execution oracle");++checks;
  auto sign=[&](const Bytes& message,uint64_t signer){const auto view=std::span<const uint8_t>(message);const ConsensusBeaconHeader h{little(message,0,8),little(message,8,8),root_at(message,16),root_at(message,48),capella_beacon_body_root(view.subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)};const auto signature=Key(signer).sign(consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},identity->fork_version,identity->genesis_root)));Bytes wire(100,0);put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());return wire;};
  const auto wire=sign(constructed->message_ssz,identity->index);const auto received=preparation.receive(wire,parent.execution_block.header,parent.execution_state,history,100000);check(received&&received->consensus==oracle->post.consensus&&received->execution.block.header.hash()==oracle->post.execution.block.header.hash()&&received->execution.state.root()==oracle->post.execution.state.root(),"prepared signed block transition differs from oracle");++checks;
  if(atomic_proposal||proposal_gap){
   Temp directory;Genesis g;g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
   auto tight=crash_limits;tight.blocks=initial.blocks.size()+1;
   Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");
   auto reactor=std::make_unique<CapellaDurableReactor>(owner,rules,tight,initial,std::function<void()>{},&witness);
   const auto clock=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;
   reactor->tick(clock);size_t attempts=0;while(!reactor->prepare_current_duties(slot))check(++attempts<slot/slots+4,"atomic fixture preparation stalled");
   ChainStore signer_disk(directory/"signer");BeaconSigningJournal journal(signer_disk,r.chain_id,genesis.header.hash(),identity->genesis_root,key.public_key,slots);
   std::promise<void> contender_started;auto started=contender_started.get_future();std::future<bool> contender;unsigned providers=0;
   auto provider=[&](const ConsensusRoot& root){
    if(++providers==1){contender=std::async(std::launch::async,[&]{contender_started.set_value();return reactor->block(wire);});
     check(started.wait_for(std::chrono::seconds(5))==std::future_status::ready,"concurrent admission thread did not start");
     check(contender.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready,"peer mutated Store during local signing lock");}
    return key.sign(root);
   };
   const auto before=reactor->current().token;
   const auto local=proposal_gap?reactor->propose_current(identity->index,slot,"0x0000000000000000000000000000000000000001",4,4,[&]{return clock;},journal,provider):reactor->propose_and_admit_current(identity->index,slot,"0x0000000000000000000000000000000000000001",4,4,[&]{return clock;},journal,provider);
   check(local&&*local!=wire&&providers==2&&journal.latest_proposal_slot()==slot,"local proposal fixture did not sign a distinct valid child");
   const bool incoming=contender.get();
   if(proposal_gap){check(incoming,"two-step competitor did not fill capacity");check(reactor->block(*local),"two-step proposal lost admission capacity");}
   check(!incoming&&reactor->current().state->blocks.size()==tight.blocks&&reactor->current().token.sequence==before.sequence+1,"concurrent peer displaced atomic local publication");
   check(reactor->block(*local),"atomic proposal duplicate not admitted at capacity");
   const auto committed=reactor->current().token,guard=journal.commitment();const auto tip=owner.tip().header.hash();
   reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(owner,rules,tight,witness);
   check(reactor->current().token==committed&&journal.commitment()==guard&&owner.tip().header.hash()==tip&&reactor->block(*local),"atomic publication witness restore lost block or signing guard");
   std::cout<<Json{{"scenario","capella-atomic-proposal-admission"},{"providerCalls",providers},{"contenderAttemptedDuringProvider",true},{"contenderRefusedAtCapacity",!incoming},{"localDurablyAdmitted",true},{"duplicateAdmitted",true},{"witnessRestored",true},{"networkQualified",false}}.dump()<<std::endl;return;
  }
  if(capacity_pressure){
   Temp directory;Genesis g;g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
   auto tight=crash_limits;tight.blocks=initial.blocks.size();
   Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");
   auto reactor=std::make_unique<CapellaDurableReactor>(owner,rules,tight,initial,std::function<void()>{},&witness);
   const auto clock=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;
   reactor->tick(clock);size_t attempts=0;while(!reactor->prepare_current_duties(slot))check(++attempts<slot/slots+4,"capacity fixture preparation stalled");
   ChainStore signer_disk(directory/"signer");BeaconSigningJournal journal(signer_disk,r.chain_id,genesis.header.hash(),identity->genesis_root,key.public_key,slots);
   unsigned providers=0;auto provider=[&](const ConsensusRoot& root){++providers;return key.sign(root);};
   const auto before=reactor->current().token;const auto guard=journal.commitment();const auto tip=owner.tip().header.hash();
   check(!reactor->propose_current(identity->index,slot,"0x0000000000000000000000000000000000000000",4,4,[&]{return clock;},journal,provider),"full graph invoked automatic proposal signer");
   check(!reactor->sign_proposal(identity->index,slot,body,[&]{return clock;},before,journal,provider),"full graph invoked explicit proposal signer");
   check(!reactor->propose_and_admit_current(identity->index,slot,"0x0000000000000000000000000000000000000000",4,4,[&]{return clock;},journal,provider),"full graph invoked atomic proposal signer");
   check(providers==0&&journal.commitment()==guard&&!journal.latest_proposal_slot(),"capacity refusal changed signer reservation");
   for(unsigned retry=0;retry<3;++retry)check(!reactor->block(wire)&&reactor->current().token==before&&owner.tip().header.hash()==tip,"capacity refusal changed durable Store or execution");
   // The signed wire is valid: an independently provisioned receiver admits it.
   Blockchain peer_owner(directory/"peer",g);ChainStore peer_witness(directory/"peer-witness");CapellaDurableReactor peer(peer_owner,rules,crash_limits,initial,{},&peer_witness);peer.tick(clock);
   attempts=0;while(!peer.block(wire))check(++attempts<slot/slots+4,"independent valid block admission stalled");
   reactor->tick(clock+rules.slot_duration_ms);const auto stopped=reactor->current().token;check(stopped.sequence>before.sequence&&reactor->current().state->blocks.size()==tight.blocks,"full Store stopped clock or silently grew");
   reactor.reset();reactor=std::make_unique<CapellaDurableReactor>(owner,rules,tight,witness);check(reactor->current().token==stopped&&journal.commitment()==guard,"full Store witness restore or signer guard changed");
   std::cout<<Json{{"scenario","capella-block-capacity-pressure"},{"providerCalls",providers},{"journalUnchanged",true},{"incomingRefusals",3},{"independentValidAdmission",true},{"clockAdvanced",true},{"witnessRestored",true},{"networkQualified",false},{"capacityRecoveryQualified",false}}.dump()<<std::endl;return;
  }
  for(unsigned mode=0;mode<5;++mode){auto invalid=wire;
   if(mode==0)invalid[4]^=1;
   else {auto message=constructed->message_ssz;if(mode==1)message[16]^=1;else if(mode==2)message[48]^=1;else if(mode==3)message[84]^=1;else put(message,0,slot+1);invalid=sign(message,identity->index);}
   check(!preparation.receive(invalid,parent.execution_block.header,parent.execution_state,history,100000),"prepared receive accepted invalid signature/parent/state-root/RANDAO/slot");++checks;
  }
  refused=false;try{prepare_capella_unsigned_block(original,slot,identity->index,consensus_beacon_header_root(parent.header),body,r,parent.execution_block.header,parent.execution_state,history,3,100000);}catch(const std::length_error&){refused=true;}check(refused,"legacy full-gap work budget was weakened");++checks;
  refused=false;try{preparation.construct(slot+1,identity->index,body,parent.execution_block.header,parent.execution_state,history,100000);}catch(const std::invalid_argument&){refused=true;}check(refused,"unfinished next block constructed");++checks;
  refused=false;try{preparation.receive(wire,parent.execution_block.header,parent.execution_state,history,100000,[]{throw ExecutionCancelled("cancel prepared receive");});}catch(const ExecutionCancelled&){refused=true;}check(refused,"prepared receive cancellation swallowed");++checks;
  check(preparation.prepared_state()==before&&parent.state==original,"block preparation or transition mutated its source");++checks;
  const auto clock=initial.timed.clock.genesis_time_ms+slot*rules.slot_duration_ms;
  const auto timed=prepare_capella_store_tick(initial,clock,rules,3,100000);
  const auto admitted=prepare_capella_prepared_received_block(timed,wire,rules,preparation,3,100000);
  const auto admitted_oracle=prepare_capella_received_block(timed,wire,rules,slot-parent.header.slot,100000);
  {
   const auto expected=prepare_capella_fork_block(timed,wire,rules,slot-parent.header.slot,100000);
   const auto replayed=prepare_capella_resumed_fork_block(timed,wire,rules,3,100000);
   check(expected&&replayed&&capella_store_snapshot(*expected,rules,crash_limits)==capella_store_snapshot(*replayed,rules,crash_limits),"resumed archive on_block differs from full-gap oracle");++checks;
   bool limited=false;try{prepare_capella_fork_block(timed,wire,rules,3,100000);}catch(const std::length_error&){limited=true;}check(limited,"legacy archive block budget weakened");++checks;
   auto invalid=wire;invalid[4]^=1;check(!prepare_capella_resumed_fork_block(timed,invalid,rules,3,100000),"resumed archive accepted invalid BLS");++checks;
   bool cancelled=false;try{size_t callbacks=0;prepare_capella_resumed_fork_block(timed,wire,rules,3,100000,[&]{if(++callbacks==12)throw ExecutionCancelled("cancel resumed archive");});}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&timed.blocks.size()==initial.blocks.size()&&parent.state==original,"resumed archive cancellation swallowed or changed source");++checks;
   bool zero=false;try{prepare_capella_resumed_fork_block(timed,wire,rules,0,100000);}catch(const std::invalid_argument&){zero=true;}check(zero,"resumed archive accepted zero slot budget");++checks;
  }
  check(admitted&&admitted_oracle&&capella_store_snapshot(*admitted,rules,crash_limits)==capella_store_snapshot(*admitted_oracle,rules,crash_limits),"prepared Store admission differs from ordinary fork-choice/operation processing");++checks;
  {
   const auto prior_time=initial.timed.clock.genesis_time_ms+(slot-1)*rules.slot_duration_ms+rules.attestation_due_ms;
   const auto prior_store=prepare_capella_store_tick(initial,prior_time,rules,3,100000);
   const auto assignments=prepare_capella_attestation_duties(prior_store,rules,slot-1,slot-parent.header.slot,100000);
   check(!assignments.empty(),"included long-gap vote has no assignment");const auto& assignment=assignments.begin()->second;
   auto vote=assignment.attestation;vote.signature=Key(assignment.validator_index).sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},assignment.fork_version,assignment.genesis_root)));
   const auto vote_bytes=encode_consensus_attestation_ssz(vote,r.consensus.body.validators_per_committee);
   Bytes included_body(body.begin(),body.begin()+fixed);Bytes list(4,0);put(list,0,4,4);list.insert(list.end(),vote_bytes.begin(),vote_bytes.end());
   for(size_t at:std::array<size_t,3>{212,216,220+sync_bytes})put(included_body,at,fixed+list.size(),4);
   put(included_body,224+sync_bytes,fixed+list.size()+encoded.size(),4);included_body.insert(included_body.end(),list.begin(),list.end());included_body.insert(included_body.end(),encoded.begin(),encoded.end());
   const auto block_with_vote=preparation.construct(slot,identity->index,included_body,parent.execution_block.header,parent.execution_state,history,100000);
   check(bool(block_with_vote),"signed included vote failed normal block transition");++checks;
   const auto included_wire=sign(block_with_vote->message_ssz,identity->index);
   CapellaTargetPreparation included_target(timed,rules,vote.data.target);size_t chunks=0;while(!included_target.advance(3,100000))check(++chunks<slot/3+3,"included target preparation stalled");
   const std::array<const CapellaTargetPreparation*,1> targets{&included_target};
   const auto expected=prepare_capella_received_block(timed,included_wire,rules,slot-parent.header.slot,100000);
   const auto actual=prepare_capella_contextual_received_block(timed,included_wire,rules,preparation,targets,3,100000);
   check(expected&&actual&&capella_store_snapshot(*actual,rules,crash_limits)==capella_store_snapshot(*expected,rules,crash_limits)&&!actual->timed.view.latest_messages.empty(),"included prepared vote differs from full-gap Store oracle");++checks;
   bool bounded=false;try{prepare_capella_prepared_received_block(timed,included_wire,rules,preparation,3,100000);}catch(const std::length_error&){bounded=true;}check(bounded,"included vote silently bypassed legacy replay budget");++checks;
   CapellaTargetPreparation unfinished(timed,rules,vote.data.target);const std::array<const CapellaTargetPreparation*,1> missing{&unfinished};
   bool incomplete=false;try{prepare_capella_contextual_received_block(timed,included_wire,rules,preparation,missing,3,100000);}catch(const std::invalid_argument&){incomplete=true;}check(incomplete,"included vote used incomplete target");++checks;
   auto invalid=included_wire;invalid[4]^=1;check(!prepare_capella_contextual_received_block(timed,invalid,rules,preparation,targets,3,100000),"contextual block bypassed BLS signature");++checks;
   Temp worker_directory;Genesis wg;wg.chain_id=r.chain_id;wg.consensus="beacon";wg.evm_revision=EvmRevision::Shanghai;wg.block=genesis;wg.state=genesis_state;
   auto worker_limits=crash_limits;worker_limits.replay_slots=3;
   if(vote_target_alternation){
    auto other_body=body;other_body[168]^=1;
    const auto other_block=preparation.construct(slot,identity->index,other_body,parent.execution_block.header,parent.execution_state,history,100000);
    check(bool(other_block),"second sibling construction failed");const auto other_wire=sign(other_block->message_ssz,identity->index);
    auto both=prepare_capella_received_block(*admitted,other_wire,rules,slot-parent.header.slot,100000);check(bool(both),"both individually valid siblings were not admitted");
    ConsensusRoot added_first{},added_second{};
    for(const auto& [root,record]:admitted->blocks)if(!initial.blocks.contains(root))added_first=root;
    for(const auto& [root,record]:both->blocks)if(!admitted->blocks.contains(root))added_second=root;
    check(added_first!=added_second,"sibling roots coincide");
    const auto vote_slot=slot+2*l.slots_per_historical_root+3;
    const auto vote_clock=initial.timed.clock.genesis_time_ms+vote_slot*rules.slot_duration_ms+rules.attestation_due_ms;
    const auto common=prepare_capella_store_tick(*both,vote_clock,rules,vote_slot,100000);
    auto branch1=common,branch2=common;
    branch1.blocks.erase(added_second);branch1.timed.view.nodes.erase(added_second);branch1.timely.erase(added_second);branch1.unrealized_justifications.erase(added_second);
    branch2.blocks.erase(added_first);branch2.timed.view.nodes.erase(added_first);branch2.timely.erase(added_first);branch2.unrealized_justifications.erase(added_first);
    branch1.timed.view.proposer_boost.reset();branch2.timed.view.proposer_boost.reset();
    const auto duties1=prepare_capella_attestation_duties(branch1,rules,vote_slot,vote_slot,100000),duties2=prepare_capella_attestation_duties(branch2,rules,vote_slot,vote_slot,100000);
    check(!duties1.empty()&&!duties2.empty(),"alternate branches have no vote duties");
    auto assignment1=duties1.begin()->second,assignment2=duties2.begin()->second;
    auto sign_vote=[](const auto& assignment){auto result=assignment.attestation;result.signature=Key(assignment.validator_index).sign(consensus_signing_root(consensus_attestation_root(result.data),consensus_domain({1,0,0,0},assignment.fork_version,assignment.genesis_root)));return result;};
    vote=sign_vote(assignment1);auto other=sign_vote(assignment2);
    check(other.data.target.epoch==vote.data.target.epoch&&other.data.target.root!=vote.data.target.root,"vote targets are not distinct in the same epoch");
    const auto deferred=prepare_capella_store_tick(common,vote_clock+rules.slot_duration_ms,rules,vote_slot,100000);
    const auto oracle1=prepare_capella_store_vote(deferred,vote,rules,vote_slot,100000),oracle2=prepare_capella_store_vote(deferred,other,rules,vote_slot,100000);
    check(oracle1&&oracle2,"alternating vote fixture not individually valid on common Store");
    Blockchain alternating_owner(worker_directory/"alternating-owner",wg);ChainStore alternating_witness(worker_directory/"alternating-witness");
    auto alternating=std::make_unique<CapellaDurableReactor>(alternating_owner,rules,worker_limits,*both,std::function<void()>{},&alternating_witness);alternating->tick(vote_clock+rules.slot_duration_ms);
    const auto before=alternating->current().token;bool accepted=false,other_accepted=false;size_t attempts=0;const auto maximum=2*vote_slot/3+12;
    while(!accepted&&attempts<maximum){++attempts;for(const auto* input:{&other,&vote}){
     const auto token=alternating->current().token;const bool admitted=alternating->vote(*input);
     if(!admitted)check(alternating->current().token==token,"incomplete vote preparation published state");
     if(input==&vote)accepted=admitted;else other_accepted|=admitted;
    }}
    check(accepted,"Alternating valid votes erased target continuation");
    check(alternating->current().token.sequence>before.sequence,"admitted vote did not publish checkpoint");
    const auto current=alternating->current();auto expected=oracle1;
    if(other_accepted)expected=prepare_capella_store_vote(*oracle2,vote,rules,vote_slot,100000);
    check(expected&&capella_store_snapshot(*current.state,rules,worker_limits)==capella_store_snapshot(*expected,rules,worker_limits),"alternating votes differ from full-gap oracle");
    const auto signed_votes=alternating->signed_votes();const auto first_bytes=encode_consensus_attestation_ssz(vote,r.consensus.body.validators_per_committee),second_bytes=encode_consensus_attestation_ssz(other,r.consensus.body.validators_per_committee);
    check(signed_votes.at(sha256(first_bytes))==first_bytes&&(!other_accepted||signed_votes.at(sha256(second_bytes))==second_bytes),"alternating signed votes lost exact bytes");
    auto invalid=other;invalid.signature[0]^=1;check(!alternating->vote(invalid)&&alternating->current().token==current.token,"prepared vote context bypassed invalid signature");
    check(alternating->vote(vote)&&alternating->current().token==current.token,"duplicate vote rewrote checkpoint");
    alternating.reset();CapellaDurableReactor reopened(alternating_owner,rules,worker_limits,alternating_witness);
    check(reopened.current().token==current.token&&reopened.signed_votes()==signed_votes,"alternating witness reopen lost votes or checkpoint");
    std::cout<<Json{{"scenario","capella-vote-target-alternation"},{"attemptPairs",attempts},{"individuallyValidVotes",true},{"sameEpochDistinctTargets",true},{"honestVoteProgress",true},{"otherVoteProgress",other_accepted},{"fullGapOracleEqual",true},{"invalidSignatureRejected",true},{"duplicateCheckpointStable",true},{"witnessReopened",true},{"networkQualified",false}}.dump()<<std::endl;return;
   }
   if(incoming_parent_interference){
    auto sibling_body=included_body;sibling_body[168]^=1;
    const auto sibling=preparation.construct(slot,identity->index,sibling_body,parent.execution_block.header,parent.execution_state,history,100000);
    check(bool(sibling),"parent interference sibling construction failed");const auto sibling_wire=sign(sibling->message_ssz,identity->index);
    const auto seeded=prepare_capella_received_block(timed,sibling_wire,rules,slot-parent.header.slot,100000);
    check(bool(seeded),"parent interference seed is not valid");
    const auto sibling_root=consensus_beacon_header_root(capella_fork_head(*seeded,rules).header);
    auto interleaved=prepare_capella_store_tick(*seeded,clock+rules.slot_duration_ms,rules,slot-parent.header.slot+1,100000);
    auto noise=sibling_wire;put(noise,100,slot+1,8);std::copy(sibling_root.begin(),sibling_root.end(),noise.begin()+116);noise[352]^=1;
    check(!prepare_capella_received_block(interleaved,noise,rules,slot-parent.header.slot+1,100000),"parent interference noise unexpectedly valid");
    const auto oracle=prepare_capella_received_block(interleaved,included_wire,rules,slot-parent.header.slot+1,100000);
    check(bool(oracle),"parent interference honest block not valid");
    Blockchain owner(worker_directory/"parent-interference-owner",wg);ChainStore witness(worker_directory/"parent-interference-witness");
    auto worker=std::make_unique<CapellaDurableReactor>(owner,rules,worker_limits,interleaved,std::function<void()>{},&witness);
    const auto before=worker->current().token;bool accepted=false;size_t attempts=0;const auto maximum=2*(slot-parent.header.slot)/3+12;
    while(!accepted&&attempts<maximum){++attempts;check(!worker->block(noise),"parent interference admitted invalid block");const auto prior=worker->current().token;accepted=worker->block(included_wire);if(!accepted)check(worker->current().token==prior,"parent interference published incomplete work");}
    check(accepted,"Alternating parents erased incoming block continuation");const auto current=worker->current();
    const auto honest_root=consensus_beacon_header_root({little(included_wire,100,8),little(included_wire,108,8),root_at(included_wire,116),root_at(included_wire,148),capella_beacon_body_root(std::span(included_wire).subspan(184),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)});
    check(current.token.sequence>before.sequence&&current.state->blocks.at(honest_root).state==oracle->blocks.at(honest_root).state&&current.state->blocks.at(honest_root).execution_state.root()==oracle->blocks.at(honest_root).execution_state.root(),"parent interference differs from full oracle");
    check(worker->signed_block(honest_root)==std::optional<Bytes>(included_wire),"parent interference signed archive differs");
    check(worker->block(included_wire)&&worker->current().token==current.token,"parent interference duplicate rewrote checkpoint");
    worker.reset();CapellaDurableReactor reopened(owner,rules,worker_limits,witness);
    check(reopened.current().token==current.token&&reopened.signed_block(honest_root)==std::optional<Bytes>(included_wire),"parent interference witness reopen lost archive");
    std::cout<<Json{{"scenario","capella-incoming-parent-interference"},{"attemptPairs",attempts},{"maximumPairs",maximum},{"honestBlockProgress",true},{"invalidAlternateParentRejected",true},{"fullGapOracleEqual",true},{"duplicateCheckpointStable",true},{"witnessReopened",true},{"networkQualified",false}}.dump()<<std::endl;return;
   }
   if(incoming_alternation){
    // A slashable proposer sends two individually valid siblings. Alternating
    // retries must not erase authenticated target work needed by the honest one.
    auto competing_body=included_body;competing_body[168]^=1;
    const auto competing=preparation.construct(slot,identity->index,competing_body,parent.execution_block.header,parent.execution_state,history,100000);
    check(bool(competing),"alternating sibling construction failed");const auto competing_wire=sign(competing->message_ssz,identity->index);
    check(competing_wire!=included_wire&&prepare_capella_received_block(timed,competing_wire,rules,slot-parent.header.slot,100000).has_value(),"competing fixture is not an individually valid distinct block");
    Blockchain alternating_owner(worker_directory/"alternating-owner",wg);ChainStore alternating_witness(worker_directory/"alternating-witness");
    auto alternating=std::make_unique<CapellaDurableReactor>(alternating_owner,rules,worker_limits,initial,std::function<void()>{},&alternating_witness);alternating->tick(clock);
    const auto before=alternating->current().token;bool accepted=false,sibling_accepted=false;size_t attempts=0;
    const auto maximum=2*(slot-parent.header.slot)/3+12;
    while(!accepted&&attempts<maximum){
     ++attempts;for(const auto* input:{&competing_wire,&included_wire}){
      const auto token=alternating->current().token;const bool admitted=alternating->block(*input);
      if(!admitted)check(alternating->current().token==token,"incomplete alternating preparation published state");
      if(input==&included_wire)accepted=admitted;else sibling_accepted|=admitted;
     }
    }
    check(accepted,"Alternating valid blocks erased incoming target continuation");
    const auto honest_root=consensus_beacon_header_root(capella_fork_head(*expected,rules).header);const auto current=alternating->current();
    check(current.token.sequence>before.sequence&&current.state->blocks.at(honest_root).state==expected->blocks.at(honest_root).state&&current.state->blocks.at(honest_root).execution_state.root()==expected->blocks.at(honest_root).execution_state.root(),"alternating block differs from full-gap oracle");
    check(alternating->signed_block(honest_root)==std::optional<Bytes>(included_wire),"alternating block signed archive differs");
    auto invalid=competing_wire;invalid[4]^=1;check(!alternating->block(invalid)&&alternating->current().token==current.token,"target cache bypassed invalid signature");
    check(alternating->block(included_wire)&&alternating->current().token==current.token,"alternating duplicate rewrote checkpoint");
    alternating.reset();CapellaDurableReactor reopened(alternating_owner,rules,worker_limits,alternating_witness);
    check(reopened.current().token==current.token&&reopened.signed_block(honest_root)==std::optional<Bytes>(included_wire),"alternating witness reopen lost commitment/archive");
    std::cout<<Json{{"scenario","capella-incoming-target-alternation"},{"attemptPairs",attempts},{"maximumPairs",maximum},{"individuallyValidSiblings",true},{"siblingAdmitted",sibling_accepted},{"honestBlockProgress",true},{"fullGapOracleEqual",true},{"invalidSignatureRejected",true},{"witnessReopened",true},{"networkQualified",false}}.dump()<<std::endl;return;
   }
   {
    Blockchain warm_owner(worker_directory/"warm-owner",wg);ChainStore warm_witness(worker_directory/"warm-witness");CapellaDurableReactor warmed(warm_owner,rules,worker_limits,initial,{},&warm_witness);warmed.tick(clock);
    size_t attempts=0;while(!warmed.vote(vote))check(++attempts<slot/3+4,"prewarmed included target stalled");
    attempts=0;while(!warmed.prepare_current_duties(slot))check(++attempts<slot/3+4,"prewarmed local block stalled");
    const auto before=warmed.current();const auto oracle=prepare_capella_received_block(*before.state,included_wire,rules,slot-parent.header.slot,100000);
    check(warmed.block(included_wire)&&warmed.current().token.sequence==before.token.sequence+1,"ready included target unnecessarily deferred local admission");++checks;
    check(oracle&&capella_store_snapshot(*warmed.current().state,rules,worker_limits)==capella_store_snapshot(*oracle,rules,worker_limits),"ready-cache admission differs from full-gap oracle");++checks;
   }
   Blockchain worker_owner(worker_directory/"owner",wg);ChainStore worker_witness(worker_directory/"witness");
   auto worker=std::make_unique<CapellaDurableReactor>(worker_owner,rules,worker_limits,initial,std::function<void()>{},&worker_witness);worker->tick(clock);const auto pending=worker->current().token;
   bool cancelled=false;try{worker->block(included_wire,[]{throw ExecutionCancelled("cancel included worker");});}catch(const ExecutionCancelled&){cancelled=true;}
   check(cancelled&&worker->current().token==pending,"cancelled included worker published state");++checks;
   size_t retries=0;while(!worker->block(included_wire)){check(worker->current().token==pending,"incomplete included worker published state");check(++retries<2*(slot-parent.header.slot)/3+12,"included worker continuation stalled");}
   check(retries>(slot-parent.header.slot-1)/3&&worker->current().token.sequence==pending.sequence+1,"included worker skipped target continuation or published twice");++checks;
   check(capella_store_snapshot(*worker->current().state,rules,worker_limits)==capella_store_snapshot(*expected,rules,worker_limits),"included worker Store differs from full-gap oracle");++checks;
   const auto admitted_token=worker->current().token;check(worker->block(included_wire)&&worker->current().token==admitted_token,"included duplicate rewrote checkpoint");++checks;
   worker.reset();worker=std::make_unique<CapellaDurableReactor>(worker_owner,rules,worker_limits,worker_witness);
   const auto block_root=consensus_beacon_header_root({little(included_wire,100,8),little(included_wire,108,8),root_at(included_wire,116),root_at(included_wire,148),capella_beacon_body_root(std::span<const uint8_t>(included_wire).subspan(184),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes)});
   check(worker->current().token==admitted_token&&worker->signed_block(block_root)==std::optional<Bytes>(included_wire),"included worker witness reopen lost Store or signed archive");++checks;
  }
  auto incompatible=r;incompatible.chain_id+=1;CapellaBlockPreparation wrong_profile(original,parent.header,incompatible);while(!wrong_profile.advance_for_block(slot,3,100000)){}
  refused=false;try{prepare_capella_prepared_received_block(timed,wire,rules,wrong_profile,3,100000);}catch(const std::invalid_argument&){refused=true;}check(refused,"prepared Store admitted a frame from another transition profile");++checks;
  CapellaDutyPreparation local(timed,rules,slot);size_t local_attempts=0;
  while(!local.advance(slots,100000)){check(++local_attempts<(slot-parent.header.slot)/slots+3,"local preparation failed to advance");}
  check(local_attempts>0&&local.ready()&&local.head_state()==advanced,"local slot preparation differs from full-gap oracle");++checks;
  const auto local_duties=local.duties(timed),oracle_duties=prepare_capella_attestation_duties(timed,rules,slot,slot-parent.header.slot,100000);
  check(local_duties.size()==oracle_duties.size(),"prepared local committee count differs");
  for(const auto& [member,duty]:local_duties){const auto& expected=oracle_duties.at(member);check(duty.public_key==expected.public_key&&duty.genesis_root==expected.genesis_root&&duty.fork_version==expected.fork_version&&duty.attestation.data==expected.attestation.data&&duty.attestation.aggregation_bits==expected.attestation.aggregation_bits,"prepared local attestation differs from oracle");}++checks;
  check(local.block()&&local.block()->proposer_for_block(slot,100000)==identity,"prepared local proposer differs from oracle");++checks;
  check(local.sync_state(100000)==prepare_capella_slots(original,epoch_rules,l,slot+1,slot+1-parent.header.slot,100000),"prepared local sync state differs from oracle");++checks;
  check(!local.matches(timed,rules,slot+1),"local preparation reused at wrong slot");++checks;
  {
   const auto vote_time=clock+rules.attestation_due_ms;
   const auto vote_store=prepare_capella_store_tick(initial,vote_time,rules,3,100000);
   const auto full_duties=prepare_capella_attestation_duties(vote_store,rules,slot,slot-parent.header.slot,100000);check(!full_duties.empty(),"long-gap vote test has no committee assignment");
   const auto& assignment=full_duties.begin()->second;auto vote=assignment.attestation;Key voter_key(assignment.validator_index);
   vote.signature=voter_key.sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},assignment.fork_version,assignment.genesis_root)));
   CapellaDutyPreparation moving(vote_store,rules,slot);check(!moving.advance(3,100000),"moving preparation skipped gap");const auto prefix=moving.prepared_slot();
   const auto next_slot=(slot/slots+1)*slots;const auto later=prepare_capella_store_tick(vote_store,initial.timed.clock.genesis_time_ms+next_slot*rules.slot_duration_ms+rules.attestation_due_ms,rules,3,100000);
   check(moving.retarget(later,rules,next_slot)&&moving.prepared_slot()==prefix,"slot/epoch retarget discarded authenticated progress");++checks;
   size_t moving_chunks=0;while(!moving.advance(3,100000))check(++moving_chunks<(next_slot-parent.header.slot)/3+3,"retargeted preparation stalled");
   check(moving.head_state()==prepare_capella_slots(original,epoch_rules,l,next_slot,next_slot-parent.header.slot,100000),"retargeted state differs from ordinary slots");++checks;
   const auto moving_duties=moving.duties(later),moving_oracle=prepare_capella_attestation_duties(later,rules,next_slot,next_slot-parent.header.slot,100000);check(!moving_duties.empty()&&moving_duties.size()==moving_oracle.size(),"retargeted committee mismatch");
   for(const auto& [member,duty]:moving_duties){const auto& oracle=moving_oracle.at(member);check(duty.attestation.data==oracle.attestation.data&&duty.attestation.aggregation_bits==oracle.attestation.aggregation_bits&&duty.public_key==oracle.public_key&&duty.fork_version==oracle.fork_version,"retargeted duty differs from full-gap oracle");}++checks;
   check(!moving.retarget(vote_store,rules,slot),"backwards retarget accepted");++checks;
   CapellaTargetPreparation target(vote_store,rules,vote.data.target);check(!target.advance(3,100000),"target preparation unexpectedly skipped long gap");++checks;
   bool interrupted=false;try{target.advance(3,100000,[]{throw ExecutionCancelled("target chunk cancellation");});}catch(const ExecutionCancelled&){interrupted=true;}check(interrupted&&!target.ready(),"target cancellation swallowed or fabricated completion");++checks;
   size_t chunks=0;while(!target.advance(3,100000)){check(++chunks<(slot-parent.header.slot)/3+3,"target chunks stalled");}
   const auto context=target.context(vote_store,rules),context_oracle=prepare_capella_fork_state_contexts(original,parent.header,vote.data.target,epoch_rules,l,slot-parent.header.slot,100000).target;
   check(context.checkpoint==context_oracle.checkpoint&&context.registry==context_oracle.registry&&context.randao==context_oracle.randao&&context.fork.previous_version==context_oracle.fork.previous_version&&context.fork.current_version==context_oracle.fork.current_version&&context.fork.epoch==context_oracle.fork.epoch&&context.genesis_root==context_oracle.genesis_root,"resumed target differs from full-gap oracle");++checks;
   const auto after=prepare_capella_store_tick(vote_store,initial.timed.clock.genesis_time_ms+(slot+1)*rules.slot_duration_ms,rules,3,100000);
   bool limited=false;try{prepare_capella_store_vote(after,vote,rules,3,100000);}catch(const std::length_error&){limited=true;}check(limited,"legacy vote replay budget weakened");++checks;
   const auto expected_vote=prepare_capella_store_vote(after,vote,rules,slot-parent.header.slot,100000),prepared_vote=prepare_capella_prepared_store_vote(after,vote,rules,target);
   check(expected_vote&&prepared_vote&&capella_store_snapshot(*expected_vote,rules,crash_limits)==capella_store_snapshot(*prepared_vote,rules,crash_limits),"prepared vote admission differs from ordinary oracle");++checks;
   auto invalid_vote=vote;invalid_vote.signature[0]^=1;check(!prepare_capella_prepared_store_vote(after,invalid_vote,rules,target),"prepared vote accepted invalid BLS");++checks;
   auto future_target=vote.data.target;future_target.epoch=vote_store.timed.view.current_epoch+1;bool future_refused=false;try{CapellaTargetPreparation future(vote_store,rules,future_target);}catch(const std::invalid_argument&){future_refused=true;}check(future_refused,"target frame allowed a future epoch before replay");++checks;
   auto other_profile=rules;other_profile.transition.chain_id+=1;bool mismatch=false;try{target.context(vote_store,other_profile);}catch(const std::invalid_argument&){mismatch=true;}check(mismatch,"target cache accepted a different profile");++checks;
   // Exercise the fork-clock registry update with an explicit unrealized
   // checkpoint fixture; this does not claim that its FFG votes were produced.
   auto ffg_input=vote_store;ffg_input.timed.clock.unrealized_justified=vote.data.target;
   const auto boundary_time=initial.timed.clock.genesis_time_ms+(slot/slots+1)*slots*rules.slot_duration_ms;
   const std::array<const CapellaTargetPreparation*,1> targets{&target};
   const auto ffg_oracle=prepare_capella_store_tick(ffg_input,boundary_time,rules,slot-parent.header.slot,100000);
   const auto ffg_prepared=prepare_capella_contextual_store_tick(ffg_input,boundary_time,rules,targets,3,100000);
   const auto ffg_replayed=prepare_capella_resumed_store_tick(ffg_input,boundary_time,rules,3,100000);
   check(capella_store_snapshot(ffg_replayed,rules,crash_limits)==capella_store_snapshot(ffg_oracle,rules,crash_limits),"resumed archive tick differs from full-gap oracle");++checks;
   check(capella_store_snapshot(ffg_prepared,rules,crash_limits)==capella_store_snapshot(ffg_oracle,rules,crash_limits)&&ffg_prepared.timed.view.justified==vote.data.target&&ffg_prepared.timed.view.justified_registry==context.registry,"prepared tick justification differs from full-gap oracle");++checks;
   limited=false;try{prepare_capella_store_tick(ffg_input,boundary_time,rules,3,100000);}catch(const std::length_error&){limited=true;}check(limited,"ordinary tick replay budget weakened");++checks;
   CapellaTargetPreparation incomplete(vote_store,rules,vote.data.target);const std::array<const CapellaTargetPreparation*,1> unfinished{&incomplete};
   bool bad_context=false;try{prepare_capella_contextual_store_tick(ffg_input,boundary_time,rules,unfinished,3,100000);}catch(const std::invalid_argument&){bad_context=true;}check(bad_context&&ffg_input.timed.view.justified==vote_store.timed.view.justified,"incomplete tick context published justification");++checks;
   bad_context=false;try{prepare_capella_contextual_store_tick(ffg_input,boundary_time,other_profile,targets,3,100000);}catch(const std::invalid_argument&){bad_context=true;}check(bad_context,"tick used target from another profile");++checks;
   interrupted=false;try{prepare_capella_contextual_store_tick(ffg_input,boundary_time,rules,targets,3,100000,[]{throw ExecutionCancelled("cancel contextual tick");});}catch(const ExecutionCancelled&){interrupted=true;}check(interrupted&&ffg_input.timed.view.justified==vote_store.timed.view.justified,"contextual tick swallowed cancellation or mutated source");++checks;
   {
    Temp ffg_directory;Genesis fg;fg.chain_id=r.chain_id;fg.consensus="beacon";fg.evm_revision=EvmRevision::Shanghai;fg.block=genesis;fg.state=genesis_state;
    auto ffg_limits=crash_limits;ffg_limits.replay_slots=3;Blockchain owner(ffg_directory/"owner",fg);ChainStore witness(ffg_directory/"witness");
    CapellaDurableReactor reactor(owner,rules,ffg_limits,ffg_input,{},&witness);const auto before=reactor.current().token;
    bool cancelled=false;try{reactor.tick(boundary_time,[]{throw ExecutionCancelled("cancel reactor promotion");});}catch(const ExecutionCancelled&){cancelled=true;}
    check(cancelled&&reactor.current().token==before,"cancelled Reactor tick published justification");++checks;
    reactor.tick(boundary_time);check(capella_store_snapshot(*reactor.current().state,rules,ffg_limits)==capella_store_snapshot(ffg_oracle,rules,ffg_limits),"Reactor tick promotion differs from full-gap oracle");++checks;
   }
   Temp vote_directory;Genesis vg;vg.chain_id=r.chain_id;vg.consensus="beacon";vg.evm_revision=EvmRevision::Shanghai;vg.block=genesis;vg.state=genesis_state;
   auto vote_limits=crash_limits;vote_limits.replay_slots=3;Blockchain vote_owner(vote_directory/"owner",vg);ChainStore vote_witness(vote_directory/"witness");
   auto receiver=std::make_unique<CapellaDurableReactor>(vote_owner,rules,vote_limits,initial,std::function<void()>{},&vote_witness);receiver->tick(vote_time);const auto vote_token=receiver->current().token;
   size_t attempts=0;while(!receiver->vote(vote)){check(receiver->current().token==vote_token,"unfinished gossip target published a checkpoint");check(++attempts<(slot-parent.header.slot)/3+3,"gossip target failed to resume");}
   check(attempts>0&&receiver->current().token.sequence==vote_token.sequence+1&&receiver->signed_votes().size()==1,"gossip target did not defer then admit exactly once");++checks;
   const auto accepted_token=receiver->current().token;check(receiver->vote(vote)&&receiver->current().token==accepted_token,"gossip duplicate rewrote checkpoint");++checks;
   receiver.reset();receiver=std::make_unique<CapellaDurableReactor>(vote_owner,rules,vote_limits,vote_witness);
   check(receiver->current().token==accepted_token&&receiver->signed_votes().size()==1,"long-gap signed vote witness restore failed");++checks;
   {
    Blockchain sender_owner(vote_directory/"sender",vg);ChainStore sender_witness(vote_directory/"sender-witness");CapellaDurableReactor sender(sender_owner,rules,vote_limits,initial,{},&sender_witness);sender.tick(vote_time);
    size_t polls=0;while(!sender.prepare_current_duties(slot))check(++polls<(slot-parent.header.slot)/3+3,"local vote prewarming stalled");
    ChainStore signer_disk(vote_directory/"sender-key");BeaconSigningJournal journal(signer_disk,r.chain_id,genesis.header.hash(),assignment.genesis_root,voter_key.public_key,slots);
    const auto local_vote=sender.attest_current(assignment.validator_index,slot,[&]{return vote_time;},journal,[&](const ConsensusRoot& root){return voter_key.sign(root);});
    check(local_vote&&local_vote->data==vote.data&&local_vote->signature==vote.signature&&sender.vote(*local_vote)&&journal.latest_attestation_target()==std::optional<uint64_t>(vote.data.target.epoch),"long-gap local signed vote was not immediately admitted through prepared target");++checks;
   }
   receiver->tick(initial.timed.clock.genesis_time_ms+(slot+1)*rules.slot_duration_ms);
   check(capella_store_snapshot(*receiver->current().state,rules,vote_limits)==capella_store_snapshot(*expected_vote,rules,vote_limits),"restored deferred vote applied incorrectly at next tick");++checks;
  }
  Temp directory;Genesis g;g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;g.block=genesis;g.state=genesis_state;
  {
   Blockchain local_owner(directory/"local-owner",g);ChainStore local_witness(directory/"local-witness");CapellaDurableReactor local_reactor(local_owner,rules,crash_limits,initial,{},&local_witness);check(local_reactor.prepare_current_duties(parent.header.slot),"current-head preparation unavailable");local_reactor.tick(clock);
   const auto local_token=local_reactor.current().token;size_t polls=0;
   while(!local_reactor.prepare_current_duties(slot)){check(local_reactor.current().token==local_token,"incomplete duty preparation published state");check(++polls<(slot-parent.header.slot)/slots+3,"reactor local duty preparation stalled");}
   check(polls>0&&local_reactor.current_proposer(slot)==std::optional<uint64_t>(identity->index),"reactor prepared local proposer mismatch");++checks;
   ChainStore signer_disk(directory/"local-signer");BeaconSigningJournal journal(signer_disk,r.chain_id,genesis.header.hash(),identity->genesis_root,key.public_key,slots);
   const auto local_wire=local_reactor.propose_current(identity->index,slot,"0x0000000000000000000000000000000000000000",4,4,[&]{return clock;},journal,[&](const ConsensusRoot& root){return key.sign(root);});
   check(local_wire&&journal.latest_proposal_slot()==std::optional<uint64_t>(slot),"prepared local proposal did not reserve and sign");++checks;
   const auto local_oracle=prepare_capella_received_block(timed,*local_wire,rules,slot-parent.header.slot,100000);
   check(local_oracle&&local_reactor.block(*local_wire),"prepared local proposal was not immediately admitted");++checks;
   check(capella_store_snapshot(*local_reactor.current().state,rules,crash_limits)==capella_store_snapshot(*local_oracle,rules,crash_limits),"local signed proposal Store differs from ordinary oracle");++checks;
   check(!local_reactor.propose_current(identity->index,slot,"0x0000000000000000000000000000000000000000",4,4,[&]{return clock;},journal,[&](const ConsensusRoot& root){return key.sign(root);}),"admitted local slot signed twice");++checks;
  }
  Blockchain owner(directory/"owner",g);ChainStore witness(directory/"witness");auto reactor=std::make_unique<CapellaDurableReactor>(owner,rules,crash_limits,initial,std::function<void()>{},&witness);reactor->tick(clock);
  const auto token=reactor->current().token;InboundBudget budget;CapellaConsensusService service(*reactor,budget,2*r.consensus.maximum_block_bytes+8192,r.consensus.maximum_block_bytes,4096,4096,4,2*r.consensus.maximum_block_bytes+8192);
  auto envelope=reactor->network_identity();envelope["kind"]="block";envelope["ssz"]=hex(wire);
  const auto maximum_attempts=(slot-1-parent.header.slot+slots-1)/slots+1;size_t attempts=0;bool accepted=false;
  while(!accepted&&attempts<maximum_attempts){++attempts;auto promise=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto future=promise->get_future();check(service.submit(envelope.dump(),std::make_shared<std::atomic<bool>>(false),[promise](auto decision){promise->set_value(decision);}),"prepared block worker queue refused job");check(future.wait_for(std::chrono::seconds(10))==std::future_status::ready,"prepared block worker did not finish bounded job");const auto decision=future.get();check(decision!=CapellaConsensusService::Decision::local_failure,"prepared worker hit historical replay budget or another local failure");accepted=decision==CapellaConsensusService::Decision::accepted;if(!accepted)check(reactor->current().token==token&&owner.tip().header.hash()==parent.execution_block.header.hash(),"unfinished incoming preparation published durable state");}
  service.stop();service.join();check(accepted&&attempts==1,"prepared worker required external retries or failed to admit");++checks;
  const auto committed=reactor->current();check(committed.token.sequence==token.sequence+1&&capella_store_snapshot(*committed.state,rules,crash_limits)==capella_store_snapshot(*admitted_oracle,rules,crash_limits)&&owner.tip().header.hash()==execution.header.hash()&&owner.state().root()==expected.root(),"prepared worker published incorrect core/execution checkpoint");++checks;
  check(reactor->signed_block(consensus_beacon_header_root(capella_fork_head(*committed.state,rules).header))==std::optional<Bytes>(wire),"prepared worker did not archive authenticated signed bytes");++checks;
  reactor.reset();CapellaDurableReactor reopened(owner,rules,crash_limits,witness);check(reopened.current().token==committed.token&&capella_store_snapshot(*reopened.current().state,rules,crash_limits)==capella_store_snapshot(*committed.state,rules,crash_limits),"prepared worker witness restart changed checkpoint");++checks;
  check(reopened.block(wire)&&reopened.current().token==committed.token,"prepared worker duplicate admission rewrote checkpoint");++checks;

 }
 std::cout<<Json{{"scenario","capella-prepared-block-transition"},{"workerIntegration",true},{"localProposalIntegration",true},{"longGapGossipVoteRestore",true},{"boundedRetryHarness",false},{"oneJobContinuation",true},{"witnessedPublication",true},{"checks",checks},{"variants",2},{"signedBlsBlocks",2},{"type2Transactions",2},{"epochBoundaryFinalSlot",true},{"fullGapOracleEqual",true},{"runtimeIntegrated",false},{"networkQualified",false}}.dump()<<std::endl;
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
 state.get(SignedTransaction::sign(tx,key).sender()).balance=Int(deposit_reorg?"70000000000000000000":(deposit_network||deposit_activation||deposit_invalid_signature)?"34000000000000000000":pruning_native?"3000000000000000000":"1000000000000000000");state.get(target).code=unhex("602a5f5500");
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
  // Compare cold and warm behavior after changes to both serialized slot fields.
  // A cache hit must never substitute the old SSZ or processing-epoch metadata.
  for(unsigned mutation=0;mutation<14;++mutation){
   auto altered=value;auto& block=altered.blocks.begin()->second;auto& state=block.state;
   const auto id=altered.blocks.begin()->first;
   if(mutation==0)state.ssz.back()^=1;
   else if(mutation==1)state.last_processed_epoch=state.last_processed_epoch?std::nullopt:std::optional<uint64_t>(0);
   else if(mutation==2)block.execution_block.header.extra_data.push_back(1);
   else if(mutation==3)block.receipts.push_back(Receipt{});
   else if(mutation==4)++altered.timed.view.nodes.at(id).realized_justified_epoch;
   else if(mutation==5)++altered.timed.view.nodes.at(id).unrealized_justified_epoch;
   else if(mutation==6)++altered.unrealized_justifications.at(id).epoch;
   else if(mutation==7)++block.execution_state.get(target).balance;
   else if(mutation==8)++block.execution_state.get(target).nonce;
   else if(mutation==9)block.execution_state.get(target).code.push_back(0);
   else if(mutation==10)block.execution_state.get(target).storage[Int(17)]+=1;
   else if(mutation==11)block.execution_state.get(hex(bytes(Int(987654321),20)));
   else if(mutation==12)block.execution_state.erase(target);
   else block.execution_state.get(target).storage[Int(987654321)]=0;
   auto outcome=[&](CapellaStoreSnapshotCache* memo){
    try{return std::optional<std::string>(capella_store_snapshot(altered,store_rules,snapshot_limits,{},memo));}
    catch(const std::invalid_argument&){return std::optional<std::string>{};}
   };
   const auto uncached=outcome(nullptr),cached=outcome(&cache);
   check(uncached==cached&&(!cached||*cached!=encoded),"cache masked changed complete record or justification");
  }
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
    const auto& exit_head=capella_fork_head(*before.state,store_rules);const auto exit_context=capella_fork_block_context(exit_head.state,exit_head.header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state);
    auto signed_request=[&](uint64_t requested_epoch,uint64_t index,uint64_t signer,ConsensusBytes4 domain,ConsensusBytes4 fork,ConsensusRoot genesis){SignedConsensusVoluntaryExit request{requested_epoch,index,{}};request.signature=Key(signer).sign(consensus_signing_root(consensus_voluntary_exit_root(requested_epoch,index),consensus_domain(domain,fork,genesis)));return encode_consensus_voluntary_exit_ssz(request);};
    auto wrong_fork=version;wrong_fork[0]^=1;auto wrong_genesis=genesis_root;wrong_genesis[0]^=1;
    const std::vector<std::pair<std::string,Bytes>> invalid_contexts{
     {"wrong-domain",signed_request(selected_exit.epoch,2,2,{1,0,0,0},version,genesis_root)},
     {"wrong-fork",signed_request(selected_exit.epoch,2,2,{4,0,0,0},wrong_fork,genesis_root)},
     {"wrong-genesis",signed_request(selected_exit.epoch,2,2,{4,0,0,0},version,wrong_genesis)},
     {"wrong-index",signed_request(selected_exit.epoch,3,2,{4,0,0,0},version,genesis_root)},
     {"wrong-key",signed_request(selected_exit.epoch,2,3,{4,0,0,0},version,genesis_root)},
     {"future-epoch",signed_request(exit_context.registry.epoch+1,2,2,{4,0,0,0},version,genesis_root)},
     {"unknown-index",signed_request(selected_exit.epoch,UINT64_MAX,2,{4,0,0,0},version,genesis_root)}};
    Json rejected=Json::array();for(const auto& [name,bytes]:invalid_contexts){check(!reactor->voluntary_exit_ssz(bytes)&&reactor->current().token==before.token&&reactor->pending_exits().empty(),"exit context rejection mutated state: "+name);rejected.push_back(name);}
    {Temp immature_dir;ChainStore disk(immature_dir/"queue");disk.bind_chain(r.chain_id,disk_genesis.header.hash(),"capella-exit-tenure");disk.commit(disk_genesis,disk_genesis_state,{});auto immature_rules=store_rules;immature_rules.transition.consensus.exit.shard_committee_period=exit_context.registry.epoch+1;CapellaDurableReactor immature(disk,r.chain_id,disk_genesis.header.hash(),immature_rules,snapshot_limits,*before.state);const auto token=immature.current().token;check(!immature.voluntary_exit_ssz(operation)&&immature.current().token==token&&immature.pending_exits().empty(),"immature exit changed checkpoint");rejected.push_back("insufficient-tenure");}
    std::cout<<Json{{"scenario","capella-exit-authentication-rejections"},{"rejected",rejected},{"checkpointUnchanged",true},{"queueEmpty",true}}.dump()<<std::endl;
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
   if(beacon_reads&&slot==first_slot+2)verify_beacon_read_admission(*reactor,wire,parent_root);
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
      if(applicable){
       const auto current=reactor->current();const auto& head=capella_fork_head(*current.state,store_rules);const auto root=consensus_beacon_header_root(head.header);const auto next_slot=slot+1;
       const auto identity=prepare_capella_proposer_identity(head.state,head.header,next_slot,epoch_rules,l,duties,64,100000);check(bool(identity),"exit reorg proposer missing");
       ConsensusRoot next_epoch{};for(unsigned i=0;i<8;++i)next_epoch[i]=uint8_t((next_slot/slots)>>(8*i));
       const auto reveal=Key(identity->index).sign(consensus_signing_root(next_epoch,consensus_domain({2,0,0,0},version,genesis_root)));
       const auto body=reactor->prepare_local_body(identity->index,next_slot,reveal,head.execution_block.header.coinbase,current.token,{},16,64);check(bool(body),"orphaned exit proposal missing");
       const auto candidate=prepare_capella_unsigned_block(head.state,next_slot,identity->index,root,*body,r,head.execution_block.header,head.execution_state,[&](const Int& n){return capella_execution_history(*current.state,root,n);},64,100000);check(bool(candidate),"orphaned exit proposal invalid");
       const auto wire=sign_message(candidate->message_ssz,identity->index);const auto included=prepare_capella_received_block(*current.state,wire,store_rules,64,100000);check(bool(included),"orphaned exit signed block rejected");
       const auto& state=candidate->post.consensus.ssz;const auto registry=little(state,hist+88,4);check(little(state,registry+121*selected_exit.validator_index+105,8)!=UINT64_MAX,"orphaned exit not included after reorg");
       check(reactor->current().token==current.token,"reorg preparation changed live checkpoint");
       std::cout<<Json{{"scenario","capella-exit-reincluded"},{"automaticSelection",true},{"signedBlockAccepted",true},{"liveCheckpointUnchanged",true}}.dump()<<std::endl;
      }
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
    if(slashing_reorg){qualify_slashing_reorg(*reactor->current().state,store_rules,snapshot_limits,disk_genesis,disk_genesis_state,proof_a,proof_b);return;}
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
  if(slot==first_slot){short_snapshot=capella_store_snapshot(*store,store_rules,snapshot_limits);if(prepared_block){prepared_block_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(slot_continuation){slot_continuation_loop(*store,store_rules);return;}if(signer_health){signer_health_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(duty_timing){duty_timing_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(clock_diagnostics){clock_diagnostics_loop(*store,store_rules,disk_genesis,disk_genesis_state);startup_catchup_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(deposit_reorg){deposit_reorg_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(configured_runtime){runtime_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}if(scheduled){scheduler_loop(*store,store_rules,disk_genesis,disk_genesis_state);return;}
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
    ChainStore witness(tmp/"witness");const auto intended=expected.at("new").get<std::string>();const ForkCheckpointToken committed{expected.at("sequence").get<uint64_t>()+1,sha256(Bytes(intended.begin(),intended.end()))};
    auto wrong=committed;wrong.digest=Bytes(32,0);bool rejected_commitment=false;try{CapellaDurableReactor invalid(*owner,store_rules,snapshot_limits,witness,{},wrong);}catch(const std::invalid_argument& e){rejected_commitment=std::string(e.what()).find("commitment differs")!=std::string::npos;}check(rejected_commitment&&disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes)->token==record->token,"commitment rejection published pending checkpoint");
    auto resumed=std::make_unique<CapellaDurableReactor>(*owner,store_rules,snapshot_limits,witness,std::function<void()>{},committed);const auto current=resumed->current();
    check(capella_store_snapshot(*current.state,store_rules,snapshot_limits)==expected.at("new").get<std::string>(),"witness failed to finish pending publication");const auto& head=capella_fork_head(*current.state,store_rules);const auto id=consensus_beacon_header_root(head.header);
    check(resumed->signed_block(id)==std::optional<Bytes>(sync_wire),"witness did not restore signed archive");check(owner->tip().header.hash()==head.execution_block.header.hash()&&owner->state().root()==head.execution_state.root(),"witness owner state mismatch");
    for(const auto& tx:head.execution_block.transactions)check(disk.receipt(tx.hash())&&disk.transaction(tx.hash()),"witness transaction indexes missing");
    Rpc rpc(*owner);check(!rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","eth_blockNumber"},{"params",Json::array()}}).contains("error"),"witness owner RPC unavailable");
    const auto token=current.token;resumed.reset();
    {const size_t budget=2048+8*snapshot_limits.envelope_bytes;const auto saved=witness.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),budget);check(bool(saved),"witness intent missing");auto corrupt=Json::parse(saved->snapshot);auto wire=unhex(corrupt.at("archive").at(0).at(2).get<std::string>());wire[4]^=1;corrupt["archive"][0][2]=hex(wire);ChainStore malformed(tmp/"bad-witness");malformed.bind_chain(r.chain_id,disk_genesis.header.hash(),"nano-capella-recovery-v1");malformed.commit_fork_checkpoint(r.chain_id,disk_genesis.header.hash(),{},corrupt.dump(),budget);bool refused=false;try{CapellaDurableReactor invalid(*owner,store_rules,snapshot_limits,malformed);}catch(const std::invalid_argument&){refused=true;}check(refused&&disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes)->token==token,"corrupt witness archive accepted");}
    {ChainStore empty(tmp/"empty-witness");bool refused=false;try{CapellaDurableReactor invalid(*owner,store_rules,snapshot_limits,empty);}catch(const std::invalid_argument&){refused=true;}check(refused&&disk.fork_checkpoint(r.chain_id,disk_genesis.header.hash(),snapshot_limits.envelope_bytes)->token==token,"empty witness authorized recovery");}
    {Blockchain stale(tmp/"stale",owner_genesis);bool refused=false;try{CapellaDurableReactor invalid(stale,store_rules,snapshot_limits,witness);}catch(const std::invalid_argument&){refused=true;}check(refused,"witness accepted older rollback");}
    resumed=std::make_unique<CapellaDurableReactor>(*owner,store_rules,snapshot_limits,witness);check(resumed->current().token==token,"witness duplicate restart changed token");
    std::cout<<Json{{"externalCommitmentVerified",true},{"scenario","capella-owner-witness"},{"lostSuffix",lose},{"faultWitness",fault_witness},{"corruptArchiveRejected",true},{"blockedOperations",result.at("blocked")},{"restoredSignedArchive",true},{"emptyWitnessRejected",true},{"oldRollbackRejected",true},{"physicalPowerLoss",false},{"networkActivated",false}}.dump()<<std::endl;continue;
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
if(argc==2&&(std::string(argv[1])=="--atomic-proposal"||std::string(argv[1])=="--proposal-gap")){atomic_proposal=std::string(argv[1])=="--atomic-proposal";proposal_gap=!atomic_proposal;prepared_block=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--capacity-pressure"){capacity_pressure=true;prepared_block=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--incoming-parent-interference"){incoming_parent_interference=true;prepared_block=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--incoming-alternation"){incoming_alternation=true;prepared_block=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--vote-target-alternation"){vote_target_alternation=true;prepared_block=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--prepared-block"){prepared_block=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--slot-continuation"){slot_continuation=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--signer-health"){signer_health=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--duty-timing"){duty_timing=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--clock-diagnostics"){clock_diagnostics=true;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--unknown-finality-slashing"){slashing_unknown_finality=true;slashing_new_validator=true;deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--new-validator-slashing"){slashing_new_validator=true;deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--slashing-multi"){slashing_multi=true;slashing_retirement=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--slashing-reorg"){slashing_reorg=true;scenario(false);return 0;}
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
 if(argc==3&&std::string(argv[1])=="--export-deposit-network"){deposit_network=true;pruning_native=true;network_export=fs::absolute(argv[2]);network_cli=true;managed_cli=true;cli_binary="export-only";configured_runtime=true;scenario(false);return 0;}
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
if(argc==3&&std::string(argv[1])=="--slashing-sync-writer")return slashing_sync_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--slashing-sync"){slashing_sync=true;slashing_multi=true;slashing_retirement=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==3&&std::string(argv[1])=="--exit-lifecycle-writer")return exit_lifecycle_writer(argv[2]);
if(argc==2&&std::string(argv[1])=="--exit-lifecycle-sync"){exit_lifecycle_sync=true;local_exits=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
#endif
if(argc==2&&std::string(argv[1])=="--exit-pending-pruning"){exit_pending_pruning=true;local_exits=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--exit-retirement"){exit_retirement=true;local_exits=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--http-queue"){configured_runtime=true;capella_http=true;capella_http_queue=true;scenario(false);return 0;}if(argc==3&&std::string(argv[1])=="--historical-pruning-migration"){pruning_migration_helper=argv[2];historical_pruning_restore=true;historical_state_pruning=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--historical-pruning-restore"){historical_pruning_restore=true;historical_state_pruning=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--historical-state-pruning"){historical_state_pruning=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}
if(argc==2&&std::string(argv[1])=="--historical-export"){historical_export=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==3&&std::string(argv[1])=="--cli"){cli_binary=argv[2];configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--http"){configured_runtime=true;capella_http=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--catalog-concurrency"){wire_mode=true;catalog_concurrency=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-reorg"){queued_bls_reorg=true;queued_bls_changes=true;local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-automatic"){pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--slashing-retirement"){slashing_retirement=true;pruning_automatic=true;pruning_native=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-slashing"){pruning_slashing=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--local-sync"){local_sync=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-invalid-signature"){deposit_invalid_signature=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--automatic-registration"){automatic_registration=true;configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-auto-registration"){automatic_registration=true;deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-activation-restart"){deposit_activation_restart=true;deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-activation"){deposit_activation=true;deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-consumption"){deposit_consumption=true;pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-native"){pruning_native=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-bls"){pruning_bls=true;pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--pruning-primitive"){pruning_primitive=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-owner"){queued_bls_changes=true;local_bls_changes=true;owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-rpc"){queued_bls_changes=true;local_bls_changes=true;configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-wire"){wire_mode=true;queued_bls_changes=true;local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-bls-changes"){queued_bls_changes=true;local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--exit-wire"){exit_wire=true;queued_exits=true;local_exits=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--queued-exits"){queued_exits=true;local_exits=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--local-exits"){local_exits=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--local-bls-changes"){local_bls_changes=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--wire"){wire_mode=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--beacon-reads"){beacon_reads=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--automatic-recovery"){automatic_recovery=true;configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--configured-runtime"){configured_runtime=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--scheduler-sync-rotation"){scheduled=true;sync_rotation=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-reorg-activation"){deposit_reorg_activation=true;deposit_reorg=true;pruning_native=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--deposit-reorg"){deposit_reorg=true;pruning_native=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--scheduler-registration"){automatic_registration=true;scheduled=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--scheduler"){scheduled=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--owner-reorg"){owner_reorg=true;scenario(false);return 0;}if(argc==2&&std::string(argv[1])=="--owner-finality"){owner_mode=true;local_finality=100;scenario(false);return 0;}if(argc==2&&(std::string(argv[1])=="--local-finality"||std::string(argv[1])=="--local-finality-half")){local_finality=std::string(argv[1])=="--local-finality"?100:50;scenario(false);return 0;}
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
