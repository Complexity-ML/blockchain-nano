#pragma once
// New standalone canonical-stream fixture; selected public builder primitives.
// Domain2900001 and counter20000 are distinct from the old fixture; its source
// and1024/180s/128MiB checks are unchanged. No old historical test group/main.
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
#include "nano/consensus_capella_withdrawals.hpp"
#include "nano/consensus_capella_deposits.hpp"
#include "nano/committee.hpp"
#include "nano/consensus_ffg.hpp"
#include "blst.h"
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
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <utility>
#include <unistd.h>
namespace nano::test::canonical_stream_fixture {
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
void checkpoint(){check(std::chrono::steady_clock::now()-started<std::chrono::seconds(300),"canonical-stream fixture wall bound");rusage u{};check(::getrusage(RUSAGE_SELF,&u)==0,"canonical-stream fixture resource read");
#ifdef __APPLE__
 check(u.ru_maxrss/1024<=131072,"canonical-stream fixture RSS bound");
#else
 check(u.ru_maxrss<=131072,"canonical-stream fixture RSS bound");
#endif
}
std::string read(const fs::path& path){
 struct File {int value;~File(){if(value>=0)::close(value);}};
 check(fs::canonical(path)==path,"public fixture canonical path");
 File file{::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)};check(file.value>=0,"public fixture read");
 struct stat before{},after{},linked{};
 auto same=[](const struct stat& a,const struct stat& b){
#ifdef __APPLE__
  const auto am=a.st_mtimespec,bm=b.st_mtimespec,ac=a.st_ctimespec,bc=b.st_ctimespec;
#else
  const auto am=a.st_mtim,bm=b.st_mtim,ac=a.st_ctim,bc=b.st_ctim;
#endif
  return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_size==b.st_size&&am.tv_sec==bm.tv_sec&&am.tv_nsec==bm.tv_nsec&&ac.tv_sec==bc.tv_sec&&ac.tv_nsec==bc.tv_nsec;
 };
 check(::fstat(file.value,&before)==0&&S_ISREG(before.st_mode)&&before.st_nlink==1&&before.st_size>=0&&uint64_t(before.st_size)<=32*1024*1024,"public fixture input bound/type");
 std::string out;out.reserve(size_t(before.st_size));std::array<char,65536> buffer{};
 for(;;){checkpoint();const auto n=::read(file.value,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;check(n>=0,"public fixture read failure");if(!n)break;check(size_t(n)<=size_t(before.st_size)-out.size(),"public fixture grew");out.append(buffer.data(),size_t(n));}
 check(out.size()==size_t(before.st_size)&&::fstat(file.value,&after)==0&&::lstat(path.c_str(),&linked)==0&&same(before,after)&&same(after,linked),"public fixture FD/path changed");
 const auto descriptor=std::exchange(file.value,-1);check(::close(descriptor)==0,"public fixture close");return out;
}
Bytes bytes_of(const std::string& s){return {s.begin(),s.end()};}
struct Temp{fs::path path;Temp(){auto pattern=(fs::temp_directory_path()/"nano-canonical-stream-XXXXXX").string();check(::mkdtemp(pattern.data()),"owned temporary fixture");path=fs::canonical(pattern);}~Temp(){std::error_code e;fs::remove_all(path,e);}};
struct Fixture{Genesis genesis;CapellaForkBlockRules rules;CapellaStoreSnapshotLimits limits;CapellaSlotState pre;CapellaForkStore initial;Bytes wire;};
uint64_t test_signatures=0;
uint64_t test_little(std::span<const uint8_t> b,size_t at,size_t width=8){check(width<=8&&at<=b.size()&&width<=b.size()-at,"synthetic field bounds");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
void test_put(Bytes& b,size_t at,uint64_t n,size_t width=8){check(at<=b.size()&&width<=b.size()-at,"synthetic write bounds");for(size_t i=0;i<width;++i)b[at+i]=uint8_t(n>>(8*i));}
template<class A>void test_copy(A& a,const Bytes& b){check(a.size()==b.size(),"synthetic fixed width");std::copy(b.begin(),b.end(),a.begin());}
ConsensusRoot test_root(std::span<const uint8_t> b,size_t at=0){check(at<=b.size()&&32<=b.size()-at,"synthetic root bounds");ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
struct TestKey{
 blst_scalar scalar{};Bytes public_key;
 explicit TestKey(uint64_t index){check(index<=10063,"synthetic key range");uint64_t words[4]{2900001+index,0,0,0};blst_scalar_from_uint64(&scalar,words);OPENSSL_cleanse(words,sizeof(words));blst_p1 p;blst_sk_to_pk_in_g1(&p,&scalar);public_key.resize(48);blst_p1_compress(public_key.data(),&p);}
 TestKey(const TestKey&)=delete;TestKey& operator=(const TestKey&)=delete;
 ~TestKey(){OPENSSL_cleanse(&scalar,sizeof(scalar));}
 Bytes sign(const ConsensusRoot& root)const{checkpoint();check(test_signatures<20000,"bounded test-only synthetic signatures");++test_signatures;static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,p;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&p,&h,&scalar);Bytes s(96);blst_p2_compress(s.data(),&p);OPENSSL_cleanse(&h,sizeof(h));OPENSSL_cleanse(&p,sizeof(p));return s;}
};
ConsensusRoot test_leaf(std::span<const uint8_t> b){check(b.size()<=32,"SSZ leaf bound");ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
ConsensusRoot test_pair(const ConsensusRoot& a,const ConsensusRoot& b){Bytes p(a.begin(),a.end());p.insert(p.end(),b.begin(),b.end());return test_root(sha256(p));}
ConsensusRoot test_registry_root(const Bytes& data,uint64_t limit){
 check(data.size()==64*121&&limit>=64&&limit<=UINT64_MAX/2&&(limit&(limit-1))==0,"synthetic registry bounds");std::array<ConsensusRoot,64> roots{};
 for(size_t i=0;i<64;++i){auto v=std::span<const uint8_t>(data).subspan(i*121,121);std::array<ConsensusRoot,8> f{test_pair(test_leaf(v.first(32)),test_leaf(v.subspan(32,16))),test_leaf(v.subspan(48,32)),test_leaf(v.subspan(80,8)),test_leaf(v.subspan(88,1)),test_leaf(v.subspan(89,8)),test_leaf(v.subspan(97,8)),test_leaf(v.subspan(105,8)),test_leaf(v.subspan(113,8))};for(size_t n=8;n>1;n/=2)for(size_t j=0;j<n/2;++j)f[j]=test_pair(f[j*2],f[j*2+1]);roots[i]=f[0];}
 for(size_t n=64;n>1;n/=2)for(size_t j=0;j<n/2;++j)roots[j]=test_pair(roots[j*2],roots[j*2+1]);ConsensusRoot zero{},r=roots[0];for(unsigned i=0;i<6;++i)zero=test_pair(zero,zero);for(uint64_t n=64;n<limit;n*=2){r=test_pair(r,zero);zero=test_pair(zero,zero);}ConsensusRoot length{};length[0]=64;return test_pair(r,length);
}
CapellaExecutionPayload test_payload(const Block& b){const auto& h=b.header;CapellaExecutionPayload p;test_copy(p.parent_hash,h.parent_hash);test_copy(p.fee_recipient,unhex(h.coinbase));test_copy(p.state_root,h.state_root);test_copy(p.receipts_root,h.receipts_root);test_copy(p.logs_bloom,h.logs_bloom);test_copy(p.prev_randao,h.mix_hash);test_copy(p.block_hash,h.hash());check(h.base_fee.has_value(),"synthetic base fee");auto fee=bytes(*h.base_fee,32);std::reverse(fee.begin(),fee.end());test_copy(p.base_fee_per_gas,fee);p.block_number=u64(h.number);p.gas_limit=u64(h.gas_limit);p.gas_used=u64(h.gas_used);p.timestamp=u64(h.timestamp);p.extra_data=h.extra_data;for(const auto& tx:b.transactions)p.transactions.push_back(tx.raw());return p;}
Bytes test_body(const CapellaSlotState& state,const CapellaTransitionRules& r,const Bytes& reveal,const CapellaExecutionPayload& p){const auto& l=r.consensus.state;const size_t hist=176+64*l.slots_per_historical_root,sync=(l.sync_committee_size+7)/8+96,fixed=228+sync;auto payload=encode_capella_execution_payload(p,r.execution,r.withdrawals.maximum_per_payload,r.consensus.maximum_block_bytes);Bytes b(fixed,0);check(reveal.size()==96,"synthetic RANDAO width");std::copy(reveal.begin(),reveal.end(),b.begin());std::copy_n(state.ssz.begin()+hist+4,72,b.begin()+96);b[220+(l.sync_committee_size+7)/8]=0xc0;for(size_t at:{size_t(200),size_t(204),size_t(208),size_t(212),size_t(216),220+sync})test_put(b,at,fixed,4);test_put(b,224+sync,fixed+payload.size(),4);b.insert(b.end(),payload.begin(),payload.end());return b;}
std::string test_topup_sender(){
 const auto secret=bytes(Int(1),32);check(secret==unhex("0000000000000000000000000000000000000000000000000000000000000001"),"fixed public topup scalar bytes");
 const auto sender=public_address(public_key(secret));check(address(sender)=="0x7e5f4552091a69125d5dfcb7b8c2659029395bdf","fixed public topup derived address");return sender;
}
Fixture fixture(uint64_t first_slot=1,bool prefund_native_topups=false){
 check(first_slot==1||first_slot==512,"two fixed public fixture anchor slots");const uint64_t maximum=std::max<uint64_t>(64,first_slot);
 const auto root=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/capella-bootstrap-anchor-cb532";const auto text=read(root/"manifest.json");check(hex(sha256(bytes_of(text)))=="0xec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6","public input literal pin");const auto manifest=Json::parse(text);auto input=[&](const char* kind,const char* name){const auto& pin=manifest.at("files").at(kind);auto b=read(root/name);check(pin.at("path")==name&&pin.at("bytes").get<size_t>()==b.size()&&pin.at("sha256").get<std::string>()==hex(sha256(bytes_of(b))).substr(2),"public input bytes pin");return b;};
 const auto profile=input("profile","profile.json");auto rules=load_capella_fork_profile(profile,sha256(bytes_of(profile)),1024*1024);auto pre=CapellaSlotState{bytes_of(input("genesisState","genesis-state.ssz")),std::nullopt};const auto& r=rules.transition;const auto& l=r.consensus.state;
 const size_t hist=176+64*l.slots_per_historical_root,random=hist+96,previous=random+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector,field=previous+229+96*l.sync_committee_size;const size_t validators=test_little(pre.ssz,hist+88,4),balances=test_little(pre.ssz,hist+92,4);check((balances-validators)/121==64,"synthetic registry size");
 std::set<Bytes> excluded,generated;for(const auto* kind:{"publicKeys","withdrawalPublicKeys"})for(const auto& row:manifest.at(kind))excluded.insert(unhex(row.at("publicKey").get<std::string>()));const auto official_text=read(fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-capella-slots-official/slots_1/pre.ssz");check(hex(sha256(bytes_of(official_text)))=="0xf5a464175565a8ed6a74790a60b84477c16a5b1bf71ece55736f192142cbca3d","literal official/V37 public registry bytes");const auto official=bytes_of(official_text);const size_t legacy_start=test_little(official,hist+88,4),legacy_end=test_little(official,hist+92,4);check(legacy_end>=legacy_start&&(legacy_end-legacy_start)/121==64&&legacy_end<=official.size(),"official public registry bounds");for(size_t i=0;i<64;++i)excluded.emplace(official.begin()+legacy_start+121*i,official.begin()+legacy_start+121*i+48);std::map<Bytes,Bytes> remap;
 for(size_t i=0;i<64;++i){checkpoint();const size_t at=validators+121*i;Bytes old(pre.ssz.begin()+at,pre.ssz.begin()+at+48);TestKey key(i),withdrawal(10000+i);check(!excluded.contains(key.public_key)&&!excluded.contains(withdrawal.public_key)&&generated.insert(key.public_key).second&&generated.insert(withdrawal.public_key).second,"new test keys must be disjoint from public campaign roles");remap.emplace(old,key.public_key);std::copy(key.public_key.begin(),key.public_key.end(),pre.ssz.begin()+at);auto credential=sha256(withdrawal.public_key);credential[0]=0;std::copy(credential.begin(),credential.end(),pre.ssz.begin()+at+48);test_put(pre.ssz,at+80,32000000000);pre.ssz[at+88]=0;test_put(pre.ssz,at+89,0);test_put(pre.ssz,at+97,0);test_put(pre.ssz,at+105,UINT64_MAX);test_put(pre.ssz,at+113,UINT64_MAX);test_put(pre.ssz,balances+8*i,32000000000);}
 const auto domain=test_registry_root(Bytes(pre.ssz.begin()+validators,pre.ssz.begin()+balances),l.validator_registry_limit);check(domain!=test_root(pre.ssz,8)&&hex(Bytes(domain.begin(),domain.end()))!="0x0a08c27fe4ece2483f9e581f78c66379a06f96e9c24cd1390594ff939b26f95b","new domain must not equal cb532/official/V37");std::copy(domain.begin(),domain.end(),pre.ssz.begin()+8);
 for(size_t start:std::array<size_t,2>{previous+133,previous+133+48*size_t(l.sync_committee_size+1)}){std::vector<Bytes> members;for(size_t i=0;i<l.sync_committee_size;++i){const size_t at=start+48*i;Bytes old(pre.ssz.begin()+at,pre.ssz.begin()+at+48);members.push_back(remap.at(old));std::copy(members.back().begin(),members.back().end(),pre.ssz.begin()+at);}auto aggregate=aggregate_consensus_bls_keys(members);check(bool(aggregate),"synthetic sync aggregate");std::copy(aggregate->begin(),aggregate->end(),pre.ssz.begin()+start+48*l.sync_committee_size);}
 Genesis g=Genesis::from_json(Json::parse(input("genesis","genesis.json")));g.chain_id=r.chain_id;g.consensus="beacon";g.evm_revision=EvmRevision::Shanghai;
 // Opt-in UNIT fixture only: transfer the existing public34ETH allocation to
 // the public ECDSA secret1 account BEFORE any execution/Beacon anchor binding.
 // The default D0 corpus never enters this branch or changes its input bytes.
 if(prefund_native_topups){
  constexpr const char* old_account="0xec2f2e15c4e9f2a538886bf4ccc91f77db196662";
  const auto public_account=test_topup_sender();
  auto& source=g.state.get(old_account);auto& target=g.state.get(public_account);
  const Int expected("34000000000000000000");check(source.balance==expected&&target.balance==0&&target.nonce==0&&target.code.empty(),"fixed public topup allocation");
  const Int total=source.balance+target.balance;target.balance+=source.balance;source.balance=0;check(source.balance+target.balance==total&&target.balance==expected,"topup fixture preserves total allocation");
 }
 auto& genesis=g.block;genesis.header.difficulty=0;genesis.header.timestamp=Int(std::to_string(test_little(pre.ssz,0)));genesis.header.state_root=g.state.root();genesis.header.transactions_root=transactions_root({});genesis.header.receipts_root=receipts_root({});genesis.header.withdrawals_root=withdrawals_root(Json::array());const std::string tag="Nano/Test/CanonicalStreaming/v1";genesis.header.extra_data=bytes_of(tag);
 const ExecutionDepositHistory deposits(genesis.header.hash());const auto dep=deposits.root();std::copy(dep.begin(),dep.end(),pre.ssz.begin()+hist+4);test_put(pre.ssz,hist+36,0);const auto gh=genesis.header.hash();std::copy(gh.begin(),gh.end(),pre.ssz.begin()+hist+44);test_put(pre.ssz,hist+80,0);
 auto execution_header=encode_capella_execution_header(capella_execution_payload_header(test_payload(genesis),r.execution,r.withdrawals.maximum_per_payload),r.execution.extra_data_bytes,r.maximum_state_bytes);pre.ssz.resize(test_little(pre.ssz,field,4));pre.ssz.insert(pre.ssz.end(),execution_header.begin(),execution_header.end());test_put(pre.ssz,field+4,0);test_put(pre.ssz,field+12,0);test_put(pre.ssz,field+20,pre.ssz.size(),4);std::fill(pre.ssz.begin()+64,pre.ssz.begin()+176,0);
 auto zero_body=test_body(pre,r,Bytes(96),test_payload(genesis));const auto body_root=capella_beacon_body_root(zero_body,r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint);std::copy(body_root.begin(),body_root.end(),pre.ssz.begin()+144);const auto pre_root=capella_beacon_state_root(pre.ssz,l,r.execution,r.maximum_state_bytes,checkpoint);Bytes message0(84);std::copy(pre_root.begin(),pre_root.end(),message0.begin()+48);test_put(message0,80,84,4);message0.insert(message0.end(),zero_body.begin(),zero_body.end());
 auto genesis_store=initialize_capella_fork_store(pre,message0,genesis,g.state,{}, {},rules,checkpoint);const auto parent=capella_fork_head(genesis_store,rules,checkpoint);const CapellaEpochRules epochs{r.consensus.epoch,r.execution,r.maximum_state_bytes};auto advanced=prepare_capella_slots(pre,epochs,l,first_slot,maximum,100000,checkpoint);auto identity=prepare_capella_proposer_identity(pre,parent->header,first_slot,epochs,l,r.consensus.attestation.duties,maximum,100000,checkpoint);check(bool(identity),"synthetic signed-anchor proposer");TestKey proposer(identity->index);check(proposer.public_key==identity->public_key,"synthetic proposer identity");ConsensusRoot epoch{};for(size_t i=0;i<8;++i)epoch[i]=uint8_t((first_slot/r.consensus.epoch.ffg.slots_per_epoch)>>(8*i));auto reveal=proposer.sign(consensus_signing_root(epoch,consensus_domain({2,0,0,0},identity->fork_version,domain)));
 Block execution;execution.header.parent_hash=genesis.header.hash();execution.header.number=1;execution.header.timestamp=genesis.header.timestamp+Int(std::to_string(first_slot*r.seconds_per_slot));execution.header.difficulty=0;execution.header.base_fee=next_base_fee(genesis.header);const auto mix=test_root(advanced.ssz,random+32*((first_slot/r.consensus.epoch.ffg.slots_per_epoch)%l.epochs_per_historical_vector));execution.header.mix_hash=Bytes(mix.begin(),mix.end());auto history=[&](const Int& n){check(n==0,"synthetic ancestor only");return genesis.header.hash();};State state=g.state;ExecutionEngine(r.chain_id,history,EvmRevision::Shanghai,r.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(state,execution,false);
 const auto parent_root=consensus_beacon_header_root(parent->header);auto body=test_body(advanced,r,reveal,test_payload(execution));auto candidate=prepare_capella_unsigned_block(pre,first_slot,identity->index,parent_root,body,r,genesis.header,g.state,history,maximum,100000,checkpoint);check(bool(candidate),"genuine synthetic unsigned bootstrap");const auto& message=candidate->message_ssz;ConsensusBeaconHeader header{test_little(message,0),test_little(message,8),test_root(message,16),test_root(message,48),capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),r.consensus.body,l.sync_committee_size,r.execution,r.withdrawals.maximum_per_payload,r.maximum_bls_changes,r.consensus.maximum_block_bytes,checkpoint)};
 auto signature=proposer.sign(consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},identity->fork_version,domain)));Bytes wire(100);test_put(wire,0,100,4);std::copy(signature.begin(),signature.end(),wire.begin()+4);wire.insert(wire.end(),message.begin(),message.end());const auto authenticated=prepare_capella_state_transition(pre,wire,r,genesis.header,g.state,history,maximum,100000,checkpoint);check(bool(authenticated),"full signed fixture transition authenticated");auto initial=initialize_capella_fork_store(authenticated->consensus,message,authenticated->execution.block,authenticated->execution.state,authenticated->execution.receipts,{genesis.header},rules,checkpoint);check(initial.blocks.size()==1,"genuine post-transition trusted singleton anchor");
 check(r.consensus.epoch.ffg.slots_per_epoch==8,"known public minimal epoch fixture");const auto max=r.maximum_state_bytes;CapellaStoreSnapshotLimits limits{16*1024*1024,256,256,4,max,32*max,64,100000};return {std::move(g),std::move(rules),limits,std::move(pre),std::move(initial),std::move(wire)};
}

} // namespace nano::test::canonical_stream_fixture

