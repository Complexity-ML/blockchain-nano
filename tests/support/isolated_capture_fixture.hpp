#pragma once
// Selected public proposer-fixture helpers only; no historical test group/main.
// The optional slot512 anchor is obtained by a genuine complete signed transition,
// preserving the original exit tenure/profile and replay64 on tested reactors.
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
#include <openssl/crypto.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sys/resource.h>
#include <unistd.h>
namespace nano::test::isolated_capture {
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
void checkpoint(){check(std::chrono::steady_clock::now()-started<std::chrono::seconds(180),"owned-admission test wall bound");rusage u{};check(::getrusage(RUSAGE_SELF,&u)==0,"owned-admission test resource read");
#ifdef __APPLE__
 check(u.ru_maxrss/1024<=131072,"owned-admission test RSS bound");
#else
 check(u.ru_maxrss<=131072,"owned-admission test RSS bound");
#endif
}
std::string read(const fs::path& path){std::ifstream in(path,std::ios::binary);check(bool(in),"public fixture read");std::string s((std::istreambuf_iterator<char>(in)),{});check(s.size()<=32*1024*1024,"public fixture input bound");return s;}
Bytes bytes_of(const std::string& s){return {s.begin(),s.end()};}
struct Temp{fs::path path;Temp(){auto pattern=(fs::temp_directory_path()/"nano-durable-owned-XXXXXX").string();check(::mkdtemp(pattern.data()),"owned temporary fixture");path=fs::canonical(pattern);}~Temp(){std::error_code e;fs::remove_all(path,e);}};
struct Fixture{Genesis genesis;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaSlotState pre;CapellaForkStore initial;Bytes wire;Json genesis_document;};
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
Fixture fixture(uint64_t first_slot=1){
 check(first_slot==1||first_slot==512,"two fixed public fixture anchor slots");const uint64_t maximum=std::max<uint64_t>(64,first_slot);
 const auto root=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532";const auto text=read(root/"manifest.json");check(hex(sha256(bytes_of(text)))=="0xec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6","public input literal pin");const auto manifest=Json::parse(text);auto input=[&](const char* kind,const char* name){const auto& pin=manifest.at("files").at(kind);auto b=read(root/name);check(pin.at("path")==name&&pin.at("bytes").get<size_t>()==b.size()&&pin.at("sha256").get<std::string>()==hex(sha256(bytes_of(b))).substr(2),"public input bytes pin");return b;};
 const auto profile=input("profile","profile.json");auto rules=load_capella_fork_profile(profile,sha256(bytes_of(profile)),1024*1024);auto pre=CapellaSlotState{bytes_of(input("genesisState","genesis-state.ssz")),std::nullopt};const auto& r=rules.transition;const auto& l=r.consensus.state;
 const size_t hist=176+64*l.slots_per_historical_root,random=hist+96,previous=random+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector,field=previous+229+96*l.sync_committee_size;const size_t validators=test_little(pre.ssz,hist+88,4),balances=test_little(pre.ssz,hist+92,4);check((balances-validators)/121==64,"synthetic registry size");
 std::set<Bytes> excluded,generated;for(const auto* kind:{"publicKeys","withdrawalPublicKeys"})for(const auto& row:manifest.at(kind))excluded.insert(unhex(row.at("publicKey").get<std::string>()));const auto official_text=read(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz");check(hex(sha256(bytes_of(official_text)))=="0xf5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d","literal official/V37 public registry bytes");const auto official=bytes_of(official_text);const size_t legacy_start=test_little(official,hist+88,4),legacy_end=test_little(official,hist+92,4);check(legacy_end>=legacy_start&&(legacy_end-legacy_start)/121==64&&legacy_end<=official.size(),"official public registry bounds");for(size_t i=0;i<64;++i)excluded.emplace(official.begin()+legacy_start+121*i,official.begin()+legacy_start+121*i+48);std::map<Bytes,Bytes> remap;
 for(size_t i=0;i<64;++i){checkpoint();const size_t at=validators+121*i;Bytes old(pre.ssz.begin()+at,pre.ssz.begin()+at+48);TestKey key(i),withdrawal(10000+i);check(!excluded.contains(key.public_key)&&!excluded.contains(withdrawal.public_key)&&generated.insert(key.public_key).second&&generated.insert(withdrawal.public_key).second,"new test keys must be disjoint from public campaign roles");remap.emplace(old,key.public_key);std::copy(key.public_key.begin(),key.public_key.end(),pre.ssz.begin()+at);auto credential=sha256(withdrawal.public_key);credential[0]=0;std::copy(credential.begin(),credential.end(),pre.ssz.begin()+at+48);test_put(pre.ssz,at+80,32000000000);pre.ssz[at+88]=0;test_put(pre.ssz,at+89,0);test_put(pre.ssz,at+97,0);test_put(pre.ssz,at+105,UINT64_MAX);test_put(pre.ssz,at+113,UINT64_MAX);test_put(pre.ssz,balances+8*i,32000000000);}
 const auto domain=test_registry_root(Bytes(pre.ssz.begin()+validators,pre.ssz.begin()+balances),l.validator_registry_limit);check(domain!=test_root(pre.ssz,8)&&hex(Bytes(domain.begin(),domain.end()))!="0x0a08c27fe4ece2483f9e581f78c66379a06f96e9c24cd1390594ff939b26f95b","new domain must not equal cb532/official/V37");std::copy(domain.begin(),domain.end(),pre.ssz.begin()+8);
 for(size_t start:std::array<size_t,2>{previous+133,previous+133+48*size_t(l.sync_committee_size+1)}){std::vector<Bytes> members;for(size_t i=0;i<l.sync_committee_size;++i){const size_t at=start+48*i;Bytes old(pre.ssz.begin()+at,pre.ssz.begin()+at+48);members.push_back(remap.at(old));std::copy(members.back().begin(),members.back().end(),pre.ssz.begin()+at);}auto aggregate=aggregate_consensus_bls_keys(members);check(bool(aggregate),"synthetic sync aggregate");std::copy(aggregate->begin(),aggregate->end(),pre.ssz.begin()+start+48*l.sync_committee_size);}
 auto genesis_document=Json::parse(input("genesis","genesis.json"));genesis_document["config"]["genesisHeaderVersion"]=2;genesis_document["timestamp"]=quantity(Int(std::to_string(test_little(pre.ssz,0))));genesis_document["extraData"]=hex(bytes_of("Nano/Test/StoppedCapture/v1"));Genesis g=Genesis::from_json(genesis_document);g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;auto& genesis=g.block;genesis.header.difficulty=0;genesis.header.timestamp=Int(std::to_string(test_little(pre.ssz,0)));genesis.header.state_root=g.state.root();genesis.header.transactions_root=transactions_root({});genesis.header.receipts_root=receipts_root({});genesis.header.withdrawals_root=withdrawals_root(Json::array());check(Genesis::from_json(genesis_document).block.header.hash()==genesis.header.hash(),"round-trippable public test Genesis");
 const ExecutionDepositHistory deposits(genesis.header.hash());const auto dep=deposits.root();std::copy(dep.begin(),dep.end(),pre.ssz.begin()+hist+4);test_put(pre.ssz,hist+36,0);const auto gh=genesis.header.hash();std::copy(gh.begin(),gh.end(),pre.ssz.begin()+hist+44);test_put(pre.ssz,hist+80,0);
 auto execution_header=encode_capella_execution_header(capella_execution_payload_header(test_payload(genesis),r.execution,r.withdrawals.maximum_per_payload),r.execution.extra_data_bytes,r.maximum_state_bytes);pre.ssz.resize(test_little(pre.ssz,field,4));pre.ssz.insert(pre.ssz.end(),execution_header.begin(),execution_header.end());test_put(pre.ssz,field+4,0);test_put(pre.ssz,field+12,0);test_put(pre.ssz,field+20,pre.ssz.size(),4);std::fill(pre.ssz.begin()+64,pre.ssz.begin()+176,0);
 auto zero_body=test_body(pre,r,Bytes(96),test_payload(genesis));const auto body_root=capella_beacon_body_root(zero_body,r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint);std::copy(body_root.begin(),body_root.end(),pre.ssz.begin()+144);const auto pre_root=capella_beacon_state_root(pre.ssz,l,r.execution,r.maximum_state_bytes,checkpoint);Bytes message0(84);std::copy(pre_root.begin(),pre_root.end(),message0.begin()+48);test_put(message0,80,84,4);message0.insert(message0.end(),zero_body.begin(),zero_body.end());
 auto genesis_store=initialize_capella_fork_store(pre,message0,genesis,g.state,{}, {},rules,checkpoint);const auto parent=capella_fork_head(genesis_store,rules,checkpoint);const CapellaEpochRules epochs{r.consensus.epoch,r.execution,r.maximum_state_bytes};auto advanced=prepare_capella_slots(pre,epochs,l,first_slot,maximum,100000,checkpoint);auto identity=prepare_capella_proposer_identity(pre,parent->header,first_slot,epochs,l,r.consensus.attestation.duties,maximum,100000,checkpoint);check(bool(identity),"synthetic signed-anchor proposer");TestKey proposer(identity->index);check(proposer.public_key==identity->public_key,"synthetic proposer identity");ConsensusRoot epoch{};for(size_t i=0;i<8;++i)epoch[i]=uint8_t((first_slot/r.consensus.epoch.ffg.slots_per_epoch)>>(8*i));auto reveal=proposer.sign(consensus_signing_root(epoch,consensus_domain({2,0,0,0},identity->fork_version,domain)));
 Block execution;execution.header.parent_hash=genesis.header.hash();execution.header.number=1;execution.header.timestamp=genesis.header.timestamp+Int(std::to_string(first_slot*r.seconds_per_slot));execution.header.difficulty=0;execution.header.base_fee=next_base_fee(genesis.header);const auto mix=test_root(advanced.ssz,random+32*((first_slot/r.consensus.epoch.ffg.slots_per_epoch)%l.epochs_per_historical_vector));execution.header.mix_hash=Bytes(mix.begin(),mix.end());auto history=[&](const Int& n){check(n==0,"synthetic ancestor only");return genesis.header.hash();};State state=g.state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(state,execution,false);
 const auto parent_root=consensus_beacon_header_root(parent->header);auto body=test_body(advanced,r,reveal,test_payload(execution));auto candidate=prepare_capella_unsigned_block(pre,first_slot,identity->index,parent_root,body,r,genesis.header,g.state,history,maximum,100000,checkpoint);check(bool(candidate),"genuine synthetic unsigned bootstrap");const auto& message=candidate->message_ssz;ConsensusBeaconHeader header{test_little(message,0),test_little(message,8),test_root(message,16),test_root(message,48),capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint)};
 auto signature=proposer.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},identity->fork_version,domain)));Bytes wire(100);test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());const auto authenticated=prepare_capella_state_transition(pre,wire,r,genesis.header,g.state,history,maximum,100000,checkpoint);check(bool(authenticated),"full signed fixture transition authenticated");auto initial=initialize_capella_fork_store(authenticated->consensus,message,authenticated->execution.block,authenticated->execution.state,authenticated->execution.receipts,{genesis.header},rules,checkpoint);check(initial.blocks.size()==1,"genuine post-transition trusted singleton anchor");
 check(r.consensus.epoch.ffg.slots_per_epoch==8,"known public minimal epoch fixture");const auto max=r.maximum_state_bytes;CapellaStoreSnapshotLimits limits{16*1024*1024,128,256,4,max,32*max,64,100000};return {std::move(g),std::move(rules),limits,std::move(pre),std::move(initial),std::move(wire),std::move(genesis_document)};
}

} // namespace nano::test::isolated_capture
