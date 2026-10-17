#include "nano/chain.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_capella_bootstrap_archive.hpp"
#include "nano/consensus_capella_reactor.hpp"
#include "nano/consensus_capella_execution.hpp"
#include "nano/consensus_capella_registration.hpp"
#include "nano/consensus_deposit_history.hpp"
#include "nano/storage_memory_policy.hpp"
#include "nano/checkpoint_blob_gc.hpp"
#include "ckzg/blst/bindings/blst.h"
#include "../network/capella_consensus_service.hpp"
#include "support/rocksdb_fixture.hpp"
#include <openssl/crypto.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <set>
#include <thread>
#include <sys/resource.h>
#include <unistd.h>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
namespace {
using namespace nano;namespace fs=std::filesystem;
const auto started=std::chrono::steady_clock::now();
const char* test_stage="initialization";
uint64_t test_slot=0;
Json public_peak_rss_kib(){rusage u{};if(::getrusage(RUSAGE_SELF,&u)!=0)return nullptr;
#ifdef __APPLE__
 return uint64_t(u.ru_maxrss/1024);
#else
 return uint64_t(u.ru_maxrss);
#endif
}
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void checkpoint(){check(std::chrono::steady_clock::now()-started<std::chrono::seconds(180),"proposer test wall bound");rusage u{};check(::getrusage(RUSAGE_SELF,&u)==0,"proposer test resource read");
#ifdef __APPLE__
 check(u.ru_maxrss/1024<=131072,"proposer test RSS bound");
#else
 check(u.ru_maxrss<=131072,"proposer test RSS bound");
#endif
}
std::string read(const fs::path& path){std::ifstream in(path,std::ios::binary);check(bool(in),"public fixture read");std::string s((std::istreambuf_iterator<char>(in)),{});check(s.size()<=32*1024*1024,"public fixture input bound");return s;}
Bytes bytes_of(const std::string& s){return {s.begin(),s.end()};}
struct Temp{fs::path path;Temp(){auto pattern=(fs::temp_directory_path()/"nano-proposer-slashing-XXXXXX").string();check(::mkdtemp(pattern.data()),"owned temporary fixture");path=fs::canonical(pattern);}~Temp(){std::error_code e;fs::remove_all(path,e);}};
struct Fixture{Genesis genesis;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaSlotState pre;CapellaForkStore initial;Bytes wire;CapellaSignedBootstrapArchive proof;};
uint64_t test_signatures=0;
uint64_t test_little(std::span<const uint8_t> b,size_t at,size_t width=8){check(width<=8&&at<=b.size()&&width<=b.size()-at,"synthetic field bounds");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
void test_put(Bytes& b,size_t at,uint64_t n,size_t width=8){check(at<=b.size()&&width<=b.size()-at,"synthetic write bounds");for(size_t i=0;i<width;++i)b[at+i]=uint8_t(n>>(8*i));}
template<class A>void test_copy(A& a,const Bytes& b){check(a.size()==b.size(),"synthetic fixed width");std::copy(b.begin(),b.end(),a.begin());}
ConsensusRoot test_root(std::span<const uint8_t> b,size_t at=0){check(at<=b.size()&&32<=b.size()-at,"synthetic root bounds");ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
struct TestKey{
 blst_scalar scalar{};Bytes public_key;
 explicit TestKey(uint64_t index){check(index<=10063,"synthetic key range");uint64_t words[4]{1800001+index,0,0,0};blst_scalar_from_uint64(&scalar,words);OPENSSL_cleanse(words,sizeof(words));blst_p1 p;blst_sk_to_pk_in_g1(&p,&scalar);public_key.resize(48);blst_p1_compress(public_key.data(),&p);}
 TestKey(const TestKey&)=delete;TestKey& operator=(const TestKey&)=delete;
 ~TestKey(){OPENSSL_cleanse(&scalar,sizeof(scalar));}
 Bytes sign(const ConsensusRoot& root)const{checkpoint();check(test_signatures<1024,"bounded test-only synthetic signatures");++test_signatures;static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,p;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&p,&h,&scalar);Bytes s(96);blst_p2_compress(s.data(),&p);OPENSSL_cleanse(&h,sizeof(h));OPENSSL_cleanse(&p,sizeof(p));return s;}
};
ConsensusRoot test_leaf(std::span<const uint8_t> b){check(b.size()<=32,"SSZ leaf bound");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
ConsensusRoot test_pair(const ConsensusRoot& a,const ConsensusRoot& b){Bytes p(a.begin(),a.end());p.insert(p.end(),b.begin(),b.end());return test_root(sha256(p));}
ConsensusRoot test_registry_root(const Bytes& data,uint64_t limit){
 check(data.size()==64*121&&limit>=64&&limit<=UINT64_MAX/2&&(limit&(limit-1))==0,"synthetic registry bounds");std::array<ConsensusRoot,64> roots{};
 for(size_t i=0;i<64;++i){auto v=std::span<const uint8_t>(data).subspan(i*121,121);std::array<ConsensusRoot,8> f{test_pair(test_leaf(v.first(32)),test_leaf(v.subspan(32,16))),test_leaf(v.subspan(48,32)),test_leaf(v.subspan(80,8)),test_leaf(v.subspan(88,1)),test_leaf(v.subspan(89,8)),test_leaf(v.subspan(97,8)),test_leaf(v.subspan(105,8)),test_leaf(v.subspan(113,8))};for(size_t n=8;n>1;n/=2)for(size_t j=0;j<n/2;++j)f[j]=test_pair(f[j*2],f[j*2+1]);roots[i]=f[0];}
 for(size_t n=64;n>1;n/=2)for(size_t j=0;j<n/2;++j)roots[j]=test_pair(roots[j*2],roots[j*2+1]);ConsensusRoot zero{},r=roots[0];for(unsigned i=0;i<6;++i)zero=test_pair(zero,zero);for(uint64_t n=64;n<limit;n*=2){r=test_pair(r,zero);zero=test_pair(zero,zero);}ConsensusRoot length{};length[0]=64;return test_pair(r,length);
}
CapellaExecutionPayload test_payload(const Block& b){const auto& h=b.header;CapellaExecutionPayload p;test_copy(p.parent_hash,h.parent_hash);test_copy(p.fee_recipient,unhex(h.coinbase));test_copy(p.state_root,h.state_root);test_copy(p.receipts_root,h.receipts_root);test_copy(p.logs_bloom,h.logs_bloom);test_copy(p.prev_randao,h.mix_hash);test_copy(p.block_hash,h.hash());check(h.base_fee.has_value(),"synthetic base fee");auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());test_copy(p.base_fee_per_gas,fee);p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;return p;}
Bytes test_body(const CapellaSlotState& state,const CapellaTransitionRules& r,const Bytes& reveal,const CapellaExecutionPayload& p){const auto& l=r.consensus.state;const size_t hist=176+64*l.slots_per_historical_root,sync=(l.sync_committee_size+7)/8+96,fixed=228+sync;auto payload=encode_capella_execution_payload(p,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);Bytes b(fixed,0);check(reveal.size()==96,"synthetic RANDAO width");std::copy(reveal.begin(),reveal.end(),b.begin());std::copy_n(state.ssz.begin()+hist+4,72,b.begin()+96);b[220+(l.sync_committee_size+7)/8]=0xc0;for(size_t at:{size_t(200),size_t(204),size_t(208),size_t(212),size_t(216),220+sync})test_put(b,at,fixed,4);test_put(b,224+sync,fixed+payload.size(),4);b.insert(b.end(),payload.begin(),payload.end());return b;}
Fixture fixture(bool short_exit_tenure=false){
 const auto root=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532";const auto text=read(root/"manifest.json");check(hex(sha256(bytes_of(text)))=="0xec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6","public input literal pin");const auto manifest=Json::parse(text);auto input=[&](const char* kind,const char* name){const auto& pin=manifest.at("files").at(kind);auto b=read(root/name);check(pin.at("path")==name&&pin.at("bytes").get<size_t>()==b.size()&&pin.at("sha256").get<std::string>()==hex(sha256(bytes_of(b))).substr(2),"public input bytes pin");return b;};
 const auto profile=input("profile","profile.json");auto rules=load_capella_fork_profile(profile,sha256(bytes_of(profile)),1024*1024);if(short_exit_tenure)rules.transition.consensus.exit.shard_committee_period=1;auto pre=CapellaSlotState{bytes_of(input("genesisState","genesis-state.ssz")),std::nullopt};const auto& r=rules.transition;const auto& l=r.consensus.state;
 const size_t hist=176+64*l.slots_per_historical_root,random=hist+96,previous=random+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector,field=previous+229+96*l.sync_committee_size;const size_t validators=test_little(pre.ssz,hist+88,4),balances=test_little(pre.ssz,hist+92,4);check((balances-validators)/121==64,"synthetic registry size");
 std::set<Bytes> excluded,generated;for(const auto* kind:{"publicKeys","withdrawalPublicKeys"})for(const auto& row:manifest.at(kind))excluded.insert(unhex(row.at("publicKey").get<std::string>()));const auto official_text=read(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz");check(hex(sha256(bytes_of(official_text)))=="0xf5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d","literal official/V37 public registry bytes");const auto official=bytes_of(official_text);const size_t legacy_start=test_little(official,hist+88,4),legacy_end=test_little(official,hist+92,4);check(legacy_end>=legacy_start&&(legacy_end-legacy_start)/121==64&&legacy_end<=official.size(),"official public registry bounds");for(size_t i=0;i<64;++i)excluded.emplace(official.begin()+legacy_start+121*i,official.begin()+legacy_start+121*i+48);std::map<Bytes,Bytes> remap;
 for(size_t i=0;i<64;++i){checkpoint();const size_t at=validators+121*i;Bytes old(pre.ssz.begin()+at,pre.ssz.begin()+at+48);TestKey key(i),withdrawal(10000+i);check(!excluded.contains(key.public_key)&&!excluded.contains(withdrawal.public_key)&&generated.insert(key.public_key).second&&generated.insert(withdrawal.public_key).second,"new test keys must be disjoint from public campaign roles");remap.emplace(old,key.public_key);std::copy(key.public_key.begin(),key.public_key.end(),pre.ssz.begin()+at);auto credential=sha256(withdrawal.public_key);credential[0]=0;std::copy(credential.begin(),credential.end(),pre.ssz.begin()+at+48);test_put(pre.ssz,at+80,32000000000);pre.ssz[at+88]=0;test_put(pre.ssz,at+89,0);test_put(pre.ssz,at+97,0);test_put(pre.ssz,at+105,UINT64_MAX);test_put(pre.ssz,at+113,UINT64_MAX);test_put(pre.ssz,balances+8*i,32000000000);}
 const auto domain=test_registry_root(Bytes(pre.ssz.begin()+validators,pre.ssz.begin()+balances),l.validator_registry_limit);check(domain!=test_root(pre.ssz,8)&&hex(Bytes(domain.begin(),domain.end()))!="0x0a08c27fe4ece2483f9e581f78c66379a06f96e9c24cd1390594ff939b26f95b","new domain must not equal cb532/official/V37");std::copy(domain.begin(),domain.end(),pre.ssz.begin()+8);
 for(size_t start:std::array<size_t,2>{previous+133,previous+133+48*size_t(l.sync_committee_size+1)}){std::vector<Bytes> members;for(size_t i=0;i<l.sync_committee_size;++i){const size_t at=start+48*i;Bytes old(pre.ssz.begin()+at,pre.ssz.begin()+at+48);members.push_back(remap.at(old));std::copy(members.back().begin(),members.back().end(),pre.ssz.begin()+at);}auto aggregate=aggregate_consensus_bls_keys(members);check(bool(aggregate),"synthetic sync aggregate");std::copy(aggregate->begin(),aggregate->end(),pre.ssz.begin()+start+48*l.sync_committee_size);}
 Genesis g=Genesis::from_json(Json::parse(input("genesis","genesis.json")));g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;auto& genesis=g.block;genesis.header.difficulty=0;genesis.header.timestamp=Int(std::to_string(test_little(pre.ssz,0)));genesis.header.state_root=g.state.root();genesis.header.transactions_root=transactions_root({});genesis.header.receipts_root=receipts_root({});genesis.header.withdrawals_root=withdrawals_root(Json::array());const std::string tag="Nano/Test/ProposerSlashing/v1";genesis.header.extra_data=bytes_of(tag);
 const ExecutionDepositHistory deposits(genesis.header.hash());const auto dep=deposits.root();std::copy(dep.begin(),dep.end(),pre.ssz.begin()+hist+4);test_put(pre.ssz,hist+36,0);const auto gh=genesis.header.hash();std::copy(gh.begin(),gh.end(),pre.ssz.begin()+hist+44);test_put(pre.ssz,hist+80,0);
 auto execution_header=encode_capella_execution_header(capella_execution_payload_header(test_payload(genesis),r.execution,r.withdrawals.maximum_per_payload),r.execution.extra_data_bytes,r.maximum_state_bytes);pre.ssz.resize(test_little(pre.ssz,field,4));pre.ssz.insert(pre.ssz.end(),execution_header.begin(),execution_header.end());test_put(pre.ssz,field+4,0);test_put(pre.ssz,field+12,0);test_put(pre.ssz,field+20,pre.ssz.size(),4);std::fill(pre.ssz.begin()+64,pre.ssz.begin()+176,0);
 auto zero_body=test_body(pre,r,Bytes(96),test_payload(genesis));const auto body_root=capella_beacon_body_root(zero_body,r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint);std::copy(body_root.begin(),body_root.end(),pre.ssz.begin()+144);const auto pre_root=capella_beacon_state_root(pre.ssz,l,r.execution,r.maximum_state_bytes,checkpoint);Bytes message0(84);std::copy(pre_root.begin(),pre_root.end(),message0.begin()+48);test_put(message0,80,84,4);message0.insert(message0.end(),zero_body.begin(),zero_body.end());
 auto genesis_store=initialize_capella_fork_store(pre,message0,genesis,g.state,{}, {},rules,checkpoint);const auto parent=capella_fork_head(genesis_store,rules,checkpoint);const CapellaEpochRules epochs{r.consensus.epoch,r.execution,r.maximum_state_bytes};auto advanced=prepare_capella_slots(pre,epochs,l,1,64,100000,checkpoint);auto identity=prepare_capella_proposer_identity(pre,parent->header,1,epochs,l,r.consensus.attestation.duties,64,100000,checkpoint);check(bool(identity),"synthetic slot1 proposer");TestKey proposer(identity->index);check(proposer.public_key==identity->public_key,"synthetic proposer identity");ConsensusRoot epoch{};auto reveal=proposer.sign(consensus_signing_root(epoch,consensus_domain({2,0,0,0},identity->fork_version,domain)));
 Block execution;execution.header.parent_hash=genesis.header.hash();execution.header.number=1;execution.header.timestamp=genesis.header.timestamp+Int(std::to_string(r.seconds_per_slot));execution.header.difficulty=0;execution.header.base_fee=next_base_fee(genesis.header);const auto mix=test_root(advanced.ssz,random);execution.header.mix_hash=Bytes(mix.begin(),mix.end());auto history=[&](const Int& n){check(n==0,"synthetic ancestor only");return genesis.header.hash();};State state=g.state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(state,execution,false);
 const auto parent_root=consensus_beacon_header_root(parent->header);auto body=test_body(advanced,r,reveal,test_payload(execution));auto candidate=prepare_capella_unsigned_block(pre,1,identity->index,parent_root,body,r,genesis.header,g.state,history,64,100000,checkpoint);check(bool(candidate),"genuine synthetic unsigned bootstrap");const auto& message=candidate->message_ssz;ConsensusBeaconHeader header{test_little(message,0),test_little(message,8),test_root(message,16),test_root(message,48),capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint)};
 auto signature=proposer.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},identity->fork_version,domain)));Bytes wire(100);test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());check(!prepare_capella_received_block(genesis_store,wire,rules,64,100000,checkpoint),"synthetic future slot must be refused before clock tick");const auto admission_store=prepare_capella_store_tick(genesis_store,genesis_store.timed.clock.genesis_time_ms+rules.slot_duration_ms,rules,64,100000,checkpoint);check(prepare_capella_received_block(admission_store,wire,rules,64,100000,checkpoint).has_value(),"genuine synthetic full admission");auto proof=prepare_capella_signed_bootstrap_archive(g,pre,wire,rules,64,100000,checkpoint);check(bool(proof),"genuine synthetic typed bootstrap authority");auto initial=genesis_store;
 check(r.consensus.epoch.ffg.slots_per_epoch==8,"known public minimal epoch fixture");const auto max=r.maximum_state_bytes;CapellaStoreSnapshotLimits limits{16*1024*1024,128,256,4,max,128*max,64,100000};return {std::move(g),std::move(rules),limits,std::move(pre),std::move(initial),std::move(wire),std::move(*proof)};
}
struct Pair{
 const Fixture& f;fs::path path;std::unique_ptr<Blockchain> owner;std::unique_ptr<ChainStore> witness;std::unique_ptr<CapellaDurableReactor> node;
 Pair(const Fixture& fixture,const fs::path& root,bool fresh):f(fixture),path(root){open(fresh);}
 void open(bool fresh){owner=std::make_unique<Blockchain>(path/"primary",f.genesis);witness=std::make_unique<ChainStore>(path/"witness");
  if(fresh){node=std::make_unique<CapellaDurableReactor>(*owner,f.rules,f.limits,f.initial,checkpoint,witness.get());node->tick(f.initial.timed.clock.genesis_time_ms+f.rules.slot_duration_ms,checkpoint);check(node->block_admission(f.wire,checkpoint)==CapellaBlockAdmission::accepted,"normally signed synthetic slot1 admitted");}
  else node=std::make_unique<CapellaDurableReactor>(*owner,f.rules,f.limits,*witness,checkpoint);
 }
 void close(){node.reset();owner.reset();witness.reset();}
 ForkCheckpointRecord primary()const{auto p=owner->store().fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),f.limits.envelope_bytes);check(bool(p),"primary checkpoint");return *p;}
 ForkCheckpointRecord witnessed()const{auto w=witness->fork_checkpoint(f.genesis.chain_id,f.genesis.block.header.hash(),2048+8*f.limits.envelope_bytes);check(bool(w),"independent witness checkpoint");return *w;}
 Json observation()const{const auto p=primary(),w=witnessed();const auto c=node->current();Json queue=Json::array();for(const auto& [id,wire]:node->signed_proposer_slashings())queue.push_back(Json::array({hex(id),hex(wire)}));return {{"p",p.snapshot},{"pSeq",p.token.sequence},{"w",w.snapshot},{"wSeq",w.token.sequence},{"store",capella_store_snapshot(*c.state,f.rules,f.limits,checkpoint)},{"queue",queue}};}
};
void emit(const char* name,const Json& extra=Json::object()){auto j=extra;j["scenario"]=name;j["result"]="PASS";j["newTestOnlySignaturesCreated"]=test_signatures;j["privateInputsRead"]=false;j["runtimeDutiesAuthorized"]=false;j["productionQualified"]=false;std::cout<<j.dump()<<std::endl;}
std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader> proof(const Fixture& f,uint64_t offender,uint64_t header_slot=999){
 const auto head=capella_fork_head(f.initial,f.rules,checkpoint);const auto& t=f.rules.transition;const auto ctx=capella_fork_block_context(head->state,head->header,{t.consensus.epoch,t.execution,t.maximum_state_bytes},t.consensus.state,checkpoint);TestKey key(offender);
 check(Bytes(ctx.registry.validators.at(offender).public_key.begin(),ctx.registry.validators.at(offender).public_key.end())==key.public_key,"disjoint synthetic offender key");
 ConsensusBeaconHeader a{header_slot,offender,ctx.block_root,{},{}},b=a;a.body_root.back()=1;b.body_root.back()=2;
 const auto epoch=header_slot/t.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
 return {{a,key.sign(consensus_signing_root(consensus_beacon_header_root(a),consensus_domain({0,0,0,0},version,ctx.genesis_root)))},{b,key.sign(consensus_signing_root(consensus_beacon_header_root(b),consensus_domain({0,0,0,0},version,ctx.genesis_root)))}};
}
Bytes canonical_wire(const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& p){auto wire=encode_consensus_proposer_slashing_ssz(p.first,p.second,checkpoint);if(std::lexicographical_compare(wire.begin()+208,wire.end(),wire.begin(),wire.begin()+208))std::rotate(wire.begin(),wire.begin()+208,wire.end());return wire;}
template<class Action>void unchanged_refusal(Pair& p,Action action){const auto before=p.observation();bool refused=false;try{refused=!action();}catch(const std::invalid_argument&){refused=true;}catch(const std::length_error&){refused=true;}check(refused&&p.observation()==before,"refusal changed complete checkpoint/state/queue/witness");}
void codec_case(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& p){
 auto wire=encode_consensus_proposer_slashing_ssz(p.first,p.second,checkpoint);check(wire.size()==416,"fixed proposer SSZ size");const auto original=wire;bool changed=false;auto decoded=decode_consensus_proposer_slashing_ssz(wire,[&]{if(!changed){changed=true;wire.assign(1,0);}checkpoint();});check(decoded&&encode_consensus_proposer_slashing_ssz(decoded->first,decoded->second)==original,"codec owns caller bytes across checkpoint");
 auto small=original;small.pop_back();auto large=original;large.push_back(0);check(!decode_consensus_proposer_slashing_ssz(small)&&!decode_consensus_proposer_slashing_ssz(large),"fixed SSZ size refusals");
 emit("proposer-slashing-fixed-owned-codec",{{"exactBytes",416},{"ownedInputAcrossCallbacks",true},{"wrongLengthsRefused",true}});
}
void admission_case(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& p){
 Temp temp;Pair pair(f,temp.path,true);const auto before=pair.observation();const auto old=pair.node->current();const auto wire=canonical_wire(p);InboundBudget budget;CapellaConsensusService service(*pair.node,budget,65536,32768,4096,4096,4,65536);
 auto frame=[&](const Bytes& bytes){auto j=pair.node->network_identity();j["kind"]="proposer_slashing";j["ssz"]=hex(bytes);return j;};
 for(const bool longer:{false,true}){auto bytes=wire;if(longer)bytes.push_back(0);else bytes.pop_back();std::atomic<unsigned> callbacks=0;check(!service.submit(frame(bytes).dump(),std::make_shared<std::atomic<bool>>(false),[&](auto){++callbacks;}),"wrong size admitted by service decoder");check(callbacks==0&&pair.observation()==before,"invalid decoder entered worker or changed state");}
 auto bad=wire;bad[112]^=1;auto rejected=std::make_shared<std::promise<CapellaConsensusService::Decision>>();auto rejected_future=rejected->get_future();check(service.submit(frame(bad).dump(),std::make_shared<std::atomic<bool>>(false),[rejected](auto d){rejected->set_value(d);}),"fixed malformed signature must reach full admission");check(rejected_future.wait_for(std::chrono::seconds(10))==std::future_status::ready&&rejected_future.get()==CapellaConsensusService::Decision::rejected&&pair.observation()==before,"bad signature service refusal");
 auto compact=frame(wire);compact.erase("ssz");compact["sszBase64"]=altair_base64(wire);const auto session=service.receipts().at("session").get<std::string>();const auto submitted=service.submit_receipt(session,1,compact);check(submitted.at("state")!="limited","valid receipt submitted");const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);Json final;
 do{checkpoint();final=service.receipt(session,1);if(final.at("state")=="completed")break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<end);
 check(final.at("state")=="completed"&&final.at("decision")=="accepted","genuine service proposer admission");check(service.submit_receipt(session,1,compact)==final,"same receipt ticket idempotent");service.stop();service.join();
 const auto after=pair.node->current();check(after.token.sequence==old.token.sequence+1&&after.token.digest!=old.token.digest&&capella_store_snapshot(*after.state,f.rules,f.limits,checkpoint)==before.at("store").get<std::string>(),"admission must change only durable queue, not Beacon/FFG/economics");
 check(pair.node->signed_proposer_slashings()==CapellaSignedVotes{{sha256(wire),wire}},"owned canonical proposer queue");const auto document=Json::parse(pair.primary().snapshot);check(document.at("format")=="nano-capella-durable-v5"&&document.at("proposerSlashings").size()==1,"genuine v5 envelope");
 unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(p.first,p.second,checkpoint);});unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(p.second,p.first,checkpoint);});
 auto page=pair.node->capture_inventory("proposer_slashing",4096,checkpoint)->page({},4,4096,checkpoint);check(page.at("entries").size()==1,"durably published proposer inventory");const auto id=test_root(sha256(wire));auto exported=pair.node->export_message("proposer_slashing",id,416,after.token,checkpoint);check(exported.at("available")==true&&exported.at("envelope").at("ssz").get<std::string>()==hex(wire),"exact proposer export");
 auto wrong=p;wrong.first.message.slot++;unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(wrong.first,wrong.second,checkpoint);});wrong=p;wrong.second.message.proposer_index++;unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(wrong.first,wrong.second,checkpoint);});unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(p.first,p.first,checkpoint);});wrong=p;wrong.first.signature[0]^=1;unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(wrong.first,wrong.second,checkpoint);});
 const auto state_before=pair.observation();bool cancelled=false;try{pair.node->proposer_slashing(p.first,p.second,[]{throw std::runtime_error("test cancellation");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&pair.observation()==state_before,"typed callback cancellation preserved queue and witness");
 emit("proposer-slashing-authenticated-service-admission",{{"futureHeaderSlotAcceptedWithoutDutyRestriction",true},{"economicAndForkChoiceStateUnchanged",true},{"canonicalDedupeAfterAuthentication",true},{"invalidHeadersAndSignaturesRefusedWithoutMutation",true},{"realWorkerAndReceiptAcceptedOnce",true},{"inventoryAndExportBoundToCommit",true}});
}
void formats_case(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& p){
 for(unsigned format=0;format<3;++format){Temp t;Pair pair(f,t.path,true);check(pair.node->proposer_slashing(p.first,p.second,checkpoint),"valid format queue admission");const auto queue=pair.node->signed_proposer_slashings();if(format==1)check(pair.node->migrate_reference_checkpoint({2,256*1024},checkpoint),"actual cold migration");if(format==2)check(pair.node->migrate_fragment_checkpoint(checkpoint),"actual fragment migration");const auto observed=pair.observation();const auto pin=pair.node->current().token;pair.close();pair.open(false);check(pair.node->current().token==pin&&pair.observation()==observed&&pair.node->signed_proposer_slashings()==queue,"raw queue and two-store pins retained on full reopen");
  const auto raw=pair.primary();auto fetch=[&](const Bytes& hash,size_t maximum)->std::optional<std::string>{return pair.owner->store().checkpoint_blob(f.genesis.chain_id,f.genesis.block.header.hash(),hash,maximum);};const auto closure=authenticate_capella_checkpoint_blob_closure(raw.snapshot,raw.token.digest,f.rules,f.limits,fetch,{},checkpoint);check(closure.head_context().registry.validators.size()==64,"full v5 head/queue authentication in all formats");
 }
 emit("proposer-slashing-hot-cold-fragmented-reopen",{{"realFormats",3},{"exactRawSnapshotAndWitnessRetained",true},{"fullHeadAndQueueAuthenticated",true}});
}
ConsensusBeaconHeader message_header(const Bytes& message,const Fixture& f){const auto& r=f.rules.transition;return {test_little(message,0),test_little(message,8),test_root(message,16),test_root(message,48),capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),r.consensus.body,r.consensus.state.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint)};}
Bytes signed_candidate(const CapellaPublishedStore& current,const CapellaForkBlock& parent,uint64_t slot,uint64_t proposer,const Bytes& body,const Fixture& f,Phase0ForkSlashingContext* context_out=nullptr){
 const auto& r=f.rules.transition;const auto root=consensus_beacon_header_root(parent.header);auto history=[&](const Int& n){return capella_execution_history(*current.state,root,n,checkpoint);};auto candidate=prepare_capella_unsigned_block(parent.state,slot,proposer,root,body,r,parent.execution_block.header,parent.execution_state,history,f.limits.replay_slots,f.limits.sync_trials,checkpoint);check(bool(candidate),"full unsigned proposer test transition");auto header=message_header(candidate->message_ssz,f);const auto context=capella_fork_block_context(candidate->post.consensus,header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint);if(context_out)*context_out=context;const auto epoch=slot/r.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<context.fork.epoch?context.fork.previous_version:context.fork.current_version;TestKey key(proposer);auto signature=key.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},version,context.genesis_root)));Bytes wire(100);test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),candidate->message_ssz.begin(),candidate->message_ssz.end());return wire;
}
std::optional<Bytes> local_wire(Pair& pair,uint64_t slot,Bytes* body_out=nullptr){
 const auto& f=pair.f;pair.node->tick(f.initial.timed.clock.genesis_time_ms+slot*f.rules.slot_duration_ms,checkpoint);const auto proposer=pair.node->current_proposer(slot,checkpoint);if(!proposer)return {};
 const auto current=pair.node->current();const auto parent=capella_fork_head(*current.state,f.rules,checkpoint);const auto& r=f.rules.transition;const auto ctx=capella_fork_block_context(parent->state,parent->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint);const auto epoch=slot/r.consensus.epoch.ffg.slots_per_epoch;const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;ConsensusRoot epoch_root{};for(size_t i=0;i<8;++i)epoch_root[i]=uint8_t(epoch>>(8*i));TestKey key(*proposer);const auto reveal=key.sign(consensus_signing_root(epoch_root,consensus_domain({2,0,0,0},version,ctx.genesis_root)));
 auto body=pair.node->prepare_local_body_from_pool(*proposer,slot,reveal,f.genesis.block.header.coinbase,pair.node->current().token,1,64,checkpoint);check(bool(body),"normal local proposal body");if(body_out)*body_out=*body;return signed_candidate(current,*parent,slot,*proposer,*body,f);
}
Bytes without_proposer(Bytes body,const Fixture& f){const auto start=test_little(body,200,4),end=test_little(body,204,4);check(end-start==416,"local body must include exact proposer SSZ");body.erase(body.begin()+start,body.begin()+end);const size_t sync=(f.rules.transition.consensus.state.sync_committee_size+7)/8+96;for(size_t at:{size_t(204),size_t(208),size_t(212),size_t(216),220+sync,224+sync})test_put(body,at,test_little(body,at,4)-416,4);return body;}
void vote_for(Pair& pair,uint64_t slot,const ConsensusRoot& head){
 pair.node->tick(pair.f.initial.timed.clock.genesis_time_ms+slot*pair.f.rules.slot_duration_ms+pair.f.rules.attestation_due_ms,checkpoint);const auto current=pair.node->current();auto duties=prepare_capella_attestation_duties(*current.state,pair.f.rules,slot,pair.f.limits.replay_slots,pair.f.limits.sync_trials,checkpoint);check(!duties.empty(),"real committee duties available");
 for(auto& [index,duty]:duties){duty.attestation.data.beacon_block_root=head;TestKey key(index);duty.attestation.signature=key.sign(consensus_signing_root(consensus_attestation_root(duty.attestation.data),consensus_domain({1,0,0,0},duty.fork_version,duty.genesis_root)));check(pair.node->vote(duty.attestation,checkpoint),"fully signed committee vote admission");}
}
void inclusion_reorg_finality_case(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& p){
 Temp t;Pair pair(f,t.path,true);check(pair.node->proposer_slashing(p.first,p.second,checkpoint),"queued proposer proof");const auto queued=pair.node->signed_proposer_slashings();const auto current=pair.node->current();const auto parent=capella_fork_head(*current.state,f.rules,checkpoint);const auto& r=f.rules.transition;const auto old_context=capella_fork_block_context(parent->state,parent->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint);
 uint64_t slot=2;Bytes body;std::optional<Bytes> wire;for(;slot<8;++slot){pair.node->tick(f.initial.timed.clock.genesis_time_ms+slot*f.rules.slot_duration_ms,checkpoint);const auto proposer=pair.node->current_proposer(slot,checkpoint);if(proposer&&*proposer!=0){wire=local_wire(pair,slot,&body);break;}}check(wire.has_value()&&slot<8,"finite nonoffender inclusion slot");const auto proposer=test_little(*wire,108);Phase0ForkSlashingContext baseline;const auto alt=signed_candidate(current,*parent,slot,proposer,without_proposer(body,f),f,&baseline);const auto alt_header=message_header(Bytes(alt.begin()+100,alt.end()),f);const auto alt_root=consensus_beacon_header_root(alt_header);
 check(pair.node->block_admission(*wire,checkpoint)==CapellaBlockAdmission::accepted,"normally signed block with proposer proof admitted");const auto included=capella_fork_head(*pair.node->current().state,f.rules,checkpoint);const auto included_context=capella_fork_block_context(included->state,included->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint);const auto& offender=included_context.registry.validators.at(0);check(offender.slashed&&offender.exit_epoch!=UINT64_MAX&&offender.withdrawable_epoch>offender.exit_epoch,"real sanction economics and exit queue");check(offender.balance==baseline.registry.validators.at(0).balance-old_context.registry.validators.at(0).effective_balance/r.consensus.epoch.slashing.minimum_penalty_quotient,"exact immediate slashing debit");check(pair.node->signed_proposer_slashings()==queued,"unfinalized inclusion must retain evidence");unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(p.first,p.second,checkpoint);});
 check(pair.node->block_admission(alt,checkpoint)==CapellaBlockAdmission::accepted,"genuine competing unslashed branch admitted");vote_for(pair,slot+1,alt_root);check(consensus_beacon_header_root(capella_fork_head(*pair.node->current().state,f.rules,checkpoint)->header)==alt_root,"valid votes must switch to unslashed branch");check(pair.node->signed_proposer_slashings()==queued,"reorg cannot lose unfinalized proposer evidence");const auto observed=pair.observation();pair.close();pair.open(false);check(pair.observation()==observed,"reorg restart retains exact queue and world state");
 bool reincluded=false;for(uint64_t next=slot+2;next<16&&!reincluded;++next){auto block=local_wire(pair,next);if(!block)continue;check(pair.node->block_admission(*block,checkpoint)==CapellaBlockAdmission::accepted,"reorg local proposal admission");const auto head=capella_fork_head(*pair.node->current().state,f.rules,checkpoint);const auto ctx=capella_fork_block_context(head->state,head->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint);reincluded=ctx.registry.validators.at(0).slashed;slot=next;}check(reincluded&&pair.node->signed_proposer_slashings()==queued,"automatic reinclusion after restart");
 emit("proposer-slashing-inclusion-reorg-and-reinclusion",{{"economicSlashAppliedOnlyByBlock",true},{"exactDebit",true},{"realSignedCompetingBranchAndVotes",true},{"unfinalizedEvidenceSurvivesRestart",true},{"normalLocalBodyAutomaticallyReincludes",true}});
 test_stage="finalization-enable-pruning";test_slot=slot;pair.node->set_automatic_pruning(true);bool retired=false;for(uint64_t next=slot+1;next<=48&&!retired;++next){test_slot=next;test_stage="finalization-local-proposal";auto block=local_wire(pair,next);if(block){test_stage="finalization-block-admission";check(pair.node->block_admission(*block,checkpoint)==CapellaBlockAdmission::accepted,"finite finalized chain proposal");}test_stage="finalization-head-read";const auto head=consensus_beacon_header_root(capella_fork_head(*pair.node->current().state,f.rules,checkpoint)->header);test_stage="finalization-committee-votes";vote_for(pair,next,head);test_stage="finalization-queue-retirement-read";retired=pair.node->signed_proposer_slashings().empty();}
 test_stage="finalization-finalized-state-read";check(retired,"finalized slashing must retire proposer evidence within fixture bound");const auto c=pair.node->current();const auto final=c.state->blocks.at(c.state->timed.view.finalized.root).lease(checkpoint);check(capella_fork_block_context(final->state,final->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},r.consensus.state,checkpoint).registry.validators.at(0).slashed,"retirement requires actual finalized sanction");check(pair.primary().snapshot.find("nano-capella-durable-v5")==std::string::npos,"empty proposer queue returns to legacy writer");test_stage="finalization-reopen";const auto after=pair.observation();pair.close();pair.open(false);check(pair.observation()==after&&pair.node->signed_proposer_slashings().empty(),"finalized retirement and pruning reopen");
 emit("proposer-slashing-finalized-retirement-and-pruning",{{"genuineFinalizedSlashedState",true},{"neverRetiredAtPoolAdmissionOrUnfinalizedHead",true},{"legacyWriterWhenQueueEmpty",true},{"closedOwnerReopen",true}});
}
void capacity_case(const Fixture& f,const std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>& p){
 auto bounded=f;bounded.limits.equivocations=1;Temp t;Pair pair(bounded,t.path,true);check(pair.node->proposer_slashing(p.first,p.second,checkpoint),"one bounded proof admitted");auto second=proof(f,1);unchanged_refusal(pair,[&]{return pair.node->proposer_slashing(second.first,second.second,checkpoint);});const auto first=pair.primary();pair.close();pair.open(false);check(pair.primary().snapshot==first.snapshot&&pair.node->signed_proposer_slashings().size()==1,"bounded queue retained");
 // A forged queue with a freshly recomputed envelope digest must still fail
 // full cryptographic restoration. This is an owned closed synthetic DB only.
 auto corrupt=Json::parse(pair.primary().snapshot);auto forged=unhex(corrupt.at("proposerSlashings")[0][3].get<std::string>());forged[112]^=1;corrupt["proposerSlashings"][0][0]=hex(sha256(forged));corrupt["proposerSlashings"][0][3]=hex(forged);const auto bad_snapshot=corrupt.dump();const ForkCheckpointToken bad_pin{first.token.sequence,sha256(bytes_of(bad_snapshot))};const auto original_execution_tip=hex(pair.owner->tip().header.hash());pair.close();
 {rocksdb::Options options;apply_chain_store_memory_policy(options);options.create_if_missing=false;auto db=test::open_owned_fixture(options,(t.path/"primary").string());std::string original_marker_text;check(db->Get(rocksdb::ReadOptions{},"execution-checkpoint/v1",&original_marker_text).ok(),"original execution marker exists");auto marker=Json::parse(original_marker_text);check(marker.is_object()&&marker.size()==2&&marker.at("digest").is_string()&&marker.at("tip").is_string()&&marker.at("digest").get<std::string>()==hex(first.token.digest)&&marker.at("tip").get<std::string>()==original_execution_tip,"original exact execution marker binding");marker["digest"]=hex(bad_pin.digest);rocksdb::WriteBatch batch;batch.Put("execution-checkpoint/v1",marker.dump());batch.Put("fork-checkpoint/v1/payload",bad_snapshot);batch.Put("fork-checkpoint/v1/token",Json{{"sequence",quantity(Int(std::to_string(bad_pin.sequence)))},{"digest",hex(bad_pin.digest)}}.dump());rocksdb::WriteOptions write;write.sync=true;check(db->Write(write,&batch).ok(),"owned corrupt envelope installed");}
 {Blockchain owner(t.path/"primary",bounded.genesis);const auto before=owner.store().fork_checkpoint(bounded.genesis.chain_id,bounded.genesis.block.header.hash(),bounded.limits.envelope_bytes,bad_pin);check(before&&before->snapshot==bad_snapshot,"corrupt envelope itself has a valid digest");bool refused=false;try{CapellaDurableReactor invalid(owner,bounded.rules,bounded.limits,bad_pin,checkpoint);}catch(const std::invalid_argument&){refused=true;}check(refused,"forged signature restored from v5 queue");const auto after=owner.store().fork_checkpoint(bounded.genesis.chain_id,bounded.genesis.block.header.hash(),bounded.limits.envelope_bytes,bad_pin);check(after&&after->snapshot==bad_snapshot&&after->token==bad_pin,"failed full restoration cannot repair or rewrite corrupted checkpoint");}
 emit("proposer-slashing-bounded-pool-refusals",{{"configuredEquivocationQuota",1},{"secondOffenderRefusedWithoutPartialState",true},{"perOperationWireBytes",416},{"noImplicitQuotaIncrease",true}});
}
// A delayed honest branch must not turn an accepted exit into disposable metadata.
// Even/odd proposal slots avoid signing conflicting proposals at one slot; only A votes.
void exit_reorg_prune_case(const Fixture& f,bool defer=false){
 check(f.rules.transition.consensus.exit.shard_committee_period==1,"exit fixture tenure must be explicit");
 Temp directory;Pair producer(f,directory.path/"producer",true),follower(f,directory.path/"follower",true);
 const auto& r=f.rules.transition;const auto slots=r.consensus.epoch.ffg.slots_per_epoch;
 const CapellaEpochRules epochs{r.consensus.epoch,r.execution,r.maximum_state_bytes};
 const uint64_t exit_epoch=defer?5:4,admission_slot=exit_epoch*slots+1,last_a_slot=4*slots;
 check(slots==8&&admission_slot==(defer?41:33),"finite minimal exit fixture slots");
 const auto initial_root=follower.node->current().state->timed.view.finalized.root;
 std::vector<Bytes> a_wires;std::set<uint64_t> a_slots,b_slots;size_t imported=0;
 for(uint64_t slot=2;slot<=last_a_slot;++slot){
  checkpoint();test_slot=slot;test_stage="exit-prune-build-branch-a";
  if(slot%2==0){
   auto wire=local_wire(producer,slot);check(bool(wire),"branch A assigned proposer missing");
   check(producer.node->block_admission(*wire,checkpoint)==CapellaBlockAdmission::accepted,"branch A signed block refused");
   check(a_slots.insert(test_little(*wire,100)).second,"branch A duplicate proposal slot");a_wires.push_back(std::move(*wire));
  }
  const auto head=consensus_beacon_header_root(capella_fork_head(*producer.node->current().state,f.rules,checkpoint)->header);
  vote_for(producer,slot,head);
 }
 ConsensusRoot expected_head{};
 {const auto a_current=producer.node->current();expected_head=consensus_beacon_header_root(capella_fork_head(*a_current.state,f.rules,checkpoint)->header);
 check(a_current.state->timed.view.finalized.root!=initial_root&&a_current.state->timed.view.finalized.epoch<exit_epoch,"branch A must genuinely finalize an older competing checkpoint");}
 // Only signed wires and scalar commitments are needed from A during import.
 producer.close();
 for(uint64_t slot:std::array<uint64_t,2>{3,admission_slot}){
  checkpoint();test_slot=slot;test_stage="exit-prune-build-branch-b";
  follower.node->tick(f.initial.timed.clock.genesis_time_ms+slot*f.rules.slot_duration_ms,checkpoint);
  bool ready=false;for(unsigned trial=0;trial<8&&!ready;++trial)ready=follower.node->prepare_current_duties(slot,checkpoint);
  check(ready,"bounded normal B duty preparation continuation");
  auto wire=local_wire(follower,slot);check(bool(wire),"branch B assigned proposer missing");
  check(!a_slots.contains(test_little(*wire,100))&&b_slots.insert(test_little(*wire,100)).second,"two proposals signed at one slot");
  check(follower.node->block_admission(*wire,checkpoint)==CapellaBlockAdmission::accepted,"branch B signed block refused");
 }
 ConsensusRoot admission_root{};std::array<uint8_t,48> admitted_key{};Bytes exit_wire;
 {test_stage="exit-prune-admit-on-b";const auto admitted_on=follower.node->current();
 check(admitted_on.state->timed.view.finalized.root==initial_root,"B was built under an already conflicting finalized checkpoint");
 const auto b_head=capella_fork_head(*admitted_on.state,f.rules,checkpoint);admission_root=consensus_beacon_header_root(b_head->header);
 const auto b_context=capella_fork_block_context(b_head->state,b_head->header,epochs,r.consensus.state,checkpoint);
 const auto validator_before=b_context.registry.validators.at(0);admitted_key=validator_before.public_key;check(validator_before.activation_epoch==0&&validator_before.exit_epoch==UINT64_MAX&&!validator_before.slashed,"genesis exit actor not active and unscheduled");
 SignedConsensusVoluntaryExit operation{exit_epoch,0,{}};const auto version=exit_epoch<b_context.fork.epoch?b_context.fork.previous_version:b_context.fork.current_version;
 TestKey actor(0);operation.signature=actor.sign(consensus_signing_root(consensus_voluntary_exit_root(exit_epoch,0),consensus_domain({4,0,0,0},version,b_context.genesis_root)));
 exit_wire=encode_consensus_voluntary_exit_ssz(operation);
 check(follower.node->voluntary_exit_ssz(exit_wire,checkpoint),"valid B exit was not acknowledged");}
 auto archive=[&](){return Json::parse(follower.primary().snapshot).value("signedExits",Json::array());};
 auto assert_queue=[&](const char* message){const auto rows=archive();check(rows.size()==1&&rows[0][0]==hex(sha256(exit_wire))&&rows[0][2]==hex(exit_wire),message);};
 assert_queue("acknowledged B exit not durably recorded");{const auto acknowledged=follower.observation();follower.close();follower.open(false);check(follower.observation()==acknowledged,"acknowledged B exit changed on closed-owner reopen");}
 check(follower.node->current().state->timed.view.finalized.root==initial_root,"A finality leaked into B before signed import");
 test_stage="exit-prune-delayed-branch-import";
 for(const auto& wire:a_wires){check(follower.node->block_admission(wire,checkpoint)==CapellaBlockAdmission::accepted,"genuine delayed A block refused");++imported;}
 {const auto switched=follower.node->current();
 check(consensus_beacon_header_root(capella_fork_head(*switched.state,f.rules,checkpoint)->header)==expected_head,"delayed signed chain did not select head A");
 check(switched.state->timed.view.finalized.root!=initial_root&&switched.state->timed.view.finalized.epoch<exit_epoch,"real import did not establish F older than exit");
 check(switched.state->blocks.contains(admission_root),"B admission root disappeared before pruning");assert_queue("reorg alone lost accepted exit");}
 // Only normal ticking expires live votes. No fabricated finalized root or registry mutation.
 const auto expiry_slot=(exit_epoch+2)*slots;test_slot=expiry_slot;test_stage="exit-prune-normal-vote-expiry";
 follower.node->tick(f.initial.timed.clock.genesis_time_ms+expiry_slot*f.rules.slot_duration_ms,checkpoint);
 {const auto current=follower.node->current();const auto finalized=current.state->timed.view.finalized;
 check(finalized.root!=initial_root&&finalized.epoch<exit_epoch,"tick must not manufacture the required older finality");
 const auto head=capella_fork_head(*current.state,f.rules,checkpoint),final=current.state->blocks.at(finalized.root).lease(checkpoint);
 const auto head_context=capella_fork_block_context(head->state,head->header,epochs,r.consensus.state,checkpoint);
 const auto final_context=capella_fork_block_context(final->state,final->header,epochs,r.consensus.state,checkpoint);
 check((defer?head_context.registry.epoch<exit_epoch:head_context.registry.epoch>=exit_epoch)&&final_context.registry.epoch<exit_epoch,"head/older final-state epoch relation missing");
 for(const auto* context:{&head_context,&final_context}){const auto& v=context->registry.validators.at(0);check(v.public_key==admitted_key&&v.activation_epoch==0&&v.exit_epoch==UINT64_MAX&&!v.slashed,"identity or finalized scheduling changed");}
 check(consensus_beacon_header_root(head->header)==expected_head,"normal tick changed selected A head");
 const auto pruned=prepare_capella_pruned_store(*current.state,finalized.root,f.rules,checkpoint);
 check(!pruned.blocks.contains(admission_root)&&pruned.blocks.contains(finalized.root)&&consensus_beacon_header_root(capella_fork_head(pruned,f.rules,checkpoint)->header)==expected_head,"real pruning must remove B without changing head A");
 if(defer)for(const auto& [_,block]:pruned.blocks){checkpoint();check(block.header().slot/slots<exit_epoch,"deferred pruning must have no retained exit-eligible post-state");}
 for(const auto& [_,wire]:follower.node->signed_votes()){const auto vote=decode_consensus_attestation_ssz(wire,r.consensus.body.validators_per_committee,checkpoint);check(vote&&pruned.blocks.contains(vote->data.target.root)&&pruned.blocks.contains(vote->data.beacon_block_root),"recent off-branch vote still prevents this pruning");}
 assert_queue("exit lost before real pruning call");
 emit(defer?"exit-reorg-prune-deferred-preconditions":"exit-reorg-prune-preconditions",{{"exitEpoch",exit_epoch},{"finalizedEpoch",finalized.epoch},{"finalizedStateEpoch",final_context.registry.epoch},{"headStateEpoch",head_context.registry.epoch},{"branchAImportedBlocks",imported},{"branchBBlocks",b_slots.size()},{"proposalSlotsDisjoint",true},{"sameValidatorIdentity",true},{"noExitIncluded",true},{"oldAdmissionRootWillBeRemoved",true},{"liveVoteRows",follower.node->signed_votes().size()}});}
 if(defer){
  test_stage="exit-prune-manual-deferral";
  {const auto before=follower.observation();
   check(!follower.node->prune_finalized(checkpoint),"pruning without an eligible retained exit anchor must defer");
   check(follower.observation()==before,"deferred manual pruning changed exact primary/witness/store/queue");assert_queue("deferred manual pruning lost exit bytes");
  }
  const auto old_anchor=follower.node->current().state->timed.view.anchor;const auto before_tick=follower.node->current().token;
  follower.node->set_automatic_pruning(true);test_stage="exit-prune-automatic-deferral";test_slot=expiry_slot+1;
  const auto expected_time=f.initial.timed.clock.genesis_time_ms+test_slot*f.rules.slot_duration_ms;
  follower.node->tick(expected_time,checkpoint);
  {const auto after=follower.node->current();
   check(after.state->timed.clock.time_ms==expected_time&&after.token.sequence==before_tick.sequence+1,"automatic deferral must preserve exactly the normal tick publication");
   check(after.state->timed.view.anchor==old_anchor&&after.state->blocks.contains(admission_root),"automatic deferral published a partial pruning");
   check(consensus_beacon_header_root(capella_fork_head(*after.state,f.rules,checkpoint)->header)==expected_head,"automatic deferral changed head");assert_queue("automatic deferral lost accepted exit");
  }
  {const auto stable=follower.observation();follower.close();follower.open(false);check(follower.observation()==stable,"deferred exit/witness checkpoint changed on reopen");assert_queue("deferred exit lost on reopen");}
  emit("exit-reorg-prune-deferred-summary",{{"semanticGroups",1},{"genuineDelayedBranchImport",true},{"retainedPostStatesBeforeExitEpoch",true},{"manualPruningDeferredWithoutMutation",true},{"automaticTickPublishedWithoutPruning",true},{"acceptedExitBytesPreserved",true},{"closedOwnerReopen",true},{"syntheticFixtureOnly",true},{"actualNetworkTransport",false}});return;
 }
 test_stage="exit-prune-accepted-operation-invariant";
 check(follower.node->prune_finalized(checkpoint),"real pruning failed before the accepted-operation invariant");
 {const auto after=follower.node->current();const auto rows=archive();
 std::cout<<Json{{"scenario","exit-reorg-prune-observation"},{"result",rows.size()==1?"PRESERVED":"LOST"},{"queueBefore",1},{"queueAfter",rows.size()},{"exitWireSha256",hex(sha256(exit_wire))},{"oldAdmissionRootRemoved",!after.state->blocks.contains(admission_root)},{"headUnchanged",consensus_beacon_header_root(capella_fork_head(*after.state,f.rules,checkpoint)->header)==expected_head},{"privateInputsRead",false},{"productionQualified",false}}.dump()<<std::endl;
 assert_queue("accepted signed exit lost when pruning orphan admission root against an older finalized state");
 check(follower.node->pending_exits().size()==1,"preserved exit no longer selectable on eligible head A");}
 const auto token=follower.node->current().token;
 {const auto stable=follower.observation();follower.close();follower.open(false);
 check(follower.node->current().token==token&&follower.observation()==stable,"pruned exit/witness checkpoint changed on reopen");assert_queue("reopen lost preserved exit bytes");}
 check(follower.node->voluntary_exit_ssz(exit_wire,checkpoint)&&follower.node->current().token==token,"duplicate accepted exit rewrote checkpoint after pruning");
 test_stage="exit-prune-single-reinclusion";test_slot=expiry_slot+1;
 follower.node->tick(f.initial.timed.clock.genesis_time_ms+test_slot*f.rules.slot_duration_ms,checkpoint);
 bool ready=false;for(unsigned trial=0;trial<8&&!ready;++trial)ready=follower.node->prepare_current_duties(test_slot,checkpoint);
 check(ready,"bounded normal reinclusion duty preparation continuation");
 auto included=local_wire(follower,expiry_slot+1);check(bool(included),"normal proposal after pruning missing");
 check(follower.node->block_admission(*included,checkpoint)==CapellaBlockAdmission::accepted,"normal proposal failed to include preserved exit");
 const auto selected=capella_fork_head(*follower.node->current().state,f.rules,checkpoint);const auto included_context=capella_fork_block_context(selected->state,selected->header,epochs,r.consensus.state,checkpoint);
 check(included_context.registry.validators.at(0).exit_epoch!=UINT64_MAX&&follower.node->pending_exits().empty(),"exit not scheduled exactly once by normal block transition");assert_queue("unfinalized inclusion prematurely retired exit archive");
 emit("exit-reorg-prune-summary",{{"semanticGroups",1},{"genuineDelayedBranchImport",true},{"finalityAdvancedAfterAcknowledgement",true},{"actualPruningAndClosedOwnerReopen",true},{"acceptedExitBytesPreserved",true},{"duplicateIdempotent",true},{"normalBlockReinclusion",true},{"syntheticFixtureOnly",true},{"actualNetworkTransport",false}});
}
} // namespace
int main(int argc,char** argv){try{const bool defer_case=argc==2&&std::string(argv[1])=="--exit-reorg-prune-deferred-self-test";const bool exit_case=argc==2&&(defer_case||std::string(argv[1])=="--exit-reorg-prune-self-test");check(argc==2&&(exit_case||std::string(argv[1])=="--self-test"),"proposer test selector");ScopedChainStoreMemoryPolicy memory({4*1024*1024,2,16*1024*1024,0});auto f=fixture(exit_case);if(exit_case){test_stage="exit-reorg-prune";exit_reorg_prune_case(f,defer_case);return 0;}auto p=proof(f,0);test_stage="codec";codec_case(f,p);test_stage="admission";admission_case(f,p);test_stage="formats";formats_case(f,p);test_stage="inclusion-reorg-reinclusion";inclusion_reorg_finality_case(f,p);test_stage="capacity";test_slot=0;capacity_case(f,p);test_stage="summary";checkpoint();emit("proposer-slashing-summary",{{"semanticGroups",6},{"actualPeakRssKiB",public_peak_rss_kib()},{"maximumRssKiB",131072},{"blockCacheBytes",0},{"fourMiBStrictCacheResourceQualification",false},{"syntheticFixtureOnly",true},{"fixtureValidators",64},{"signatureCeiling",1024},{"actualNetworkTransport",false},{"physicalPowerLossQualified",false},{"historicalSnapshotsAltered",false},{"completeProductionQualification",false}});return 0;}catch(const std::exception& e){std::cerr<<Json{{"diagnostic","proposer-slashing-test-failure-v1"},{"stage",test_stage},{"syntheticSlot",test_slot},{"actualPeakRssKiB",public_peak_rss_kib()},{"maximumRssKiB",131072},{"blockCacheBytes",0},{"privateInputsRead",false},{"runtimeDutiesAuthorized",false},{"productionQualified",false}}.dump()<<std::endl;std::cerr<<e.what()<<std::endl;return 1;}}