namespace nano::test::canonical_stream_fixture {
struct LongFixture {
 Fixture f;std::map<ConsensusRoot,CheckpointArchiveEntry> archives;std::optional<CheckpointArchiveEntry> wrong_branch;
 std::map<ConsensusRoot,std::pair<Block,Bytes>> execution;
 // Only populated for the explicit nonzero-deposit regression. Legacy D0
 // generation retains neither extra histories nor consensus-state copies.
 std::map<ConsensusRoot,ExecutionDepositHistory> native_histories;
 std::map<ConsensusRoot,Bytes> native_states;
 ConsensusBeaconHeader head;ConsensusCheckpoint finalized;
 size_t peak_generator_records=0,peak_generator_state_bytes=0;
 uint64_t generator_prunes=0,archive_bytes=0;
 explicit LongFixture(uint64_t count,bool attest=true,uint64_t native_topups=0):f(fixture(1,native_topups!=0)){
  check(count>0&&count<=1026&&native_topups<=16&&native_topups<=count,"new streaming fixture block bound");auto store=f.initial;auto tip=store.timed.view.anchor;
  const auto& t=f.rules.transition;const auto& l=t.consensus.state;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
  const CapellaEpochRules epochs{t.consensus.epoch,t.execution,t.maximum_state_bytes};
  auto remember=[&](const ConsensusRoot& id,const CapellaForkBlock& b){execution.emplace(id,std::make_pair(b.execution_block,b.execution_state.root()));if(native_topups){check(bool(b.deposit_history),"nonzero fixture authenticated history");native_histories.emplace(id,*b.deposit_history);native_states.emplace(id,b.state.ssz);}};
  remember(tip,*store.blocks.at(tip).lease(checkpoint));std::vector<Bytes> pending;
  Phase0Deposit topup{};Bytes topup_calldata;std::string topup_sender;
  if(native_topups){
   topup_sender=test_topup_sender();const auto& funded=f.genesis.state.get(topup_sender);check(funded.balance==Int("34000000000000000000")&&funded.nonce==0&&funded.code.empty(),"derived public topup sender was funded before anchor binding");
   const size_t historical=176+64*l.slots_per_historical_root,validators=test_little(f.pre.ssz,historical+88,4);TestKey existing(0);
   test_copy(topup.message.public_key,existing.public_key);std::copy_n(f.pre.ssz.begin()+validators+48,32,topup.message.withdrawal_credentials.begin());topup.message.amount_gwei=1000000000;
   test_copy(topup.signature,existing.sign(consensus_signing_root(consensus_deposit_root(topup.message),consensus_domain({3,0,0,0},t.consensus.deposit.genesis_fork_version,{}))));
   check(verify_consensus_deposit_signature(topup.message,topup.signature,t.consensus.deposit.genesis_fork_version),"public existing-key topup signature");
   const std::string selector="deposit(bytes,bytes,bytes,bytes32)";topup_calldata=keccak(Bytes(selector.begin(),selector.end()));topup_calldata.resize(4);Bytes args(416);
   auto word=[&](size_t at,size_t value){const auto v=bytes(Int(value),32);std::copy(v.begin(),v.end(),args.begin()+at);};word(0,128);word(32,224);word(64,288);const auto data_root=consensus_deposit_data_root(topup);std::copy(data_root.begin(),data_root.end(),args.begin()+96);
   word(128,48);std::copy(topup.message.public_key.begin(),topup.message.public_key.end(),args.begin()+160);word(224,32);std::copy(topup.message.withdrawal_credentials.begin(),topup.message.withdrawal_credentials.end(),args.begin()+256);word(288,96);std::copy(topup.signature.begin(),topup.signature.end(),args.begin()+320);topup_calldata.insert(topup_calldata.end(),args.begin(),args.end());
  }
  auto votes=[&](const CapellaForkBlock& b){
   std::vector<Bytes> out;if(!attest)return out;const auto slot=b.header.slot,epoch=slot/slots;const auto ctx=capella_fork_block_context(b.state,b.header,epochs,l,checkpoint);const auto cp=capella_fork_checkpoints(b.state,epochs,l,checkpoint);
   const auto hist=176+64*l.slots_per_historical_root,randao=hist+96;ConsensusRandaoState random;random.epoch=epoch;random.last_reveal_slot=slot;for(size_t i=0;i<l.epochs_per_historical_vector;++i)random.mixes.push_back(test_root(b.state.ssz,randao+32*i));
   const auto assignment=phase0_attestation_duties(ctx.registry,random,t.consensus.attestation.duties,epoch);const auto target=slot%slots==0?consensus_beacon_header_root(b.header):test_root(b.state.ssz,176+32*((epoch*slots)%l.slots_per_historical_root));const auto version=epoch<ctx.fork.epoch?ctx.fork.previous_version:ctx.fork.current_version;
   for(uint64_t k=0;k<assignment.committees_per_slot;++k){const auto members=partition_committee(assignment.shuffled,(slot%slots)*assignment.committees_per_slot+k,assignment.committees_per_slot*slots);if(members.empty())continue;Phase0SignedAttestation vote{{slot,k,consensus_beacon_header_root(b.header),cp.realized.current_justified,{epoch,target}},std::vector<bool>(members.size(),true),{}};std::vector<Bytes> signatures;for(const auto index:members)signatures.push_back(TestKey(index).sign(consensus_signing_root(consensus_attestation_root(vote.data),consensus_domain({1,0,0,0},version,ctx.genesis_root))));auto aggregate=aggregate_consensus_bls_signatures(signatures);check(bool(aggregate),"real streaming fixture attestation aggregation");vote.signature=*aggregate;out.push_back(encode_consensus_attestation_ssz(vote,t.consensus.body.validators_per_committee,checkpoint));}return out;
  };
  pending=votes(*store.blocks.at(tip).lease(checkpoint));
  for(uint64_t i=0;i<count;++i){
   checkpoint();
   {const auto& v=store.timed.view;const auto lookahead=t.consensus.attestation.duties.min_seed_lookahead;const auto at=v.finalized.root;if(at!=v.anchor&&lookahead<v.current_epoch&&store.blocks.at(at).header().slot<=(v.current_epoch-lookahead-1)*slots){store=prepare_capella_pruned_store(store,at,f.rules,checkpoint);++generator_prunes;}}
   size_t live=0;for(const auto& [_,b]:store.blocks)live+=b.lease(checkpoint)->state.ssz.size();check(store.blocks.size()<256&&live+t.maximum_state_bytes<=f.limits.total_state_bytes,"generator refuses unfinalized full window");peak_generator_records=std::max(peak_generator_records,store.blocks.size());peak_generator_state_bytes=std::max(peak_generator_state_bytes,live);
   Bytes wire;ConsensusRoot id{};uint64_t slot=0;
   {
    const auto parent=store.blocks.at(tip).lease(checkpoint);slot=parent->header.slot+1;test_slot=slot;auto advanced=prepare_capella_slots(parent->state,epochs,l,slot,f.limits.replay_slots,f.limits.sync_trials,checkpoint);auto proposer=prepare_capella_proposer_identity(parent->state,parent->header,slot,epochs,l,t.consensus.attestation.duties,f.limits.replay_slots,f.limits.sync_trials,checkpoint);check(bool(proposer),"real streaming proposer");TestKey key(proposer->index);check(key.public_key==proposer->public_key,"streaming key matches authenticated proposer");ConsensusRoot epoch{};for(size_t j=0;j<8;++j)epoch[j]=uint8_t((slot/slots)>>(8*j));const auto reveal=key.sign(consensus_signing_root(epoch,consensus_domain({2,0,0,0},proposer->fork_version,proposer->genesis_root)));
    const auto hist=176+64*l.slots_per_historical_root,randao=hist+96;Block execution;execution.header.parent_hash=parent->execution_block.header.hash();execution.header.number=parent->execution_block.header.number+1;execution.header.timestamp=parent->execution_block.header.timestamp+Int(std::to_string(t.seconds_per_slot));execution.header.gas_limit=parent->execution_block.header.gas_limit;execution.header.coinbase=parent->execution_block.header.coinbase;execution.header.base_fee=next_base_fee(parent->execution_block.header);execution.header.difficulty=0;const auto mix=test_root(advanced.ssz,randao+32*((slot/slots)%l.epochs_per_historical_vector));execution.header.mix_hash=Bytes(mix.begin(),mix.end());
    const auto state_root=capella_beacon_state_root(advanced.ssz,l,t.execution,t.maximum_state_bytes,checkpoint);const auto withdrawals=expected_capella_withdrawals(read_capella_withdrawal_state(advanced.ssz,state_root,l,t.execution,t.maximum_state_bytes,slots,checkpoint),t.withdrawals,checkpoint);execution.withdrawals=Json::array();for(const auto& w:withdrawals)execution.withdrawals.push_back(w.to_json());auto history=[&](const Int& h){return capella_execution_history(store,tip,h,checkpoint);};
    if(i<native_topups){Transaction tx;tx.chain_id=t.chain_id;tx.to=ethereum_deposit_contract;tx.gas=1000000;tx.gas_price=10;tx.nonce=Int(std::to_string(i));tx.value=Int("1000000000000000000");tx.data=topup_calldata;auto signed_tx=SignedTransaction::sign(tx,bytes(Int(1),32));check(signed_tx.sender()==topup_sender,"public fixed topup sender");execution.transactions.push_back(std::move(signed_tx));}
    auto state=parent->execution_state;ExecutionEngine(t.chain_id,history,EvmRevision::Shanghai,t.replay_policy,ConsensusInputPolicy::EthereumValidated).execute_block(state,execution,false);
    Bytes mandatory;auto body=test_body(advanced,t,reveal,test_payload(execution));if(f.rules.native_deposit_votes){const auto vote=capella_native_deposit_vote(store,tip,slot,f.rules,checkpoint);check(vote.size()==72,"native streaming fixture vote");std::copy(vote.begin(),vote.end(),body.begin()+96);mandatory=capella_native_deposit_operations(store,tip,advanced.ssz,vote,f.rules,checkpoint);if(!native_topups)check(mandatory.empty(),"empty native fixture produces no invented deposits");}
    if(!pending.empty()||!mandatory.empty()){const size_t sync=(l.sync_committee_size+7)/8+96,fixed=228+sync;auto payload=encode_capella_execution_payload(test_payload(execution),t.execution,t.withdrawals.maximum_per_payload,t.consensus.maximum_block_bytes);Bytes list(4*pending.size());for(size_t j=0;j<pending.size();++j){test_put(list,j*4,list.size(),4);list.insert(list.end(),pending[j].begin(),pending[j].end());}body.resize(fixed);test_put(body,212,fixed+list.size(),4);for(size_t at:{size_t(216),220+sync})test_put(body,at,fixed+list.size()+mandatory.size(),4);test_put(body,224+sync,fixed+list.size()+mandatory.size()+payload.size(),4);body.insert(body.end(),list.begin(),list.end());body.insert(body.end(),mandatory.begin(),mandatory.end());body.insert(body.end(),payload.begin(),payload.end());}
    auto sign_body=[&](const Bytes& value){const auto candidate=prepare_capella_unsigned_block(parent->state,slot,proposer->index,tip,value,t,parent->execution_block.header,parent->execution_state,history,f.limits.replay_slots,f.limits.sync_trials,checkpoint);check(bool(candidate),"real streaming unsigned block");const auto& message=candidate->message_ssz;ConsensusBeaconHeader h{test_little(message,0),test_little(message,8),test_root(message,16),test_root(message,48),capella_beacon_body_root(std::span<const uint8_t>(message).subspan(84),t.consensus.body,l.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,checkpoint)};const auto identity=consensus_beacon_header_root(h);const auto signature=key.sign(consensus_signing_root(identity,consensus_domain({0,0,0,0},proposer->fork_version,proposer->genesis_root)));Bytes signed_wire(100);test_put(signed_wire,0,100,4);std::copy(signature.begin(),signature.end(),signed_wire.begin()+4);signed_wire.insert(signed_wire.end(),message.begin(),message.end());return CheckpointArchiveEntry{Bytes(identity.begin(),identity.end()),slot,std::move(signed_wire)};};
    auto main=sign_body(body);id=test_root(main.root);wire=std::move(main.payload);if(i==0&&attest){auto alternate=body;alternate[168]^=1;wrong_branch=sign_body(alternate);check(wrong_branch->root!=main.root,"genuine sibling must differ from canonical branch");}
   }
   store=prepare_capella_resumed_store_tick(store,store.timed.clock.genesis_time_ms+slot*f.rules.slot_duration_ms,f.rules,f.limits.replay_slots,f.limits.sync_trials,checkpoint);if(i==0&&wrong_branch){auto alternate=prepare_capella_resumed_fork_block(store,wrong_branch->payload,f.rules,f.limits.replay_slots,f.limits.sync_trials,checkpoint);check(alternate&&alternate->blocks.contains(test_root(wrong_branch->root)),"wrong-branch fixture is itself a genuinely valid signed sibling");}auto next=prepare_capella_resumed_fork_block(store,wire,f.rules,f.limits.replay_slots,f.limits.sync_trials,checkpoint);check(next&&next->blocks.contains(id),"genuine full signed streaming fixture admission");store=std::move(*next);tip=id;
   {const auto post=store.blocks.at(tip).lease(checkpoint);head=post->header;remember(tip,*post);pending=votes(*post);}
   size_t after_bytes=0;for(const auto& [_,b]:store.blocks)after_bytes+=b.lease(checkpoint)->state.ssz.size();check(store.blocks.size()<=256&&after_bytes<=f.limits.total_state_bytes,"generator live window remains bounded after admission");peak_generator_records=std::max(peak_generator_records,store.blocks.size());peak_generator_state_bytes=std::max(peak_generator_state_bytes,after_bytes);
   check(wire.size()<=16*1024*1024-archive_bytes,"new fixture raw wire byte bound");archive_bytes+=wire.size();archives.emplace(tip,CheckpointArchiveEntry{Bytes(tip.begin(),tip.end()),slot,std::move(wire)});
  }
  check(!wrong_branch||wrong_branch->payload.size()<=16*1024*1024-archive_bytes,"canonical plus sibling source wire bound");finalized=store.timed.view.finalized;if(attest)check(finalized.epoch>0&&generator_prunes>0&&capella_fork_head(store,f.rules,checkpoint)->header==head,"real signed FFG progression, selected HEAD and generator pruning required");else check(finalized==f.initial.timed.view.finalized&&generator_prunes==0,"stalled signed branch must not fabricate finality or pruning");
 }
};
}
