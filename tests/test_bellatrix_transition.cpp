#include "support/altair_transition_rules.hpp"
#include "nano/consensus_bellatrix_transition.hpp"
#include "nano/consensus_proposer.hpp"
#include "nano/consensus_bellatrix_fork_store.hpp"
#include "nano/committee.hpp"
#include "nano/consensus_bellatrix_store_snapshot.hpp"
#include "nano/storage.hpp"
#include "nano/consensus_bellatrix_reactor.hpp"
#include "nano/consensus_bellatrix_duty.hpp"
#include "nano/consensus_bellatrix_validator.hpp"
#include "nano/consensus_bellatrix_deposits.hpp"
#include "nano/consensus_bellatrix_runtime.hpp"
#include "support/bellatrix_runtime_scenario.hpp"
#include "nano/http.hpp"
#include "../network/bellatrix_rpc_service.hpp"
#include "../network/bellatrix_consensus_service.hpp"
#include "nano/consensus_message_ssz.hpp"
#include <future>
#include "blst.h"
#ifdef __APPLE__
extern "C" void nano_test_arm_sync_fault(const char*);
extern "C" unsigned nano_test_sync_failures();
extern "C" void nano_test_disarm_sync_fault();
#endif
using namespace test;using namespace test::phase0_minimal;
namespace {
void put(Bytes& b,size_t at,uint64_t n,size_t width=8){for(size_t i=0;i<width;++i)b.at(at+i)=uint8_t(n>>(8*i));}
template<class A> void copy_to(A& dst,const Bytes& src){check(dst.size()==src.size(),"fixture width");std::copy(src.begin(),src.end(),dst.begin());}
BellatrixExecutionPayload payload(const Block& block){const auto& h=block.header;BellatrixExecutionPayload p;copy_to(p.parent_hash,h.parent_hash);copy_to(p.fee_recipient,unhex(h.coinbase));copy_to(p.state_root,h.state_root);copy_to(p.receipts_root,h.receipts_root);copy_to(p.logs_bloom,h.logs_bloom);copy_to(p.prev_randao,h.mix_hash);copy_to(p.block_hash,h.hash());auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());copy_to(p.base_fee_per_gas,fee);p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;for(const auto& tx:block.transactions)p.transactions.push_back(tx.raw());return p;}
struct Key {
 blst_scalar secret;Bytes public_key;
 explicit Key(uint64_t index){uint64_t words[4]{index+1,0,0,0};blst_scalar_from_uint64(&secret,words);blst_p1 p;blst_sk_to_pk_in_g1(&p,&secret);public_key.resize(48);blst_p1_compress(public_key.data(),&p);}
 Bytes sign(const ConsensusRoot& root)const{static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),32,reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
};
Bytes staking_calldata(uint64_t index,const ConsensusBytes4& fork){
 Key validator(index);Phase0Deposit d{};copy_to(d.message.public_key,validator.public_key);d.message.withdrawal_credentials[0]=1;d.message.amount_gwei=32000000000;
 copy_to(d.signature,validator.sign(consensus_signing_root(consensus_deposit_root(d.message),consensus_domain({3,0,0,0},fork,{}))));
 const std::string signature="deposit(bytes,bytes,bytes,bytes32)";auto data=keccak(Bytes(signature.begin(),signature.end()));data.resize(4);Bytes args(416);
 auto word=[&](size_t at,size_t value){const auto encoded=bytes(Int(value),32);std::copy(encoded.begin(),encoded.end(),args.begin()+at);};word(0,128);word(32,224);word(64,288);const auto root=consensus_deposit_data_root(d);std::copy(root.begin(),root.end(),args.begin()+96);word(128,48);std::copy(d.message.public_key.begin(),d.message.public_key.end(),args.begin()+160);word(224,32);std::copy(d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(d.signature.begin(),d.signature.end(),args.begin()+320);data.insert(data.end(),args.begin(),args.end());return data;
}
struct RunningBellatrixHttp {
 HttpServer& server;std::thread thread;
 explicit RunningBellatrixHttp(HttpServer& s):server(s),thread([&s]{s.run();}){}
 ~RunningBellatrixHttp(){server.stop();thread.join();}
};
struct BellatrixNetworkPair {
 std::unique_ptr<Blockchain> observer;
 std::unique_ptr<BellatrixDurableReactor> reactor;
 std::unique_ptr<BellatrixLocalClock> clock;
 std::shared_ptr<BellatrixRpcService> ea,eb;
 std::unique_ptr<Rpc> ra,rb;std::unique_ptr<HttpServer> sa,sb;
 std::unique_ptr<RunningBellatrixHttp> ha,hb;
 std::unique_ptr<Process> receiver,sender;
 BellatrixNetworkPair(const fs::path& path,const fs::path& daemon,const Genesis& genesis,
  const BellatrixForkBlockRules& rules,const BellatrixStoreSnapshotLimits& limits,const BellatrixForkStore& initial,
  Blockchain& source,BellatrixDurableReactor& source_reactor,BellatrixLocalClock& source_clock,std::atomic<uint64_t>& wall){
  observer=std::make_unique<Blockchain>(path/"observer",genesis);reactor=std::make_unique<BellatrixDurableReactor>(*observer,rules,limits,initial);
  clock=std::make_unique<BellatrixLocalClock>(*reactor,rules.slot_duration_ms,[&wall]{return wall.load();});
  const auto identity=source_reactor.network_identity();const auto& t=rules.transition.consensus;const auto committee=uint64_t(t.body.validators_per_committee);
  const AltairWireProfile profile{identity.at("chainId").get<std::string>(),identity.at("genesisHash").get<std::string>(),identity.at("forkDigest").get<std::string>(),2*t.maximum_block_bytes+1024,t.maximum_block_bytes,size_t(228+(committee+8)/8),size_t(464+16*committee)};
  auto endpoint=[&](BellatrixDurableReactor& r,BellatrixLocalClock& c){return std::make_shared<BellatrixRpcService>(r,profile,4,2*t.maximum_block_bytes,10s,[clock=&c](const auto& check){clock->poll(check);},[clock=&c]{return clock->healthy();},identity.at("consensusIdentity").get<std::string>(),true);};
  ea=endpoint(source_reactor,source_clock);eb=endpoint(*reactor,*clock);
  ra=std::make_unique<Rpc>(source,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},nullptr,ea);
  rb=std::make_unique<Rpc>(*observer,"fixture-token",AdmissionBudget::Clock::now,RpcSimulationLimits{},nullptr,eb);
  sa=std::make_unique<HttpServer>(*ra,"127.0.0.1",0);sb=std::make_unique<HttpServer>(*rb,"127.0.0.1",0);
  ha=std::make_unique<RunningBellatrixHttp>(*sa);hb=std::make_unique<RunningBellatrixHttp>(*sb);
  receiver=std::make_unique<Process>(std::vector<std::string>{daemon.string(),std::to_string(sb->port()),(path/"observer-peer").string(),"/ip4/127.0.0.1/tcp/0"},Env{{"NANO_NETWORK_TOKEN","fixture-token"}});
  const auto address=receiver->event("listen").at("address").get<std::string>();receiver->event("altair-ingress-ready");
  sender=std::make_unique<Process>(std::vector<std::string>{daemon.string(),std::to_string(sa->port()),(path/"source-peer").string(),"/ip4/127.0.0.1/tcp/0",address},Env{{"NANO_NETWORK_TOKEN","fixture-token"}});
  sender->event("altair-ingress-ready");sender->event("peer");
 }
 void sync(const Bytes& hash){until([&]{return observer->tip().header.hash()==hash;},"Bellatrix observer canonical sync",20);}
 void stop(){check(sender->stop().code==0&&receiver->stop().code==0,"Bellatrix daemon shutdown");ea->stop();eb->stop();ea->join();eb->join();}
};
void check_finalized_rpc(Blockchain& chain,const BellatrixDurableReactor& reactor,const std::string& account,const std::string& contract){
 const auto current=reactor.current();const auto& finalized=current.state->blocks.at(current.state->timed.view.finalized.root);
 Rpc endpoint(chain);auto query=[&](const char* method,Json params){auto response=endpoint.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});check(response.contains("result"),std::string("finalized RPC error: ")+response.dump());return response.at("result");};
 const auto block=query("eth_getBlockByNumber",Json::array({"finalized",false}));
 check(block.at("hash")==hex(finalized.execution_block.header.hash())&&block.at("number")==quantity(finalized.execution_block.header.number),"RPC finalized block differs from Beacon checkpoint");
 const auto& state=finalized.execution_state;
 check(query("eth_getBalance",Json::array({account,"finalized"}))==quantity(state.get(account).balance),"finalized balance mismatch");
 check(query("eth_getTransactionCount",Json::array({account,"finalized"}))==quantity(state.get(account).nonce),"finalized nonce mismatch");
 check(query("eth_getCode",Json::array({contract,"finalized"}))==hex(state.get(contract).code),"finalized code mismatch");
 check(query("eth_getStorageAt",Json::array({contract,"0x0","finalized"}))==hex(bytes(state.get_storage(contract,0),32)),"finalized storage mismatch");
 check(query("eth_getBlockTransactionCountByNumber",Json::array({"finalized"}))==quantity(Int(std::to_string(finalized.execution_block.transactions.size()))),"finalized transaction count mismatch");
 const auto fees=query("eth_feeHistory",Json::array({"0x1","finalized",Json::array()}));
 check(fees.at("oldestBlock")==quantity(finalized.execution_block.header.number)&&fees.at("baseFeePerGas").at(0)==quantity(*finalized.execution_block.header.base_fee),"finalized fee history mismatch");
 const auto unchanged=chain.state().root();check(query("eth_call",Json::array({Json{{"to",contract},{"gas","0x186a0"}},"finalized"}))=="0x"&&chain.state().root()==unchanged,"finalized simulation mutated canonical state");

}
Bytes signed_header(const ConsensusBeaconHeader& h,const Bytes& signature){Bytes out(208);put(out,0,h.slot);put(out,8,h.proposer_index);std::copy(h.parent_root.begin(),h.parent_root.end(),out.begin()+16);std::copy(h.state_root.begin(),h.state_root.end(),out.begin()+48);std::copy(h.body_root.begin(),h.body_root.end(),out.begin()+80);std::copy(signature.begin(),signature.end(),out.begin()+112);return out;}
}
int main(int argc,char** argv){try{
 const bool deposit_activation=argc==2&&std::string(argv[1])=="--deposit-activation";
 const bool deposit_loop=deposit_activation||(argc==2&&std::string(argv[1])=="--deposit-loop");
 const bool deposit_store=argc==2&&std::string(argv[1])=="--deposit-store";
 const bool partition=argc==4&&std::string(argv[1])=="--partition-recovery";
 const bool distributed=partition||(argc==4&&std::string(argv[1])=="--distributed-finality");
 const bool crash=argc==4&&std::string(argv[1])=="--crash-runtime";
 const bool managed=crash||distributed||(argc==4&&std::string(argv[1])=="--managed-runtime");
 const bool exits_reorg=argc==3&&std::string(argv[1])=="--exit-reorg";
 const bool exits_pruning=argc==3&&std::string(argv[1])=="--exit-pruning";
 const bool exits_network=exits_reorg||exits_pruning||(argc==3&&std::string(argv[1])=="--exit-network");
 const bool exits_only=exits_network||(argc==2&&std::string(argv[1])=="--exit-loop");
 const bool pruning=argc==3&&std::string(argv[1])=="--slashing-pruning";
 const bool slashing_network=pruning||(argc==3&&std::string(argv[1])=="--slashing-network");
 const bool network=exits_network||slashing_network||(argc==3&&std::string(argv[1])=="--validator-network");
 const bool recovery=argc==2&&std::string(argv[1])=="--runtime-recovery";
 const bool operations=argc==2&&std::string(argv[1])=="--runtime-slashing";
 const bool runtime_only=operations||recovery||(argc==2&&std::string(argv[1])=="--runtime");
 const bool validator_only=deposit_loop||exits_only||network||(argc==2&&std::string(argv[1])=="--validator-loop");
 check(argc==1||deposit_store||validator_only||runtime_only||managed,"unknown Bellatrix qualification arguments");
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",refs=fixtures/"consensus-bellatrix-containers-official",source=fixtures/"consensus-bellatrix-slots-official";
 const auto manifest=load(source/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","initial state revision");
 for(const auto& row:manifest.at("cases"))if(row.at("name")=="slots_1")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(source/path))).substr(2)==info.at("sha256").get<std::string>(),"initial state digest");
 auto common=altair_transition_rules(fixtures);const auto preset=read(refs/"minimal-bellatrix.yaml");auto p=[&](const char* k){return parameter(preset,k);};
 for(const auto& row:load(refs/"manifest.json").at("presets"))check(hex(sha256(binary(refs/row.at("name").get<std::string>()))).substr(2)==row.at("sha256").get<std::string>(),"Bellatrix preset digest");
 common.epoch.rewards.inactivity_penalty_quotient=p("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.minimum_penalty_quotient=p("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX");common.epoch.slashing.proportional_slashing_multiplier=p("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX");
 const auto clock_file=fixtures/"consensus-altair-fork-head-official/minimal-config-v1.6.0-beta.0.yaml";
 const auto clock_manifest=load(fixtures/"consensus-altair-fork-head-official/manifest.json");
 check(hex(sha256(binary(clock_file))).substr(2)==clock_manifest.at("referenceFiles").at("minimal-config-v1.6.0-beta.0.yaml").at("sha256").get<std::string>(),"clock config digest");
 const auto seconds=parameter(read(clock_file),"SECONDS_PER_SLOT");
 // Explicit synthetic lifecycle profile: one epoch of tenure for this scenario.
 if(exits_only)common.exit.shard_committee_period=1;
 if(deposit_loop){common.epoch.epochs_per_eth1_voting_period=1;common.state.eth1_votes_limit=common.epoch.ffg.slots_per_epoch;}
 const BellatrixTransitionRules r{common,{uint32_t(p("MAX_EXTRA_DATA_BYTES")),uint32_t(p("MAX_BYTES_PER_TRANSACTION")),uint32_t(p("MAX_TRANSACTIONS_PER_PAYLOAD"))},4*1024*1024,seconds,1337,ReplayPolicy::RequireChainId};
 const auto& l=r.consensus.state;const auto& duties=r.consensus.attestation.duties;const uint64_t slots=duties.slots_per_epoch;
 const size_t hist=176+64*l.slots_per_historical_root,randao=hist+96,execution_field=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;
 BellatrixSlotState beacon{binary(source/"slots_1/pre.ssz"),std::nullopt};
 if(managed){const auto now=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();put(beacon.ssz,0,uint64_t(now)-2*seconds);}
 const auto genesis_time=little(beacon.ssz,0,8);const auto genesis_root=root_at(beacon.ssz,8);
 const auto key=bytes(Int(777),32);const std::string target="0x0000000000000000000000000000000000001234";
 Transaction tx;tx.type=2;tx.to=target;tx.gas=100000;tx.max_fee=2000000000;tx.priority_fee=100000000;
 const auto sender=SignedTransaction::sign(tx,key).sender();State state;state.get(sender).balance=Int("1000000000000000000");state.get(target).code=unhex("600054600101600055600143034060015500");
 const auto deposit_secret=bytes(Int(778),32);Transaction deposit_tx;deposit_tx.chain_id=1337;deposit_tx.to=ethereum_deposit_contract;deposit_tx.gas=1000000;deposit_tx.gas_price=2000000000;
 if(deposit_store||deposit_loop){
  const auto fixture=load(fixtures/"ethereum-blocks/for_prague--prague--eip6110_deposits--deposits--deposit.json");state.get(ethereum_deposit_contract)=State::from_json(fixture.begin().value().at("pre")).get(ethereum_deposit_contract);
  state.get(SignedTransaction::sign(deposit_tx,deposit_secret).sender()).balance=Int("1000000000000000000000");
  const auto empty_root=ExecutionDepositHistory(Bytes(32)).root();std::copy(empty_root.begin(),empty_root.end(),beacon.ssz.begin()+hist+4);put(beacon.ssz,hist+36,0);put(beacon.ssz,hist+80,0);
 }
 Block parent;parent.header.difficulty=0;parent.header.timestamp=Int(std::to_string(genesis_time));parent.header.state_root=state.root();
 Json managed_genesis;
 if(managed){managed_genesis={{"config",{{"chainId",1337},{"consensus","beacon"},{"evmFork","Paris"}}},{"timestamp",quantity(Int(std::to_string(genesis_time)))},{"alloc",state.to_json()}};parent=Genesis::from_json(managed_genesis).block;}
 const auto initial_header=encode_bellatrix_execution_header(bellatrix_execution_payload_header(payload(parent),r.execution),r.execution.extra_data_bytes,r.maximum_state_bytes);
 beacon.ssz.resize(little(beacon.ssz,execution_field,4));beacon.ssz.insert(beacon.ssz.end(),initial_header.begin(),initial_header.end());
 std::map<Int,Bytes> hashes{{0,parent.header.hash()}};std::function<Bytes(const Int&)> history=[&](const Int& n){return hashes.at(n);};
 BellatrixForkBlockRules fork_rules{r,seconds*1000,seconds*1000/3,40};
 if(deposit_store||deposit_loop){fork_rules.native_deposit_votes=deposit_loop;fork_rules.deposit_contract_code_hash=keccak(state.get(ethereum_deposit_contract).code);const auto encoded=bellatrix_fork_profile(fork_rules).dump();check(load_bellatrix_fork_profile(encoded,sha256(Bytes(encoded.begin(),encoded.end())),encoded.size()).deposit_contract_code_hash==fork_rules.deposit_contract_code_hash,"deposit profile roundtrip");}
 const Block execution_genesis=parent;const State genesis_state=state;std::vector<BellatrixForkStore> reorg_stores;
 BellatrixStoreSnapshotLimits reactor_limits{32*1024*1024,32,128,128,4*1024*1024,8*1024*1024,64,100000};
 if(deposit_activation){reactor_limits.envelope_bytes=128*1024*1024;reactor_limits.blocks=160;reactor_limits.messages=512;reactor_limits.total_state_bytes=64*1024*1024;}
 Temp reactor_location;Genesis deposit_genesis;std::unique_ptr<Blockchain> deposit_chain;auto reactor_disk=std::make_unique<ChainStore>(reactor_location/"chain");reactor_disk->bind_chain(1337,execution_genesis.header.hash(),"bellatrix-reactor-test");reactor_disk->commit(execution_genesis,genesis_state,{});std::unique_ptr<BellatrixDurableReactor> reactor;
 std::map<uint64_t,std::unique_ptr<ChainStore>> signer_disks;std::map<uint64_t,std::unique_ptr<BeaconSigningJournal>> signers;
 auto local_journal=[&](uint64_t index)->BeaconSigningJournal&{if(!signers.contains(index)){signer_disks[index]=std::make_unique<ChainStore>(reactor_location/("key-"+std::to_string(index)));signers[index]=std::make_unique<BeaconSigningJournal>(*signer_disks[index],1337,execution_genesis.header.hash(),genesis_root,Key(index).public_key,slots);}return *signers.at(index);};
 size_t local_proposals=0,local_attestations=0,signature_refusals=0;
 InboundBudget wire_budget;std::unique_ptr<BellatrixConsensusService> service;size_t wire_messages=0,wire_refusals=0;
 auto deliver=[&](const char* kind,const Bytes& bytes){Json frame=reactor->network_identity();frame["kind"]=kind;frame["ssz"]=hex(bytes);std::promise<BellatrixConsensusService::Decision> completed;auto result=completed.get_future();auto cancelled=std::make_shared<std::atomic<bool>>(false);auto lease=wire_budget.acquire("fixture-"+std::to_string(wire_messages),InboundBudget::Clock::now(),InboundBudget::Traffic::bulk);check(bool(lease),"wire fixture ingress limited");check(service->submit(frame.dump(),cancelled,[&](auto d){completed.set_value(d);}),"wire service refused valid envelope");check(result.wait_for(std::chrono::seconds(10))==std::future_status::ready,"wire worker did not finish");++wire_messages;return result.get()==BellatrixConsensusService::Decision::accepted;};

 std::optional<BellatrixForkStore> store;size_t reorgs=0,store_cancellations=0;
 size_t blocks=0,refusals=0,cancellations=0;Bytes first_proof;uint64_t offender=0;
 for(uint64_t slot=1;slot<=slots+1;++slot){
  const auto old_beacon=beacon;const auto old_state=state.root();
  const auto advanced=prepare_bellatrix_slots(beacon,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l,slot,64,100000);
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
  tx.nonce=Int(blocks);execution.transactions.push_back(SignedTransaction::sign(tx,key));
  if(deposit_store&&slot>1){
   deposit_tx.data=staking_calldata(64+slot,r.consensus.deposit.genesis_fork_version);deposit_tx.value=Int("32000000000000000000");deposit_tx.nonce=Int(std::to_string(slot-2));execution.transactions.push_back(SignedTransaction::sign(deposit_tx,deposit_secret));
  }
  State expected=state;ExecutionEngine(r.chain_id,history,EvmRevision::Paris,r.replay_policy).execute_block(expected,execution,false);
  const auto encoded=encode_bellatrix_execution_payload(payload(execution),r.execution,r.consensus.maximum_block_bytes);
  const size_t sync_bytes=(l.sync_committee_size+7)/8+96,fixed=224+sync_bytes;
  auto body=[&](const Bytes& proof){Bytes b(fixed,0);std::copy(reveal.begin(),reveal.end(),b.begin());std::copy_n(advanced.ssz.begin()+hist+4,72,b.begin()+96);b[220+(l.sync_committee_size+7)/8]=0xc0;put(b,200,fixed,4);for(size_t at:std::array<size_t,5>{204,208,212,216,220+sync_bytes})put(b,at,fixed+proof.size(),4);b.insert(b.end(),proof.begin(),proof.end());b.insert(b.end(),encoded.begin(),encoded.end());return b;};
  const auto contents=body(slot==slots+1?first_proof:Bytes{});
  const auto built=prepare_bellatrix_unsigned_block(beacon,slot,proposer,parent_root,contents,r,parent.header,state,history,64,100000);check(built.has_value(),"Bellatrix construction rejected");
  auto sign_message=[&](Bytes message,uint64_t index=UINT64_MAX){const auto owner=index==UINT64_MAX?proposer:index;Key k(owner);const auto bytes=std::span<const uint8_t>(message);const ConsensusBeaconHeader h{little(message,0,8),little(message,8,8),root_at(message,16),root_at(message,48),bellatrix_beacon_body_root(bytes.subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.consensus.maximum_block_bytes)};const auto sig=k.sign(consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},version,genesis_root)));Bytes wire(100,0);put(wire,0,100,4);std::copy(sig.begin(),sig.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());return wire;};
  auto wire=sign_message(built->message_ssz);size_t calls=0;
  if(slot>1){
   const auto now=(genesis_time+slot*seconds)*1000;reactor->tick(now);const auto commitment=reactor->current().token;auto& journal=local_journal(proposer);size_t signature_calls=0;
   check(reactor->local_proposer(slot,commitment)==proposer,"batched proposer differs from reference selection");
   {auto stale=commitment;++stale.sequence;bool classified=false;try{reactor->local_proposer(slot,stale);}catch(const StaleBellatrixDuty&){classified=true;}check(classified,"stale duty lacks explicit retry classification");}

   auto signer=[&](const ConsensusRoot& root){++signature_calls;return signing.sign(root);};
   const auto randao_signature=reactor->sign_randao(proposer,slot,[&]{return now;},commitment,journal,signer);check(randao_signature&&*randao_signature==reveal,"local RANDAO differs");
   const auto local=reactor->sign_proposal(proposer,slot,contents,[&]{return now;},commitment,journal,signer);check(local&&*local==wire,"local signed Bellatrix block differs");++local_proposals;
   if(slot==2){
    auto refuse=[&](auto action){const auto before=signature_calls;bool rejected=false;try{rejected=!action().has_value();}catch(const std::exception&){rejected=true;}check(rejected&&signature_calls==before,"unsafe local signature emitted");++signature_refusals;};
    auto stale=commitment;++stale.sequence;refuse([&]{return reactor->sign_proposal(proposer,slot,contents,[&]{return now;},stale,journal,signer);});
    refuse([&]{return reactor->sign_proposal(proposer,slot,contents,[&]{return now+seconds*1000;},commitment,journal,signer);});
    auto conflicting=contents;conflicting[168]^=1;refuse([&]{return reactor->sign_proposal(proposer,slot,conflicting,[&]{return now;},commitment,journal,signer);});
    const auto saved=journal.commitment();signers.erase(proposer);signer_disks.erase(proposer);signer_disks[proposer]=std::make_unique<ChainStore>(reactor_location/("key-"+std::to_string(proposer)));signers[proposer]=std::make_unique<BeaconSigningJournal>(*signer_disks[proposer],1337,execution_genesis.header.hash(),genesis_root,signing.public_key,slots,saved);
    refuse([&]{return reactor->sign_proposal(proposer,slot,conflicting,[&]{return now;},commitment,*signers.at(proposer),signer);});
    uint64_t live_clock=now;bool expired=false;try{reactor->sign_proposal(proposer,slot,contents,[&]{return live_clock;},commitment,*signers.at(proposer),[&](const ConsensusRoot& root){auto signature=signing.sign(root);live_clock+=seconds*1000;return signature;});}catch(const std::runtime_error&){expired=true;}check(expired,"proposal escaped after signing provider crossed slot boundary");++signature_refusals;

   }
  }

  const auto received=prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,64,100000,[&]{++calls;});
  check(received&&received->consensus==built->post.consensus&&received->execution.state.root()==expected.root(),"Bellatrix signed transition differs");
  check(received->execution.state.get_storage(target,0)==Int(blocks+1)&&received->execution.receipts[0].success,"EVM write missing");
  check(received->execution.state.get_storage(target,1)==integer(parent.header.hash()),"BLOCKHASH did not use parent branch");
  if(slot==1){
   store=initialize_bellatrix_fork_store(received->consensus,built->message_ssz,received->execution.block,received->execution.state,received->execution.receipts,{parent.header},fork_rules);
   if(managed){test::bellatrix_managed_runtime_scenario(managed_genesis,fork_rules,reactor_limits,*store,argv[2],argv[3],distributed,partition,crash);return 0;}
   if(runtime_only){
    Genesis g;g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Paris;g.block=execution_genesis;g.state=genesis_state;
    test::bellatrix_runtime_scenario(g,fork_rules,reactor_limits,*store,sender,recovery,operations?std::function<Bytes(uint64_t,const ConsensusRoot&)>([](uint64_t index,const ConsensusRoot& root){Key key(index);return key.sign(root);}):std::function<Bytes(uint64_t,const ConsensusRoot&)>{});if(recovery)test::bellatrix_core_recovery_scenario(g,fork_rules,reactor_limits,*store);return 0;
   }
   if(validator_only){
    Temp live;Genesis g;g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Paris;g.block=execution_genesis;g.state=genesis_state;
    ForkCheckpointToken saved;Bytes final_hash,final_state;uint64_t finalized=0;Json loop_status;
    Transaction pending_tx=tx;pending_tx.nonce=1;const auto pending=SignedTransaction::sign(pending_tx,key);
    {
     Blockchain chain(live/"chain",g);BellatrixDurableReactor bound(chain,fork_rules,reactor_limits,*store);
     std::atomic<uint64_t> wall=store->timed.clock.time_ms;BellatrixLocalClock clock(bound,seconds*1000,[&]{return wall.load();});
     std::vector<std::unique_ptr<ChainStore>> journals;std::vector<BellatrixValidatorKey> keys;
     size_t entrant_signatures=0;std::shared_ptr<BeaconSigningJournal> entrant_journal;
     for(uint64_t i=0;i<(deposit_activation?65u:64u);++i){
      auto signer=std::make_shared<Key>(i);journals.push_back(std::make_unique<ChainStore>(live/("signer-"+std::to_string(i))));
      auto journal=std::make_shared<BeaconSigningJournal>(*journals.back(),r.chain_id,execution_genesis.header.hash(),genesis_root,signer->public_key,slots);
      if(i==64)entrant_journal=journal;
      keys.push_back({i,std::move(journal),[signer,i,&entrant_signatures](const ConsensusRoot& root){if(i==64)++entrant_signatures;return signer->sign(root);},sender});
     }
     BellatrixValidatorLoop loop(bound,clock,seconds*1000,slots,128,std::move(keys),[&]{return wall.load();});
     std::unique_ptr<BellatrixNetworkPair> peers;
     if(network)peers=std::make_unique<BellatrixNetworkPair>(live.path,argv[2],g,fork_rules,reactor_limits,*store,chain,bound,clock,wall);
     if(deposit_loop){
      deposit_tx.nonce=0;deposit_tx.value=Int("32000000000000000000");deposit_tx.data=staking_calldata(64,r.consensus.deposit.genesis_fork_version);const auto funded=SignedTransaction::sign(deposit_tx,deposit_secret);check(chain.submit(funded),"staking transaction not admitted");
      uint64_t admitted_slot=0,activation_epoch=UINT64_MAX,finalized_before_activation=0;bool malicious_refused=false,activation_qualified=false;
      for(uint64_t next_slot=2;next_slot<=(deposit_activation?16:2)*slots;++next_slot){
       wall=(genesis_time+next_slot*seconds)*1000;
       if(next_slot==3){
        bound.tick(wall);const auto current=bound.current();const auto proposer=bound.current_proposer(next_slot);check(proposer.has_value(),"deposit test proposer missing");Key signer(*proposer);const auto& head=bellatrix_fork_head(*current.state,fork_rules);const auto context=bellatrix_fork_block_context(head.state,head.header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l);const uint64_t epoch=next_slot/slots;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;ConsensusRoot epoch_root{};for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));const auto reveal=signer.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,genesis_root)));
        auto body=bound.prepare_local_body(*proposer,next_slot,reveal,sender,current.token,128);check(body.has_value(),"native deposit proposal failed");(*body)[96]^=1;
        ChainStore rejected_disk(live/"reject-signer");BeaconSigningJournal rejected_journal(rejected_disk,1337,execution_genesis.header.hash(),genesis_root,signer.public_key,slots);size_t signatures=0;
        check(!bound.sign_proposal(*proposer,next_slot,*body,[&]{return wall.load();},current.token,rejected_journal,[&](const ConsensusRoot& root){++signatures;return signer.sign(root);})&&signatures==0,"unbacked deposit vote reached signer");
        const auto fabricated=prepare_bellatrix_unsigned_block(head.state,next_slot,*proposer,consensus_beacon_header_root(head.header),*body,r,head.execution_block.header,head.execution_state,[&](const Int& height){return bellatrix_execution_history(*current.state,consensus_beacon_header_root(head.header),height);},64,100000);check(fabricated.has_value(),"malicious vote must be structurally valid for admission test");
        const ConsensusBeaconHeader h{next_slot,*proposer,consensus_beacon_header_root(head.header),root_at(fabricated->message_ssz,48),bellatrix_beacon_body_root(std::span<const uint8_t>(fabricated->message_ssz).subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.consensus.maximum_block_bytes)};const auto signature=signer.sign(consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},version,genesis_root)));Bytes wire(100);put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),fabricated->message_ssz.begin(),fabricated->message_ssz.end());check(!bound.block(wire)&&bound.current().token==current.token,"unbacked signed deposit vote published");malicious_refused=true;
       }
       if(next_slot==slots+slots/2){
        bound.tick(wall);const auto current=bound.current();const auto proposer=bound.current_proposer(next_slot);check(proposer.has_value(),"mandatory deposit proposer missing");Key signer(*proposer);const auto& head=bellatrix_fork_head(*current.state,fork_rules);const auto context=bellatrix_fork_block_context(head.state,head.header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l);const uint64_t epoch=next_slot/slots;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;ConsensusRoot epoch_root{};for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));const auto reveal=signer.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,genesis_root)));
        auto body=bound.prepare_local_body(*proposer,next_slot,reveal,sender,current.token,128);check(body.has_value()&&little(*body,216,4)-little(*body,212,4)==1240,"majority proposal lacks mandatory deposit proof");(*body)[little(*body,212,4)]^=1;
        ChainStore rejected_disk(live/"reject-deposit-proof");BeaconSigningJournal rejected_journal(rejected_disk,1337,execution_genesis.header.hash(),genesis_root,signer.public_key,slots);size_t signatures=0;check(!bound.sign_proposal(*proposer,next_slot,*body,[&]{return wall.load();},current.token,rejected_journal,[&](const ConsensusRoot& root){++signatures;return signer.sign(root);})&&signatures==0&&bound.current().token==current.token,"forged mandatory deposit proof reached signer");
       }
       loop.poll();check(chain.tip().header.number==Int(std::to_string(next_slot)),"deposit validator loop stalled");
       const auto current=bound.current();const auto& head=bellatrix_fork_head(*current.state,fork_rules);const auto registry=bellatrix_fork_block_context(head.state,head.header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l).registry;
       if(registry.validators.size()==65&&!admitted_slot)admitted_slot=next_slot;
       if(deposit_activation){
        const auto member=registry.validators.find(64);const bool active=member!=registry.validators.end()&&member->second.activation_epoch<=next_slot/slots;
        if(!active)check(entrant_signatures==0,"new key signed before activation");
        else {
         if(activation_epoch==UINT64_MAX){activation_epoch=member->second.activation_epoch;finalized_before_activation=current.state->timed.view.finalized.epoch;}
         if(entrant_journal->latest_attestation_target()&&current.state->timed.view.latest_messages.contains(64)&&current.state->timed.view.finalized.epoch>finalized_before_activation){activation_qualified=true;saved=current.token;final_hash=chain.tip().header.hash();final_state=chain.state().root();finalized=current.state->timed.view.finalized.epoch;break;}
        }
        if(next_slot%slots==0)std::cout<<Json{{"activationProgressSlot",next_slot},{"activationEpoch",member==registry.validators.end()?UINT64_MAX:member->second.activation_epoch},{"finalizedEpoch",current.state->timed.view.finalized.epoch},{"entrantSignatures",entrant_signatures}}.dump()<<std::endl;
       }
       if(next_slot<=slots+slots/2-1)check(registry.validators.size()==64,"deposit admitted before strict voting majority");
       if(next_slot==2*slots){
        check(registry.validators.size()==65&&admitted_slot==slots+slots/2&&malicious_refused,"native deposit admission timing");const auto& member=registry.validators.at(64);check(Bytes(member.public_key.begin(),member.public_key.end())==Key(64).public_key&&member.balance==32000000000&&member.activation_eligibility_epoch!=UINT64_MAX&&member.activation_epoch==UINT64_MAX,"new validator state/activation queue mismatch");
        check(little(head.state.ssz,hist+80,8)==1&&head.deposit_history->count()==1&&chain.store().receipt(funded.hash()).has_value()&&chain.store().receipt(funded.hash())->success,"staking deposit missing from execution or Beacon cursor");
        const auto before=current.token;auto included=std::find_if(current.signed_blocks->begin(),current.signed_blocks->end(),[&](const auto& entry){return little(*entry.second,100,8)==admitted_slot;});check(included!=current.signed_blocks->end(),"missing signed admission block");const auto& wire=*included->second;check(bound.block(wire)&&bound.current().token==before,"duplicate staking block changed checkpoint");
        saved=current.token;final_hash=chain.tip().header.hash();final_state=chain.state().root();
       }
      }
      loop.stop();
      if(deposit_activation){check(activation_qualified&&entrant_signatures>0,"new validator activation/participation not qualified within bounded run");std::cout<<Json{{"scenario","bellatrix-deposit-activation"},{"activationEpoch",activation_epoch},{"entrantSignatures",entrant_signatures},{"finalizedEpoch",finalized},{"attestationAdmitted",true}}.dump()<<std::endl;}
      std::cout<<Json{{"scenario","bellatrix-native-deposit-admission"},{"admissionSlot",admitted_slot},{"validators",65},{"activationEligible",true},{"activated",activation_qualified},{"unbackedVoteRejected",malicious_refused}}.dump()<<'\n';
     }else
     if(exits_only){
      Key exiting(0);SignedConsensusVoluntaryExit request{0,0,{}};
      auto sign_exit=[&](SignedConsensusVoluntaryExit& exit){const auto view=bound.current();const auto& h=bellatrix_fork_head(*view.state,fork_rules);const auto& t=fork_rules.transition.consensus;const auto ctx=bellatrix_fork_block_context(h.state,h.header,{t.epoch,fork_rules.transition.execution,fork_rules.transition.maximum_state_bytes},t.state);const auto version=exit.epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;exit.signature=Key(exit.validator_index).sign(consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,exit.validator_index),consensus_domain({4,0,0,0},version,ctx.genesis_root)));};
      sign_exit(request);check(!bound.voluntary_exit_ssz(encode_consensus_voluntary_exit_ssz(request)),"exit accepted before minimum active tenure");
      for(uint64_t slot=2;slot<=slots;++slot){wall=(genesis_time+slot*seconds)*1000;loop.poll();if(peers)peers->sync(chain.tip().header.hash());}
      request.epoch=1;sign_exit(request);const auto wire=encode_consensus_voluntary_exit_ssz(request);
      const auto decoded=decode_consensus_voluntary_exit_ssz(wire);check(decoded&&decoded->epoch==request.epoch&&decoded->validator_index==request.validator_index&&decoded->signature==request.signature,"exit SSZ round trip");
      auto invalid=wire;invalid.back()^=1;const auto before=bound.current().token;
      check(!bound.voluntary_exit_ssz(invalid)&&bound.current().token==before,"invalid exit changed checkpoint");
      auto future=request;future.epoch=2;sign_exit(future);check(!bound.voluntary_exit_ssz(encode_consensus_voluntary_exit_ssz(future)),"future exit admitted");
      check(bound.voluntary_exit_ssz(wire)&&bound.current().signed_exits->size()==1,"eligible exit not archived");
      if(peers)until([&]{return peers->reactor->current().signed_exits->size()==1;},"voluntary exit received before block",20);
      const auto admitted=bound.current().token;check(bound.voluntary_exit_ssz(wire)&&bound.current().token==admitted,"duplicate exit amplified checkpoint");
      const auto disk=chain.store().fork_checkpoint(r.chain_id,execution_genesis.header.hash(),reactor_limits.envelope_bytes,admitted);
      check(disk&&Json::parse(disk->snapshot).at("format")=="nano-bellatrix-durable-v5","pending exit not in durable snapshot");
      const auto& t=fork_rules.transition.consensus;auto context=[&]{const auto v=bound.current();const auto& h=bellatrix_fork_head(*v.state,fork_rules);return bellatrix_fork_block_context(h.state,h.header,{t.epoch,fork_rules.transition.execution,fork_rules.transition.maximum_state_bytes},t.state);};
      check(context().registry.validators.at(0).exit_epoch==UINT64_MAX,"admission mutated registry before block");
      if(exits_reorg){
       const auto next_slot=slots+1;wall=(genesis_time+next_slot*seconds)*1000;clock.poll();
       const auto view=bound.current();const auto& parent=bellatrix_fork_head(*view.state,fork_rules);const auto parent_root=consensus_beacon_header_root(parent.header);
       const BellatrixEpochRules er{t.epoch,fork_rules.transition.execution,fork_rules.transition.maximum_state_bytes};
       const auto identity=prepare_bellatrix_proposer_identity(parent.state,parent.header,next_slot,er,t.state,t.attestation.duties,reactor_limits.replay_slots,reactor_limits.sync_trials);check(bool(identity),"missing reorg proposer");
       Key proposer_key(identity->index);const auto ctx=context();const auto epoch=next_slot/slots;ConsensusRoot epoch_root{};for(unsigned i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));
       const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
       const auto reveal=proposer_key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,ctx.genesis_root)));
       const auto body=bound.prepare_local_body(identity->index,next_slot,reveal,sender,view.token,128);check(bool(body),"missing exit body");
       auto build=[&](const Bytes& contents){
        auto history=[&](const Int& n){return bellatrix_execution_history(*view.state,parent_root,n);};
        const auto built=prepare_bellatrix_unsigned_block(parent.state,next_slot,identity->index,parent_root,contents,fork_rules.transition,parent.execution_block.header,parent.execution_state,history,reactor_limits.replay_slots,reactor_limits.sync_trials);check(bool(built),"invalid reorg candidate");
        const auto& message=built->message_ssz;const ConsensusBeaconHeader h{little(message,0,8),little(message,8,8),root_at(message,16),root_at(message,48),bellatrix_beacon_body_root(std::span<const uint8_t>(message).subspan(84),t.body,t.state.sync_committee_size,fork_rules.transition.execution,t.maximum_block_bytes)};
        const auto root=consensus_beacon_header_root(h);const auto signature=proposer_key.sign(consensus_signing_root(root,consensus_domain({0,0,0,0},version,ctx.genesis_root)));
        Bytes signed_block(100);put(signed_block,0,100,4);std::copy(signature.begin(),signature.end(),signed_block.begin()+4);signed_block.insert(signed_block.end(),message.begin(),message.end());return std::pair(root,signed_block);
       };
       const auto canonical=build(*body);check(bound.block(canonical.second),"canonical exit block refused");peers->sync(chain.tip().header.hash());check(context().registry.validators.at(0).exit_epoch!=UINT64_MAX,"canonical exit not applied");
       // Synthetic equivocating proposer: remove the exit and vary graffiti to
       // obtain the higher-root sibling used by the specified tie-break rule.
       auto sibling=*body;const auto payload_field=220+(t.state.sync_committee_size+7)/8+96;
       const auto exit_begin=little(sibling,216,4),payload_begin=little(sibling,payload_field,4);check(payload_begin-exit_begin==112,"expected one pending exit");
       sibling.erase(sibling.begin()+exit_begin,sibling.begin()+payload_begin);put(sibling,payload_field,exit_begin,4);
       std::optional<std::pair<ConsensusRoot,Bytes>> alternative;
       for(uint64_t nonce=1;nonce<=64;++nonce){put(sibling,168,nonce);auto built=build(sibling);if(built.first>canonical.first){alternative=std::move(built);break;}}
       check(bool(alternative)&&bound.block(alternative->second),"higher-root competing block refused");
       wall=(genesis_time+(next_slot+1)*seconds)*1000;clock.poll();
       check(consensus_beacon_header_root(bellatrix_fork_head(*bound.current().state,fork_rules).header)==alternative->first,"fork choice did not switch to sibling");
       check(context().registry.validators.at(0).exit_epoch==UINT64_MAX&&bound.current().signed_exits->size()==1,"reorg lost pending exit or failed registry rollback");peers->sync(chain.tip().header.hash());
       loop.poll();peers->sync(chain.tip().header.hash());
       check(context().registry.validators.at(0).exit_epoch!=UINT64_MAX,"pending exit not reincluded after reorg");
       std::cout<<Json{{"scenario","bellatrix-exit-reorg"},{"syntheticEquivocatingProposer",true},{"exitRolledBack",true},{"pendingRequestRetained",true},{"reincluded",true}}.dump()<<'\n';
      }else{wall=(genesis_time+(slots+1)*seconds)*1000;loop.poll();}
      const auto member=context().registry.validators.at(0);
      check(member.exit_epoch!=UINT64_MAX&&!member.slashed,"block did not schedule voluntary exit");
      check(bound.current().signed_exits->size()==1,"unfinalized exit proof prematurely dropped");
      auto pending_wire=wire;
      if(exits_pruning){
       bool finalized_exit=false;
       for(uint64_t slot=slots+2;slot<=5*slots;++slot){
        wall=(genesis_time+slot*seconds)*1000;loop.poll();peers->sync(chain.tip().header.hash());
        for(auto* reactor:{&bound,peers->reactor.get()}){
         const auto view=reactor->current();const auto& finalized=view.state->blocks.at(view.state->timed.view.finalized.root);
         const auto ctx=bellatrix_fork_block_context(finalized.state,finalized.header,{t.epoch,fork_rules.transition.execution,fork_rules.transition.maximum_state_bytes},t.state);
         const bool scheduled=ctx.registry.validators.at(0).exit_epoch!=UINT64_MAX;
         check(view.signed_exits->size()==(scheduled?0:1),"pending exit lifetime differs from finalized scheduling");
         if(reactor==&bound)finalized_exit=scheduled;
        }
        if(finalized_exit)break;
       }
       check(finalized_exit,"voluntary exit did not finalize");const auto token=bound.current().token;
       check(bound.voluntary_exit_ssz(wire)&&bound.current().token==token&&bound.current().signed_exits->empty(),"finalized exit replay recreated archive");
       SignedConsensusVoluntaryExit fresh{1,1,{}};sign_exit(fresh);pending_wire=encode_consensus_voluntary_exit_ssz(fresh);
       check(bound.voluntary_exit_ssz(pending_wire),"new exit refused after pruning");
       until([&]{const auto view=peers->reactor->current();return view.signed_exits->size()==1&&view.signed_exits->begin()->second->ssz==pending_wire;},"new exit relayed after pruned cursor",20);
       std::cout<<Json{{"scenario","bellatrix-exit-finalized-pruning"},{"retainedUntilFinality",true},{"pruned",true},{"replayDidNotPublish",true},{"newExitRelayed",true}}.dump()<<'\n';
      }
      if(peers){
       peers->sync(chain.tip().header.hash());peers->stop();
       const auto view=peers->reactor->current();const auto token=view.token;const auto hash=peers->observer->tip().header.hash();
       check(view.signed_exits->size()==1&&view.signed_exits->begin()->second->ssz==pending_wire,"peer archive differs from signed exit");
       if(!exits_pruning)check(view.signed_exits->begin()->second->admission_root!=consensus_beacon_header_root(bellatrix_fork_head(*view.state,fork_rules).header),"exit inclusion did not advance head");
       peers->hb.reset();peers->sb.reset();peers->rb.reset();peers->eb.reset();peers->clock.reset();peers->reactor.reset();peers->observer.reset();
       Blockchain reopened(live/"observer",g);BellatrixDurableReactor restored(reopened,fork_rules,reactor_limits,token);
       check(restored.current().signed_exits->size()==1&&reopened.tip().header.hash()==hash,"reopen lost exit or execution head");
       const auto snapshot=restored.current();const auto& h=bellatrix_fork_head(*snapshot.state,fork_rules);
       const auto ctx=bellatrix_fork_block_context(h.state,h.header,{t.epoch,fork_rules.transition.execution,fork_rules.transition.maximum_state_bytes},t.state);
       check(ctx.registry.validators.at(0).exit_epoch==member.exit_epoch,"peer exit schedule differs after reopen");
       check(restored.voluntary_exit_ssz(wire)&&restored.current().token==token,"reopened pending duplicate published again");
       std::cout<<Json{{"scenario","bellatrix-network-voluntary-exit"},{"p2pDaemons",2},{"cores",2},{"receivedBeforeInclusion",true},{"reopened",true},{"scheduledExitEpoch",member.exit_epoch}}.dump()<<'\n';
      }

      std::cout<<Json{{"scenario","bellatrix-local-voluntary-exit"},{"syntheticTenureEpochs",1},{"invalidSignatureRejected",true},{"futureEpochRejected",true},{"duplicateDidNotPublish",true},{"durableArchive",true},{"scheduledByBlock",true},{"exitEpoch",member.exit_epoch},{"withdrawableEpoch",member.withdrawable_epoch},{"payoutImplemented",false},{"networkAdmissionQualified",exits_network}}.dump()<<'\n';return 0;
     }
     if(slashing_network){
      wall=(genesis_time+2*seconds)*1000;loop.poll();peers->sync(chain.tip().header.hash());
      const auto current=bound.current();const auto& parent=bellatrix_fork_head(*current.state,fork_rules);
      const auto& t=fork_rules.transition.consensus;const BellatrixEpochRules er{t.epoch,fork_rules.transition.execution,fork_rules.transition.maximum_state_bytes};
      const auto context=bellatrix_fork_block_context(parent.state,parent.header,er,t.state);
      const auto proposer=prepare_bellatrix_proposer_identity(parent.state,parent.header,3,er,t.state,t.attestation.duties,reactor_limits.replay_slots,reactor_limits.sync_trials);
      check(bool(proposer),"missing next proposer");const auto offender=(proposer->index+1)%64;
      const auto balance=context.registry.validators.at(offender).balance;
      IndexedConsensusAttestation a{{parent.header.slot,0,{}, {context.registry.epoch,{}},{context.registry.epoch,{}}},{offender},{}},b=a;b.data.beacon_block_root[0]=1;
      const auto version=context.registry.epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;
      Key malicious(offender);for(auto* vote:{&a,&b})vote->signature=malicious.sign(consensus_signing_root(consensus_attestation_root(vote->data),consensus_domain({1,0,0,0},version,context.genesis_root)));
      const auto proof=encode_consensus_attester_slashing_ssz(a,b,t.body.validators_per_committee);
      check(peers->reactor->current().signed_slashings->empty(),"observer already has proof");
      check(bound.slashing_ssz(proof),"source rejected proof");
      until([&]{return peers->reactor->current().signed_slashings->size()==1;},"signed proof P2P propagation before block inclusion",20);
      const auto received=peers->reactor->current();
      check(*received.signed_slashings->begin()->second==proof&&received.state->timed.view.equivocating_indices.contains(offender),"observer changed proof or failed exclusion");
      check(peers->observer->tip().header.number==2,"proof relay unexpectedly published a block");
      wall=(genesis_time+3*seconds)*1000;loop.poll();peers->sync(chain.tip().header.hash());
      const auto after=peers->reactor->current();const auto& head=bellatrix_fork_head(*after.state,fork_rules);
      const auto result=bellatrix_fork_block_context(head.state,head.header,er,t.state);
      const auto& member=result.registry.validators.at(offender);
      check(member.slashed&&member.exit_epoch!=UINT64_MAX&&member.balance<balance,"observer failed economic slashing transition");
      check(peers->observer->state().root()==chain.state().root(),"execution state differs after slashing");
      if(pruning){
       check(bound.current().signed_slashings->size()==1&&after.signed_slashings->size()==1,"head-only sanction discarded reorg evidence");
       bool finalized_slashing=false;
       for(uint64_t slot=4;slot<=4*slots;++slot){
        wall=(genesis_time+slot*seconds)*1000;loop.poll();peers->sync(chain.tip().header.hash());
        for(auto* reactor:{&bound,peers->reactor.get()}){
         const auto view=reactor->current();const auto& finalized=view.state->blocks.at(view.state->timed.view.finalized.root);
         const auto ctx=bellatrix_fork_block_context(finalized.state,finalized.header,er,t.state);
         const bool slashed=ctx.registry.validators.at(offender).slashed;
         check(view.signed_slashings->size()==(slashed?0:1),"proof lifetime differs from finalized sanction");
         if(reactor==&bound)finalized_slashing=slashed;
        }
        if(finalized_slashing)break;
       }
       check(finalized_slashing,"slashing did not finalize before scenario limit");
       const auto before=bound.current().token;
       check(bound.slashing_ssz(proof)&&bound.current().signed_slashings->empty()&&bound.current().token==before,"finalized proof replay recreated archive or checkpoint");
       auto fresh_a=a,fresh_b=b;const auto fresh_offender=(offender+1)%64;Key fresh_key(fresh_offender);
       for(auto* vote:{&fresh_a,&fresh_b}){vote->indices={fresh_offender};vote->signature=fresh_key.sign(consensus_signing_root(consensus_attestation_root(vote->data),consensus_domain({1,0,0,0},version,context.genesis_root)));}
       const auto fresh_proof=encode_consensus_attester_slashing_ssz(fresh_a,fresh_b,t.body.validators_per_committee);
       check(bound.slashing_ssz(fresh_proof)&&bound.current().signed_slashings->size()==1,"archive rejected new offender after pruning");
       until([&]{const auto view=peers->reactor->current();return view.signed_slashings->size()==1&&*view.signed_slashings->begin()->second==fresh_proof;},"new proof relay after pruned cursor",20);
       std::cout<<Json{{"scenario","bellatrix-finalized-proof-pruning"},{"retainedBeforeFinality",true},{"prunedAfterFinality",true},{"replayDidNotRearchive",true},{"twoCores",true}}.dump()<<'\n';
      }
      peers->stop();
      const auto observer_token=peers->reactor->current().token;
      const auto observer_head=peers->observer->tip().header.hash();
      if(pruning){
       peers->hb.reset();peers->sb.reset();peers->rb.reset();peers->eb.reset();
       peers->clock.reset();peers->reactor.reset();peers->observer.reset();
       Blockchain reopened(live/"observer",g);
       BellatrixDurableReactor restored(reopened,fork_rules,reactor_limits,observer_token);
       check(restored.current().signed_slashings->size()==1&&reopened.tip().header.hash()==observer_head,"restart lost new proof or execution head");
       check(restored.slashing_ssz(proof)&&restored.current().token==observer_token,"restarted node rearchived finalized evidence");
       std::cout<<Json{{"scenario","bellatrix-pruning-reopen"},{"checkpointPreserved",true},{"replayDidNotRearchive",true}}.dump()<<'\n';
      }
      std::cout<<Json{{"scenario","bellatrix-slashing-network"},{"realP2pDaemons",2},{"cores",2},{"independentCoreProcesses",false},{"syntheticClock",true},{"proofReceivedBeforeBlock",true},{"offender",offender},{"slashed",true},{"balanceBefore",balance},{"balanceAfter",member.balance},{"exitEpoch",member.exit_epoch},{"executionStateMatched",true}}.dump()<<'\n';return 0;
     }
     if(!deposit_loop){
     if(peers)check(rpc(peers->sa->port(),"eth_sendRawTransaction",Json::array({hex(pending.raw())}))==hex(pending.hash()),"wallet RPC transaction admission");
     else check(chain.submit(pending),"validator scenario wallet transaction admitted");
     for(uint64_t next_slot=2;next_slot<=4*slots;++next_slot){
      if(next_slot==3*slots+1){auto later_tx=pending_tx;later_tx.nonce=2;check(chain.submit(SignedTransaction::sign(later_tx,key)),"post-checkpoint transaction admission");}
      wall=(genesis_time+next_slot*seconds)*1000;loop.poll();
      check(chain.tip().header.number==Int(std::to_string(next_slot)),"validator loop missed expected proposal");
      if(next_slot==slots){const auto queued=bound.current();check(!queued.signed_votes->empty()&&queued.state->timed.view.latest_messages.empty(),"current-slot votes applied before their eligibility slot");}
      if(next_slot==slots+1)check(!bound.current().state->timed.view.latest_messages.empty(),"deferred votes not applied on the next slot");
      // FFG intentionally skips its first two epochs; at this boundary the
      // targets are justified, with finalization checked after the next epoch.
      if(next_slot==3*slots)check(bound.current().state->timed.view.justified.epoch>=2,"participation did not justify the expected checkpoint");
      check(chain.tip().header.state_root==chain.state().root(),"validator publication exposes inconsistent EVM state");
      if(peers)peers->sync(chain.tip().header.hash());
      if(next_slot%slots==0){
       const auto view=bound.current();const auto& head=bellatrix_fork_head(*view.state,fork_rules);const auto& b=head.state.ssz;
       const auto at=randao+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
       const auto prev=little(b,at,4),cur=little(b,at+4,4);size_t previous_target=0,current_target=0;
       for(size_t i=0;i<64;++i){previous_target+=(b.at(prev+i)&2)!=0;current_target+=(b.at(cur+i)&2)!=0;}
       std::cout<<Json{{"slot",next_slot},{"justified",view.state->timed.view.justified.epoch},{"finalized",view.state->timed.view.finalized.epoch},{"previousTargetParticipants",previous_target},{"currentTargetParticipants",current_target},{"archivedVotes",view.signed_votes->size()},{"loop",loop.status()}}.dump()<<std::endl;
      }
     }
     check(chain.store().receipt(pending.hash()).has_value()&&chain.pending_count()==0&&chain.state().get_storage(target,0)==3,"wallet transaction executed and removed from pool");
     check(chain.state().get(sender).balance<genesis_state.get(sender).balance,"Beacon candidate must not mint legacy mining rewards");
     const auto current=bound.current();finalized=current.state->timed.view.finalized.epoch;check(finalized>=2&&current.state->timed.view.justified.epoch>=3,"validator loop failed to advance finality");
     std::set<Bytes> included_votes;
     for(const auto& [root,wire]:*current.signed_blocks){
      const auto message=little(*wire,0,4),body_at=message+little(*wire,message+80,4);
      const auto begin=body_at+little(*wire,body_at+208,4),end=body_at+little(*wire,body_at+212,4);
      if(begin==end)continue;const auto count=little(*wire,begin,4)/4;
      for(size_t i=0;i<count;++i){const auto from=begin+little(*wire,begin+4*i,4),to=i+1<count?begin+little(*wire,begin+4*(i+1),4):end;
       check(included_votes.insert(sha256(Bytes(wire->begin()+from,wire->begin()+to))).second,"local chain repeatedly included identical attestation");}
     }
     check(included_votes.size()>=128,"deduplication discarded useful participation");
     check_finalized_rpc(chain,bound,sender,target);
     check(chain.finalized_block()->header.number<chain.tip().header.number&&current.state->blocks.at(current.state->timed.view.finalized.root).execution_state.get_storage(target,0)==2,"finalized view includes a later transaction");
     saved=current.token;final_hash=chain.tip().header.hash();final_state=chain.state().root();loop_status=loop.status();
     if(peers){check(peers->observer->state().root()==final_state&&peers->reactor->current().state->timed.view.finalized.epoch==finalized,"observer finality/state mismatch");check(peers->observer->store().receipt(pending.hash()).has_value(),"observer missing wallet receipt");peers->stop();}
     }
    }
    {
     Blockchain reopened(live/"chain",g);bool gated=false;try{Rpc rpc(reopened);}catch(const std::exception&){gated=true;}check(gated,"reopened Beacon RPC escaped checkpoint gate");
     BellatrixDurableReactor restored(reopened,fork_rules,reactor_limits,saved);Rpc rpc(reopened);
     if(!deposit_loop)check_finalized_rpc(reopened,restored,sender,target);
     check(reopened.tip().header.hash()==final_hash&&reopened.state().root()==final_state&&(deposit_loop||restored.current().state->timed.view.finalized.epoch==finalized),"validator finality/execution lost after reopen");
     if(deposit_loop){const auto& head=bellatrix_fork_head(*restored.current().state,fork_rules);const auto context=bellatrix_fork_block_context(head.state,head.header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},l);check(context.registry.validators.size()==65&&Bytes(context.registry.validators.at(64).public_key.begin(),context.registry.validators.at(64).public_key.end())==Key(64).public_key&&little(head.state.ssz,hist+80,8)==1,"reopen lost admitted validator or deposit cursor");}
    }
    if(deposit_loop){std::cout<<Json{{"scenario","bellatrix-native-deposit-reopen"},{"reopened",true}}.dump()<<'\n';return 0;}
    std::cout<<Json{{"scenario","bellatrix-local-validator-loop"},{"localKeys",64},{"syntheticClock",true},{"realP2pDaemons",network?2:0},{"independentCoreProcesses",false},{"validatorHosts",1},{"walletTransactionExecuted",true},{"finalizedEpoch",finalized},{"reopened",true},{"loop",loop_status}}.dump()<<'\n';return 0;
   }

   if(deposit_store){deposit_genesis.chain_id=1337;deposit_genesis.consensus="beacon";deposit_genesis.evm_revision=EvmRevision::Paris;deposit_genesis.block=execution_genesis;deposit_genesis.state=genesis_state;deposit_chain=std::make_unique<Blockchain>(reactor_location/"deposit-chain",deposit_genesis);reactor=std::make_unique<BellatrixDurableReactor>(*deposit_chain,fork_rules,reactor_limits,*store);}
   else reactor=std::make_unique<BellatrixDurableReactor>(*reactor_disk,1337,execution_genesis.header.hash(),fork_rules,reactor_limits,*store);
   service=std::make_unique<BellatrixConsensusService>(*reactor,wire_budget,4*1024*1024,r.consensus.maximum_block_bytes,65536,65536,4,8*1024*1024);
   Json wrong=reactor->network_identity();wrong["kind"]="block";wrong["ssz"]=hex(wire);const auto original=wrong;
   auto refused=[&](const std::string& frame){check(!service->decode(frame),"foreign/invalid network frame accepted");++wire_refusals;};
   wrong["consensusIdentity"]=hex(Bytes(32,1));refused(wrong.dump());wrong=original;wrong["chainId"]="0x1";refused(wrong.dump());wrong=original;wrong["kind"]="tick";refused(wrong.dump());wrong=original;wrong.erase("consensusIdentity");refused(wrong.dump());refused("{\"kind\":\"block\","+original.dump().substr(1));refused(std::string(4*1024*1024+1,'x'));
   auto reject_anchor=[&](const State& candidate,const std::vector<Header>& ancestors){bool rejected=false;try{initialize_bellatrix_fork_store(received->consensus,built->message_ssz,received->execution.block,candidate,received->execution.receipts,ancestors,fork_rules);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"inconsistent execution anchor accepted");};
   reject_anchor(received->execution.state,{});auto corrupt=received->execution.state;corrupt.get(target).balance+=1;reject_anchor(corrupt,{parent.header});
  }else{
   check(!prepare_bellatrix_received_block(*store,wire,fork_rules,64,100000),"future block accepted");
   store=prepare_bellatrix_store_tick(*store,(genesis_time+slot*seconds)*1000,fork_rules,64,100000);reactor->tick((genesis_time+slot*seconds)*1000);
   auto bad=wire;bad[4]^=1;check(!prepare_bellatrix_received_block(*store,bad,fork_rules,64,100000),"invalid block signature accepted by Store");
   const auto before=store->blocks.size();size_t store_calls=0;auto next=prepare_bellatrix_received_block(*store,wire,fork_rules,64,100000,[&]{++store_calls;});
   if(slot==2){
    for(size_t stop:std::set<size_t>{1,store_calls/2,store_calls}){size_t count=0;bool cancelled=false;try{prepare_bellatrix_received_block(*store,wire,fork_rules,64,100000,[&]{if(++count==stop)throw std::runtime_error("cancel Store");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel Store";}check(cancelled&&store->blocks.size()==before&&bellatrix_fork_head(*store,fork_rules).execution_state.root()==old_state,"Store cancellation mutated parents");++store_cancellations;}
    auto corrupt_store=*store;corrupt_store.blocks.at(parent_root).execution_state.get(target).balance+=1;bool rejected=false;try{prepare_bellatrix_received_block(corrupt_store,wire,fork_rules,64,100000);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"unbound parent EVM state accepted");
    auto unknown=wire;unknown[116]^=1;check(!prepare_bellatrix_received_block(*store,unknown,fork_rules,64,100000),"unknown Beacon parent accepted");
   }
   check(next&&next->blocks.size()==before+1&&store->blocks.size()==before,"Store publication not private");store=std::move(*next);
   check(deliver("block",wire),"durable reactor rejected block");
   check(prepare_bellatrix_received_block(*store,wire,fork_rules,64,100000)->blocks.size()==store->blocks.size(),"duplicate block replaced Store");
  }
  check(bellatrix_fork_head(*store,fork_rules).execution_state.root()==expected.root(),"fork head EVM mismatch");
  if(deposit_store){
   const auto& head=bellatrix_fork_head(*reactor->current().state,fork_rules);check(head.deposit_history&&head.deposit_history->count()==slot-1,"reactor missed executed deposit");
   check(deposit_chain->tip().header.hash()==head.execution_block.header.hash()&&deposit_chain->state().root()==head.execution_state.root(),"deposit checkpoint differs from canonical EVM");
   if(slot==3){
    // A second valid signed block from the same parent omits only the deposit.
    auto alternate=execution;alternate.transactions.pop_back();State alt_state=state;ExecutionEngine(r.chain_id,history,EvmRevision::Paris,r.replay_policy).execute_block(alt_state,alternate,false);
    auto alt_body=contents;alt_body.resize(fixed);const auto alt_payload=encode_bellatrix_execution_payload(payload(alternate),r.execution,r.consensus.maximum_block_bytes);alt_body.insert(alt_body.end(),alt_payload.begin(),alt_payload.end());
    const auto alt=prepare_bellatrix_unsigned_block(beacon,slot,proposer,parent_root,alt_body,r,parent.header,state,history,64,100000);check(alt.has_value(),"alternative deposit branch construction");check(deliver("block",sign_message(alt->message_ssz)),"alternative deposit branch rejected");
    const auto current=reactor->current();size_t one=0,two=0;for(const auto& [_,b]:current.state->blocks){check(bool(b.deposit_history),"missing branch deposit history");one+=b.deposit_history->count()==1;two+=b.deposit_history->count()==2;}check(one==2&&two==1,"competing branch deposit counts mixed");
    const auto serialized=bellatrix_store_snapshot(*current.state,fork_rules,reactor_limits);auto altered=Json::parse(serialized);bool changed=false;for(auto& b:altered["blocks"])if(!b[5][3]["deposits"].empty()){b[5][3]["deposits"][0]=std::string("0x")+std::string(368,'0');changed=true;break;}check(changed,"no deposit snapshot to corrupt");
    const auto corrupt=altered.dump();bool rejected=false;try{bellatrix_store_from_snapshot(corrupt,sha256(Bytes(corrupt.begin(),corrupt.end())),fork_rules,reactor_limits);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"forged deposit history accepted with recomputed digest");
    auto legacy=fork_rules;legacy.deposit_contract_code_hash.clear();rejected=false;try{bellatrix_store_from_snapshot(serialized,sha256(Bytes(serialized.begin(),serialized.end())),legacy,reactor_limits);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"deposit profile restored as legacy");
    const auto token=current.token;const auto canonical_hash=deposit_chain->tip().header.hash();
    service->stop();service->join();service.reset();reactor.reset();deposit_chain.reset();deposit_chain=std::make_unique<Blockchain>(reactor_location/"deposit-chain",deposit_genesis);reactor=std::make_unique<BellatrixDurableReactor>(*deposit_chain,fork_rules,reactor_limits,token);
    check(deposit_chain->tip().header.hash()==canonical_hash&&bellatrix_store_snapshot(*reactor->current().state,fork_rules,reactor_limits)==serialized,"restart lost branch deposits or execution checkpoint");
    std::cout<<Json{{"scenario","bellatrix-deposit-store"},{"fundedDeposits",2},{"competingBranches",true},{"snapshotTamperingRejected",true},{"atomicExecutionReopen",true},{"beaconAdmission",false}}.dump()<<'\n';return 0;
   }
  }
  if(slot==1){
   offender=proposer;const ConsensusBeaconHeader h{slot,proposer,parent_root,root_at(built->message_ssz,48),bellatrix_beacon_body_root(contents,r.consensus.body,l.sync_committee_size,r.execution,r.consensus.maximum_block_bytes)};
   auto conflicting=h;conflicting.body_root[0]^=1;const auto sign_header=[&](const ConsensusBeaconHeader& header){return signing.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,genesis_root)));};first_proof=signed_header(h,sign_header(h));const auto other=signed_header(conflicting,sign_header(conflicting));first_proof.insert(first_proof.end(),other.begin(),other.end());
   auto deny=[&](const Bytes& bad){bool refused=false;try{refused=!prepare_bellatrix_state_transition(beacon,bad,r,parent.header,state,history,64,100000);}catch(const std::invalid_argument&){refused=true;}check(refused&&beacon==old_beacon&&state.root()==old_state,"bad signed block accepted or mutated parents");++refusals;};
   auto bad=wire;bad[4]^=1;deny(bad);
   auto message=built->message_ssz;message[48]^=1;deny(sign_message(message));
   message=built->message_ssz;message[16]^=1;deny(sign_message(message));
   message=built->message_ssz;message[84]^=1;deny(sign_message(message));
   message=built->message_ssz;message[84+fixed+472]^=1;deny(sign_message(message));
   message=built->message_ssz;std::copy(reveal.begin(),reveal.end(),message.begin()+84+220+(l.sync_committee_size+7)/8);deny(sign_message(message));
   message=built->message_ssz;const auto other_index=(proposer+1)%registry.size();put(message,8,other_index);Key other_key(other_index);const auto wrong_reveal=other_key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,genesis_root)));std::copy(wrong_reveal.begin(),wrong_reveal.end(),message.begin()+84);deny(sign_message(message,other_index));
   for(size_t stop:std::set<size_t>{1,calls/4,calls/2,3*calls/4,calls-1,calls}){size_t seen=0;bool stopped=false;try{prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,64,100000,[&]{if(++seen==stop)throw std::runtime_error("cancel full block");});}catch(const std::runtime_error& e){stopped=std::string(e.what())=="cancel full block";}check(stopped&&beacon==old_beacon&&state.root()==old_state,"full transition cancellation leaked state");++cancellations;}
   bool cancellation_type=false;try{prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,64,100000,[]{throw std::invalid_argument("cancel body hash");});}catch(const std::invalid_argument& e){cancellation_type=std::string(e.what())=="cancel body hash";}check(cancellation_type,"body cancellation treated as invalid peer block");++cancellations;
   auto small=r;small.maximum_state_bytes=beacon.ssz.size()-1;bool state_budget=false;try{prepare_bellatrix_state_transition(beacon,wire,small,parent.header,state,history,64,100000);}catch(const std::length_error&){state_budget=true;}check(state_budget,"signed transition state budget ignored");
   small=r;small.consensus.maximum_block_bytes=wire.size()-1;check(!prepare_bellatrix_state_transition(beacon,wire,small,parent.header,state,history,64,100000),"signed transition block budget ignored");
   bool slots_budget=false;try{prepare_bellatrix_state_transition(beacon,wire,r,parent.header,state,history,0,100000);}catch(const std::length_error&){slots_budget=true;}check(slots_budget,"signed transition slots budget ignored");
  }
  if(slot==slots+1){

   const auto current=reactor->current();bool assigned=false;
   const auto batch=prepare_bellatrix_attestation_duties(*current.state,fork_rules,slot,64,100000);
   check(batch.size()==8,"minimal slot committee must contain eight assignments");
   std::set<uint64_t> indices;for(const auto& [index,duty]:batch){indices.insert(index);check(std::count(duty.attestation.aggregation_bits.begin(),duty.attestation.aggregation_bits.end(),true)==1,"batched duty must have one signer");}
   check(reactor->local_attesters(slot,current.token)==indices,"scheduler committee differs from prepared duties");
   check(reactor->current_attesters(slot)==indices,"current-state committee differs from explicit checkpoint");
   check(!reactor->current_proposer(slot+1)&&reactor->current_attesters(slot+1).empty(),"local selector accepted future slot");
   {size_t calls=0;const auto& first=batch.begin()->second;auto& journal=local_journal(first.validator_index);
    check(!reactor->attest_current(UINT64_MAX,slot,[&]{return (genesis_time+slot*seconds)*1000;},journal,[&](const ConsensusRoot&){++calls;return Bytes();})&&calls==0,"unassigned local key reached signer");}

   auto excluded=*current.state;excluded.timed.view.equivocating_indices.insert(batch.begin()->first);
   const auto clean=prepare_bellatrix_attestation_duties(excluded,fork_rules,slot,64,100000);
   check(clean.size()+1==batch.size()&&!clean.contains(batch.begin()->first),"batched duties include equivocator");
   check(prepare_bellatrix_attestation_duties(*current.state,fork_rules,slot+1,64,100000).empty(),"batched duties accepted stale slot");

   for(uint64_t index=0;index<64;++index){const auto duty=prepare_bellatrix_attestation_duty(*current.state,fork_rules,index,slot,64,100000);if(!duty)continue;assigned=true;Key voter(index);auto& journal=local_journal(index);const auto vote=reactor->sign_attestation(index,slot,[&]{return (genesis_time+slot*seconds)*1000;},current.token,journal,[&](const ConsensusRoot& root){return voter.sign(root);});check(vote&&vote->data==duty->attestation.data&&vote->signature==voter.sign(consensus_signing_root(consensus_attestation_root(vote->data),consensus_domain({1,0,0,0},duty->fork_version,duty->genesis_root))),"local attestation mismatch");++local_attestations;break;}
   check(assigned,"no local attester assignment");
   const auto control=prepare_bellatrix_unsigned_block(beacon,slot,proposer,parent_root,body({}),r,parent.header,state,history,64,100000);check(control.has_value(),"slashing control failed");
   // A second signed block has a different EVM transfer and no slashing proof.
   auto side_execution=execution;side_execution.transactions.clear();auto side_tx=tx;side_tx.value=7;side_execution.transactions.push_back(SignedTransaction::sign(side_tx,key));State side_state=state;ExecutionEngine(r.chain_id,history,EvmRevision::Paris,r.replay_policy).execute_block(side_state,side_execution,false);
   auto side_body=body({});side_body.resize(fixed);const auto side_payload=encode_bellatrix_execution_payload(payload(side_execution),r.execution,r.consensus.maximum_block_bytes);side_body.insert(side_body.end(),side_payload.begin(),side_payload.end());
   const auto side=prepare_bellatrix_unsigned_block(beacon,slot,proposer,parent_root,side_body,r,parent.header,state,history,64,100000);check(side.has_value(),"competing block construction failed");
   auto branch=prepare_bellatrix_received_block(*store,sign_message(side->message_ssz),fork_rules,64,100000);check(branch&&branch->blocks.size()==store->blocks.size()+1,"competing branch rejected");store=std::move(*branch);check(deliver("block",sign_message(side->message_ssz)),"durable reactor rejected competing block");
   const auto canonical=consensus_beacon_header_root(bellatrix_fork_head(*store,fork_rules).header);
   ConsensusRoot alternate{};for(const auto& [id,record]:store->blocks)if(record.execution_block.header.hash()==side_execution.header.hash())alternate=id;
   check(alternate!=ConsensusRoot{}&&canonical!=alternate,"competing execution not distinguished");
   store=prepare_bellatrix_store_tick(*store,(genesis_time+(slot+1)*seconds)*1000,fork_rules,64,100000);reactor->tick((genesis_time+(slot+1)*seconds)*1000);
   const auto& target_record=store->blocks.at(parent_root);const BellatrixEpochRules er{r.consensus.epoch,r.execution,r.maximum_state_bytes};const ConsensusCheckpoint target_checkpoint{epoch,parent_root};
   const auto context=prepare_bellatrix_fork_state_contexts(target_record.state,target_record.header,target_checkpoint,er,l,64,100000);
   const auto assignment=phase0_attestation_duties(context.target.registry,context.target.randao,duties,epoch);
   const auto committee=partition_committee(assignment.shuffled,(slot%slots)*assignment.committees_per_slot,assignment.committees_per_slot*slots);check(committee.size()*assignment.committees_per_slot>=6,"slot committees too small for reorg scenario");
   auto vote_for=[&](size_t position,const ConsensusRoot& head){const auto index=position/committee.size();const auto members=partition_committee(assignment.shuffled,(slot%slots)*assignment.committees_per_slot+index,assignment.committees_per_slot*slots);position%=committee.size();Phase0SignedAttestation vote{{slot,index,head,{},target_checkpoint},std::vector<bool>(members.size()),{}};vote.aggregation_bits[position]=true;Key signer(members[position]);vote.signature=signer.sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},context.target.fork.current_version,context.target.genesis_root)));auto result=prepare_bellatrix_store_vote(*store,vote,fork_rules,64,100000);check(result.has_value(),"valid branch vote rejected");store=std::move(*result);check(deliver("attestation",encode_consensus_attestation_ssz(vote,r.consensus.body.validators_per_committee)),"durable reactor rejected BLS vote");check(bellatrix_store_snapshot(*reactor->current().state,fork_rules,reactor_limits)==bellatrix_store_snapshot(*store,fork_rules,reactor_limits),"reactor vote changed Store semantics");};
   vote_for(0,canonical);check(bellatrix_fork_head(*store,fork_rules).execution_state.root()==expected.root(),"canonical vote lost paired EVM state");
   vote_for(1,alternate);vote_for(2,alternate);check(bellatrix_fork_head(*store,fork_rules).execution_state.root()==side_state.root(),"vote reorg did not switch EVM state");++reorgs;reorg_stores.push_back(*store);
   vote_for(3,canonical);vote_for(4,canonical);vote_for(5,canonical);check(bellatrix_fork_head(*store,fork_rules).execution_state.root()==expected.root(),"return reorg did not restore EVM state");++reorgs;reorg_stores.push_back(*store);
   const auto start=little(received->consensus.ssz,hist+88,4),balance_start=little(received->consensus.ssz,hist+92,4),control_balances=little(control->post.consensus.ssz,hist+92,4);
   check(received->consensus.ssz[start+121*offender+88]==1,"double proposal not slashed");
   const auto effective=little(advanced.ssz,validators+121*offender+80,8),penalty=effective/r.consensus.epoch.slashing.minimum_penalty_quotient,reward=effective/r.consensus.epoch.slashing.whistleblower_reward_quotient;
   for(size_t i=0;i<registry.size();++i){const __int128 want=__int128(little(control->post.consensus.ssz,control_balances+8*i,8))-(i==offender?penalty:0)+(i==proposer?reward:0);check(__int128(little(received->consensus.ssz,balance_start+8*i,8))==want,"Bellatrix slashing economics mismatch");}
  }
  check(beacon==old_beacon&&state.root()==old_state,"successful preparation changed parents");beacon=received->consensus;state=received->execution.state;parent=received->execution.block;hashes[parent.header.number]=parent.header.hash();++blocks;
 }
 check(beacon.last_processed_epoch==0,"epoch boundary not processed");
 check(bellatrix_store_snapshot(*reactor->current().state,fork_rules,reactor_limits)==bellatrix_store_snapshot(*store,fork_rules,reactor_limits),"reactor state differs from in-memory admission");
 const auto published=reactor->current();check(published.signed_blocks->size()==9&&published.signed_votes->size()==6,"durable signed archives incomplete");
 {
  Json duplicate=reactor->network_identity();duplicate["kind"]="block";duplicate["ssz"]=hex(*published.signed_blocks->begin()->second);const auto frame=duplicate.dump();
  const auto unchanged=reactor->current().token;
  check(reactor->block(*published.signed_blocks->begin()->second)&&reactor->current().token==unchanged,"exact block retry changed the durable duty token");
  std::promise<void> entered,release;auto started=entered.get_future();auto resume=release.get_future().share();
  BellatrixConsensusService bounded(*reactor,wire_budget,4*1024*1024,r.consensus.maximum_block_bytes,65536,65536,1,8*1024*1024,[&](const auto& check){entered.set_value();resume.wait();check();});
  const auto token_before=reactor->current().token;std::promise<BellatrixConsensusService::Decision> completion;auto result=completion.get_future();auto cancelled=std::make_shared<std::atomic<bool>>(false);
  check(bounded.submit(frame,cancelled,[&](auto d){completion.set_value(d);}),"bounded service did not admit first job");
  if(started.wait_for(std::chrono::seconds(10))!=std::future_status::ready){release.set_value();throw std::runtime_error("bounded worker did not start");}
  const bool extra=bounded.submit(frame,std::make_shared<std::atomic<bool>>(false),[](auto){});bounded.stop();release.set_value();bounded.join();check(!extra&&result.get()==BellatrixConsensusService::Decision::local_failure&&reactor->current().token==token_before,"stopped/saturated service published checkpoint");
  check(!bounded.submit(frame,std::make_shared<std::atomic<bool>>(false),[](auto){}),"stopped service admitted work");
  BellatrixConsensusService tiny(*reactor,wire_budget,4*1024*1024,r.consensus.maximum_block_bytes,65536,65536,4,1);check(!tiny.submit(frame,std::make_shared<std::atomic<bool>>(false),[](auto){}),"worker byte budget ignored");
 }
 service->stop();service->join();service.reset();reactor.reset();reactor_disk.reset();reactor_disk=std::make_unique<ChainStore>(reactor_location/"chain");reactor=std::make_unique<BellatrixDurableReactor>(*reactor_disk,1337,execution_genesis.header.hash(),fork_rules,reactor_limits,published.token);
 check(reactor->current().token==published.token&&reactor->current().signed_blocks->size()==9&&reactor->current().signed_votes->size()==6,"reactor restart lost checkpoint/archives");
 const auto clock_ms=reactor->current().state->timed.clock.time_ms;size_t calls_before_write=0;reactor->tick(clock_ms+1,[&]{++calls_before_write;});const auto before_cancel=reactor->current().token;
 size_t reactor_cancellations=0;for(size_t stop:std::set<size_t>{1,calls_before_write/2,calls_before_write}){size_t seen=0;bool stopped=false;try{reactor->tick(clock_ms+2,[&]{if(++seen==stop)throw std::runtime_error("cancel reactor");});}catch(const std::runtime_error& e){stopped=std::string(e.what())=="cancel reactor";}check(stopped&&reactor->current().token==before_cancel&&reactor_disk->fork_checkpoint(1337,execution_genesis.header.hash(),reactor_limits.envelope_bytes)->token==before_cancel,"reactor cancellation published candidate");++reactor_cancellations;}
 BellatrixDurableReactor competing(*reactor_disk,1337,execution_genesis.header.hash(),fork_rules,reactor_limits,before_cancel);competing.tick(clock_ms+2);bool stale=false;try{reactor->tick(clock_ms+1);}catch(const std::runtime_error& e){stale=std::string(e.what())=="fork checkpoint differs from external commitment";}check(stale&&reactor_disk->fork_checkpoint(1337,execution_genesis.header.hash(),reactor_limits.envelope_bytes)->token==competing.current().token,"stale reactor overwrote checkpoint");bool unavailable=false;try{reactor->current();}catch(const std::runtime_error&){unavailable=true;}check(unavailable,"stale reactor remained publishable");

 bool reactor_sync_error=false;
#ifdef __APPLE__
 const auto stable=competing.current();fs::path wal;for(const auto& entry:fs::directory_iterator(reactor_location/"chain"))if(entry.path().extension()==".log"&&(wal.empty()||entry.path().filename()>wal.filename()))wal=entry.path();check(!wal.empty(),"reactor WAL missing");nano_test_arm_sync_fault(wal.c_str());bool failed=false;try{competing.tick(clock_ms+3);}catch(const std::runtime_error&){failed=true;}bool closed=false;try{competing.current();}catch(const std::runtime_error&){closed=true;}nano_test_disarm_sync_fault();check(failed&&closed&&nano_test_sync_failures()>0&&stable.state->timed.clock.time_ms==clock_ms+2,"uncertain reactor published new state");reactor_sync_error=true;
#endif
 const BellatrixStoreSnapshotLimits limits{16*1024*1024,32,128,128,4*1024*1024,8*1024*1024,64,100000};
 size_t snapshot_calls=0;const auto encoded=bellatrix_store_snapshot(*store,fork_rules,limits,[&]{++snapshot_calls;});
 const auto digest=sha256(Bytes(encoded.begin(),encoded.end()));const auto decoded=bellatrix_store_from_snapshot(encoded,digest,fork_rules,limits);
 check(bellatrix_store_snapshot(decoded,fork_rules,limits)==encoded&&bellatrix_fork_head(decoded,fork_rules).execution_state.root()==state.root(),"full Bellatrix Store roundtrip");
 unsigned snapshot_refusals=0;auto reject_snapshot=[&](const std::string& bytes,const Bytes& commitment,const BellatrixForkBlockRules& rules){bool rejected=false;try{bellatrix_store_from_snapshot(bytes,commitment,rules,limits);}catch(const std::exception&){rejected=true;}check(rejected,"invalid Bellatrix snapshot accepted");++snapshot_refusals;};
 auto wrong_digest=digest;wrong_digest[0]^=1;reject_snapshot(encoded,wrong_digest,fork_rules);
 auto other_rules=fork_rules;other_rules.transition.chain_id+=1;reject_snapshot(encoded,digest,other_rules);
 auto damaged=Json::parse(encoded);damaged["blocks"][0][5][1]=State{}.to_json();auto damaged_bytes=damaged.dump();reject_snapshot(damaged_bytes,sha256(Bytes(damaged_bytes.begin(),damaged_bytes.end())),fork_rules);
 damaged=Json::parse(encoded);damaged["ancestors"]=Json::array();damaged_bytes=damaged.dump();reject_snapshot(damaged_bytes,sha256(Bytes(damaged_bytes.begin(),damaged_bytes.end())),fork_rules);
 damaged=Json::parse(encoded);damaged["blocks"].push_back(damaged["blocks"][0]);damaged_bytes=damaged.dump();reject_snapshot(damaged_bytes,sha256(Bytes(damaged_bytes.begin(),damaged_bytes.end())),fork_rules);
 damaged=Json::parse(encoded);damaged["blocks"][0][5][2][0]["blockHash"]=hex(Bytes(32,1));damaged_bytes=damaged.dump();reject_snapshot(damaged_bytes,sha256(Bytes(damaged_bytes.begin(),damaged_bytes.end())),fork_rules);
 auto too_small=limits;too_small.envelope_bytes=encoded.size()-1;bool bounded=false;try{bellatrix_store_from_snapshot(encoded,digest,fork_rules,too_small);}catch(const std::invalid_argument&){bounded=true;}check(bounded,"snapshot envelope budget ignored");
 for(size_t stop:std::set<size_t>{1,snapshot_calls/2,snapshot_calls}){size_t seen=0;bool stopped=false;try{bellatrix_store_snapshot(*store,fork_rules,limits,[&]{if(++seen==stop)throw std::runtime_error("cancel snapshot");});}catch(const std::runtime_error& e){stopped=std::string(e.what())=="cancel snapshot";}check(stopped,"snapshot cancellation lost");}
 Temp location;const auto database=location/"bellatrix";std::optional<ForkCheckpointToken> token;Bytes disk_tip=execution_genesis.header.hash();
 // Persist both competing heads, with a close/reopen and full revalidation each time.
 reorg_stores.insert(reorg_stores.begin(),*store);
 for(size_t event=0;event<reorg_stores.size();++event){const auto& candidate=reorg_stores[event];const auto& head=bellatrix_fork_head(candidate,fork_rules);const auto snapshot=bellatrix_store_snapshot(candidate,fork_rules,limits);std::vector<ExecutionCheckpointBlock> suffix;
  auto id=consensus_beacon_header_root(head.header);const Int common_height=event?Int(std::to_string(slots)):Int(0);Bytes common_hash=execution_genesis.header.hash();
  for(;;){const auto& record=candidate.blocks.at(id);if(record.execution_block.header.number==common_height){common_hash=record.execution_block.header.hash();break;}suffix.push_back({record.execution_block,record.execution_state,record.receipts});if(id==candidate.timed.view.anchor)break;id=record.header.parent_root;}std::reverse(suffix.begin(),suffix.end());
  {ChainStore disk(database);if(!event){disk.bind_chain(1337,execution_genesis.header.hash(),"bellatrix-checkpoint-test");disk.commit(execution_genesis,genesis_state,{});}token=disk.commit_execution_checkpoint(1337,execution_genesis.header.hash(),token,disk_tip,common_height,common_hash,suffix,snapshot,32*1024*1024,32);}
  {ChainStore disk(database);const auto persisted=disk.fork_checkpoint(1337,execution_genesis.header.hash(),limits.envelope_bytes,token);check(persisted.has_value(),"Bellatrix checkpoint lost");const auto restored=bellatrix_store_from_snapshot(persisted->snapshot,persisted->token.digest,fork_rules,limits);const auto& selected=bellatrix_fork_head(restored,fork_rules);disk_tip=selected.execution_block.header.hash();check(disk.latest()->header.hash()==disk_tip&&disk.state_at(selected.execution_block.header.number)->root()==selected.execution_state.root(),"Bellatrix recovery split Beacon/EVM");check(disk.receipt(selected.execution_block.transactions[0].hash())->block_hash==disk_tip,"Bellatrix recovery receipt mismatch");}
 }

 std::cout<<Json{{"scenario","signed-bellatrix-evm-chain"},{"blocks",blocks},{"validators",64},{"invalidSignedBlocks",refusals},{"cancellations",cancellations},{"doubleProposalSlashed",true},{"pairedExecutionReorgs",reorgs},{"storeCancellations",store_cancellations},{"forkBlocks",store->blocks.size()},{"durableCommit",true},{"durableReactor",true},{"localProposals",local_proposals},{"localAttestations",local_attestations},{"localSignatureRefusals",signature_refusals},{"reactorSyncErrorTested",reactor_sync_error},{"reactorCancellations",reactor_cancellations},{"snapshotRefusals",snapshot_refusals},{"pairedCheckpointReopens",reorg_stores.size()},{"networkActive",false},{"wireMessages",wire_messages},{"wireRefusals",wire_refusals}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
