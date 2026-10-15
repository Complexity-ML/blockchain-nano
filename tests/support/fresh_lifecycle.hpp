#pragma once
// Explicit fresh synthetic actions and stopped RO observations. No runtime,
// Reactor, BeaconSigningJournal, live-state fallback or inferred secret/index.
#include "nano/checkpoint_blob_gc.hpp"
#include "nano/chain.hpp"
#include "nano/consensus_message_ssz.hpp"
#include "nano/consensus_capella_withdrawals.hpp"
#include "capella_canonical_stream.hpp"
#include "../../src/beacon_block_json.hpp"
#include "blst.h"
#include <openssl/crypto.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <set>
#include <sys/stat.h>
#include <unistd.h>

namespace nano::test::fresh_lifecycle {
namespace fs=std::filesystem;
inline const char* current_stage="dispatch";
inline void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
inline std::string q(uint64_t n){return quantity(Int(std::to_string(n)));}
inline void shape(const Json& j,std::initializer_list<const char*> fields){
 require(j.is_object()&&j.size()==fields.size(),"document shape");for(auto f:fields)require(j.contains(f),"missing field");
}
inline uint64_t number(const Json& j){require(j.is_string(),"quantity type");const auto n=u64(quantity(j));require(q(n)==j.get<std::string>(),"canonical quantity");return n;}
inline uint64_t decimal(const Json& j){require(j.is_string(),"decimal type");const auto raw=j.get<std::string>();require(!raw.empty()&&raw.size()<=20&&std::all_of(raw.begin(),raw.end(),[](char c){return c>='0'&&c<='9';}),"decimal encoding");const auto n=u64(quantity(j));require(std::to_string(n)==raw,"canonical decimal");return n;}
inline std::string sha(std::string_view s){return hex(sha256(Bytes(s.begin(),s.end()))).substr(2);}
inline void digest(const Json& j){const auto s=j.get<std::string>();require(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"digest shape");}
inline Bytes bytes(const Json& j,size_t n){const auto s=j.get<std::string>();require(s.size()==2+2*n,"encoded width");auto b=unhex(s);require(b.size()==n&&hex(b)==s,"canonical bytes");return b;}
inline ConsensusRoot root(const Json& j){const auto b=bytes(j,32);ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
inline std::string text(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
inline ConsensusCheckpoint checkpoint(const Json& j){shape(j,{"epoch","root"});return {number(j.at("epoch")),root(j.at("root"))};}
inline ForkCheckpointToken token(const Json& j){shape(j,{"sequence","digest"});const auto n=number(j.at("sequence"));require(n>0,"zero token");return {n,bytes(j.at("digest"),32)};}
inline bool same(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const bool times=a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec&&a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 const bool times=a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 return times&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_nlink==b.st_nlink&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_size==b.st_size;
}
inline void path_guard(const fs::path& p){require(p.is_absolute()&&p==p.lexically_normal(),"canonical path");fs::path at;for(const auto& leaf:p){at/=leaf;struct stat st{};require(::lstat(at.c_str(),&st)==0&&!S_ISLNK(st.st_mode),"path type");}}
inline bool inside(const fs::path& a,const fs::path& b){const auto rel=b.lexically_relative(a);return !rel.empty()&&!rel.is_absolute()&&*rel.begin()!="..";}
struct Fd {int fd=-1;explicit Fd(int n):fd(n){require(fd>=0,"open");}~Fd(){if(fd>=0)::close(fd);}Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;};
struct Input {fs::path path;struct stat material;std::string content;};
struct Output {fs::path path;dev_t device;ino_t inode;gid_t group;std::string content;};
class Inputs {
 std::vector<Input> held_;
 std::vector<Output> outputs_;
public:
 std::function<void()> check;
 explicit Inputs(std::function<void()> fn):check(std::move(fn)){}
 std::string read(const fs::path& p,size_t max){
  check();path_guard(p);Fd f(::open(p.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK));struct stat a{},b{},c{};
  require(::fstat(f.fd,&a)==0&&S_ISREG(a.st_mode)&&a.st_uid==::geteuid()&&a.st_nlink==1&&a.st_size>0&&uint64_t(a.st_size)<=max,"input type/owner/size");
  std::string out(size_t(a.st_size),'\0');size_t at=0;while(at<out.size()){check();const auto n=::read(f.fd,out.data()+at,out.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"input truncated");at+=size_t(n);}char extra;
  require(::read(f.fd,&extra,1)==0&&::fstat(f.fd,&b)==0&&::lstat(p.c_str(),&c)==0&&same(a,b)&&same(a,c),"input changed");
  const auto old=std::find_if(held_.begin(),held_.end(),[&](const auto& row){return row.path==p;});
  if(old!=held_.end())require(same(old->material,a)&&old->content==out,"admitted input changed");else held_.push_back({p,a,out});return out;
 }
 void output(const fs::path& p,const struct stat& s,std::string_view raw){outputs_.push_back({p,s.st_dev,s.st_ino,s.st_gid,std::string(raw)});}
 void fence(){for(size_t i=0,n=held_.size();i<n;++i)(void)read(held_[i].path,held_[i].content.size());for(const auto& row:outputs_){path_guard(row.path);Fd f(::open(row.path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));struct stat a{},b{},c{};require(::fstat(f.fd,&a)==0&&S_ISREG(a.st_mode)&&a.st_uid==::geteuid()&&a.st_nlink==1&&(a.st_mode&07777)==0600&&a.st_dev==row.device&&a.st_ino==row.inode&&a.st_gid==row.group&&a.st_size>=0&&uint64_t(a.st_size)==row.content.size(),"new output identity");std::string raw(row.content.size(),'\0');size_t at=0;while(at<raw.size()){check();const auto n=::read(f.fd,raw.data()+at,raw.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"new output truncated");at+=size_t(n);}char extra;require(::read(f.fd,&extra,1)==0&&raw==row.content&&::fstat(f.fd,&b)==0&&::lstat(row.path.c_str(),&c)==0&&same(a,b)&&same(a,c),"new output bytes changed");}}
};
inline Json parse(std::string_view s){std::vector<std::set<std::string>> keys;return Json::parse(s.begin(),s.end(),[&](int depth,Json::parse_event_t event,Json& value){require(depth<=64,"JSON depth");if(event==Json::parse_event_t::object_start)keys.emplace_back();else if(event==Json::parse_event_t::object_end)keys.pop_back();else if(event==Json::parse_event_t::key)require(keys.back().insert(value.get<std::string>()).second,"duplicate field");return true;});}
inline std::string bound_file(Inputs& in,const fs::path& base,const Json& d,const char* leaf,size_t max){shape(d,{"path","sha256","bytes"});digest(d.at("sha256"));require(d.at("path")==leaf&&d.at("bytes").is_number_unsigned()&&d.at("bytes").get<uint64_t>()<=max,"file descriptor");auto raw=in.read(base/leaf,max);require(raw.size()==d.at("bytes").get<uint64_t>()&&sha(raw)==d.at("sha256").get<std::string>(),"file commitment");return raw;}
inline Json artifact(Inputs& in,const Json& d,size_t max){shape(d,{"path","sha256","bytes"});digest(d.at("sha256"));require(d.at("bytes").is_number_unsigned()&&d.at("bytes").get<uint64_t>()<=max,"artifact bound");auto raw=in.read(d.at("path").get<std::string>(),max);require(raw.size()==d.at("bytes").get<uint64_t>()&&sha(raw)==d.at("sha256").get<std::string>(),"artifact commitment");return parse(raw);}
struct Identity {std::string owner,environment;Bytes public_key;};
inline Identity identity(const Json& j,const Json& manifest){
 shape(j,{"owner","keyEnv","publicKey"});Identity out{j.at("owner").get<std::string>(),j.at("keyEnv").get<std::string>(),bytes(j.at("publicKey"),48)};
 require(out.owner=="mac-native"||out.owner=="mac-docker"||out.owner=="vps-docker","owner");require(out.environment.starts_with("NANO_TEST_CAPELLA_NETWORK_KEY_")&&out.environment.size()<=128&&std::all_of(out.environment.begin(),out.environment.end(),[](unsigned char c){return (c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';}),"environment authority");
 const auto& keys=manifest.at("publicKeys");require(keys.is_array()&&!keys.empty(),"public keys");size_t matches=0;for(const auto& row:keys){bytes(row.at("publicKey"),48);if(row.at("publicKey")==hex(out.public_key))++matches;}require(matches==1,"fresh public key membership");return out;
}
inline std::optional<uint64_t> resolve(const ConsensusSlashingState& registry,const Identity& id){std::optional<uint64_t> result;for(const auto& [index,v]:registry.validators)if(Bytes(v.public_key.begin(),v.public_key.end())==id.public_key){require(!result,"duplicate registry key");result=index;}return result;}
inline void local_configuration(const Json& c,const Identity& id,const std::string& owner,const std::optional<uint64_t>& index){
 if(id.owner!=owner)return;const auto& keys=c.at("validator").at("keys");require(keys.is_array(),"configured keys");size_t found=0;for(const auto& row:keys)if(row.at("keyEnv")==id.environment){++found;if(!row.at("index").is_null())require(index&&number(row.at("index"))==*index,"configured index differs from authenticated public key");}require(found==1,"selected configured key missing or duplicated");
}
inline Json identity_view(const Phase0ForkSlashingContext& context,const Identity& id){
 const auto index=resolve(context.registry,id);Json out{{"owner",id.owner},{"keyEnv",id.environment},{"publicKey",hex(id.public_key)},{"resolvedIndex",index?Json(q(*index)):Json(nullptr)},{"registered",bool(index)},{"epoch",q(context.registry.epoch)},{"activationEligibilityEpoch",nullptr},{"activationEpoch",nullptr},{"exitEpoch",nullptr},{"withdrawableEpoch",nullptr},{"slashed",nullptr},{"balanceGwei",nullptr},{"effectiveBalanceGwei",nullptr},{"withdrawalCredentials",nullptr},{"withdrawalRecipient",nullptr}};
 if(index){const auto& v=context.registry.validators.at(*index);out["activationEligibilityEpoch"]=q(v.activation_eligibility_epoch);out["activationEpoch"]=q(v.activation_epoch);out["exitEpoch"]=q(v.exit_epoch);out["withdrawableEpoch"]=q(v.withdrawable_epoch);out["slashed"]=v.slashed;out["balanceGwei"]=q(v.balance);out["effectiveBalanceGwei"]=q(v.effective_balance);out["withdrawalCredentials"]=text(v.withdrawal_credentials);if(v.withdrawal_credentials[0]==1)out["withdrawalRecipient"]=hex(Bytes(v.withdrawal_credentials.begin()+12,v.withdrawal_credentials.end()));}return out;
}
inline ConsensusBeaconHeader signed_header(const Bytes& wire,const CapellaForkBlockRules& r,const std::function<void()>& check){
 require(wire.size()>=184&&wire.size()<=r.transition.consensus.maximum_block_bytes,"block width");auto n=[&](size_t at,size_t width=8){require(at<=wire.size()&&width<=wire.size()-at,"block offset");uint64_t x=0;for(size_t k=0;k<width;++k)x|=uint64_t(wire[at+k])<<(8*k);return x;};auto rt=[&](size_t at){require(at<=wire.size()&&32<=wire.size()-at,"root offset");ConsensusRoot out{};std::copy_n(wire.begin()+at,32,out.begin());return out;};
 require(n(0,4)==100&&n(180,4)==84,"signed block offsets");const auto& t=r.transition;return {n(100),n(108),rt(116),rt(148),capella_beacon_body_root(std::span<const uint8_t>(wire).subspan(184),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,check)};
}
inline ConsensusBytes4 fork_version(const Phase0ForkSlashingContext& c,uint64_t epoch){return epoch<c.fork.epoch?c.fork.previous_version:c.fork.current_version;}
class Secret {
 blst_scalar scalar_{};
public:
 explicit Secret(const Identity& id){
  blst_scalar candidate{};struct ClearScalar{blst_scalar& s;~ClearScalar(){OPENSSL_cleanse(&s,sizeof(s));}} clear_scalar{candidate};
  // This is the sole key-environment access in the fresh implementation.
  const char* value=std::getenv(id.environment.c_str());require(value!=nullptr,"selected fresh key absent");size_t n=0;while(n<67&&value[n])++n;require(n==66,"fresh scalar width");std::string copied(value,n);Bytes decoded;
  struct Clean{std::string& s;Bytes& b;~Clean(){if(!s.empty())OPENSSL_cleanse(s.data(),s.size());if(!b.empty())OPENSSL_cleanse(b.data(),b.size());}} clean{copied,decoded};
  require(copied.starts_with("0x"),"scalar encoding");decoded=unhex(copied);require(decoded.size()==32&&hex(decoded)==copied,"canonical scalar");blst_scalar_from_bendian(&candidate,decoded.data());require(blst_sk_check(&candidate),"scalar validity");std::array<uint8_t,32> roundtrip{};blst_bendian_from_scalar(roundtrip.data(),&candidate);const bool equal=std::equal(roundtrip.begin(),roundtrip.end(),decoded.begin());OPENSSL_cleanse(roundtrip.data(),roundtrip.size());require(equal,"scalar roundtrip");blst_p1 p;blst_sk_to_pk_in_g1(&p,&candidate);Bytes pub(48);blst_p1_compress(pub.data(),&p);require(pub==id.public_key,"selected secret public key differs");scalar_=candidate;
 }
 Secret(const Secret&)=delete;Secret& operator=(const Secret&)=delete;
 ~Secret(){OPENSSL_cleanse(&scalar_,sizeof(scalar_));}
 Bytes sign(const ConsensusRoot& message){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,message.data(),message.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&scalar_);Bytes out(96);blst_p2_compress(out.data(),&s);return out;}
};
inline void private_directory(const fs::path& p){path_guard(p.parent_path());require(::mkdir(p.c_str(),0700)==0,"new output required");struct stat st{};require(::lstat(p.c_str(),&st)==0&&S_ISDIR(st.st_mode)&&st.st_uid==::geteuid()&&(st.st_mode&0777)==0700,"private output directory");}
inline Json write_private(Inputs& in,const fs::path& p,std::string_view raw){Fd f(::open(p.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));size_t at=0;while(at<raw.size()){in.check();const auto n=::write(f.fd,raw.data()+at,raw.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"output write");at+=size_t(n);}require(::fsync(f.fd)==0,"output fsync");Fd parent(::open(p.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));require(::fsync(parent.fd)==0,"output parent fsync");struct stat a{},b{};require(::fstat(f.fd,&a)==0&&::lstat(p.c_str(),&b)==0&&same(a,b)&&S_ISREG(a.st_mode)&&a.st_nlink==1&&a.st_uid==::geteuid()&&(a.st_mode&07777)==0600,"new output identity");in.output(p,a,raw);return {{"sha256",sha(raw)},{"bytes",raw.size()}};}
inline std::optional<CheckpointArchiveEntry> archive(ChainStore& disk,const ConsensusRoot& id,size_t maximum,const std::function<void()>& check){check();require(maximum<=(SIZE_MAX-512)/2,"archive envelope bound");auto entry=disk.checkpoint_archive(Bytes(id.begin(),id.end()),2*maximum+512);require(!entry||entry->payload.size()<=maximum,"archive payload bound");return entry;}

// Shared stopped-copy validation owns handles only for the callback's lifetime.
// Inventories and admitted inputs are fenced after every handle has closed,
// including callback refusal. Same-user writer races are not authorized.
template<class Callback>
Json stopped(int argc,char** argv,Callback&& callback){
 require(argc==11,"fresh lifecycle argc");current_stage="input_authority";const auto began=std::chrono::steady_clock::now();uint64_t duration=60;
 const auto check=[&]{require(std::chrono::steady_clock::now()-began<std::chrono::seconds(duration),"lifecycle deadline");};Inputs in(check);
 const fs::path manifest_path(argv[2]);const auto manifest_raw=in.read(manifest_path,16*1024*1024),config_raw=in.read(argv[3],16*1024*1024),stop_raw=in.read(argv[4],16*1024*1024),request_raw=in.read(argv[5],1024*1024);
 const std::string request_sha(argv[6]);digest(Json(request_sha));require(sha(request_raw)==request_sha,"external request digest");const auto manifest=parse(manifest_raw),configuration=parse(config_raw),stop=parse(stop_raw),request=parse(request_raw);
 shape(request,{"format","owner","manifestSha256","configurationSha256","stopSha256","expectedHead","identities","resources","stage","action","syntheticActionAuthorized","intentionallyAuthorConflictingHeaders","crossOwnerEvidenceTransferAuthorized","evidence"});
 require(request.at("format")=="nano-fresh-lifecycle-request-v1"&&request.at("manifestSha256")==sha(manifest_raw)&&request.at("configurationSha256")==sha(config_raw)&&request.at("stopSha256")==sha(stop_raw),"request input identity");
 const std::string owner(argv[9]);require(request.at("owner")==owner&&(owner=="mac-native"||owner=="mac-docker"||owner=="vps-docker"),"request owner");
 require(manifest.at("format")=="nano-isolated-capella-fixture-domain-v1"&&manifest.at("synthetic")==true&&manifest.at("productionQualified")==false&&manifest.at("fork")=="Capella/Shanghai","fresh synthetic manifest");digest(manifest.at("campaignId"));
 const auto& res=request.at("resources");shape(res,{"maximumWallSeconds","maximumHistoryBlocks","maximumArchiveBytes","maximumIndexBytes","maximumInventoryFiles","maximumInventoryBytes"});duration=number(res.at("maximumWallSeconds"));require(duration>0&&duration<=(std::string(argv[1])=="--fresh-action"?60u:600u),"wall budget");const auto inventory_files=number(res.at("maximumInventoryFiles")),inventory_bytes=number(res.at("maximumInventoryBytes"));require(inventory_files>0&&inventory_bytes>0,"inventory budgets");
 current_stage="domain_configuration";const auto& c=configuration.at("capella");const auto base=manifest_path.parent_path();const auto profile=bound_file(in,base,manifest.at("files").at("profile"),"profile.json",1024*1024);const auto rules=load_capella_fork_profile(profile,sha256(Bytes(profile.begin(),profile.end())),1024*1024);require(c.at("profile").at("sha256")==hex(sha256(Bytes(profile.begin(),profile.end())))&&rules.native_deposit_votes,"native profile binding");
 const auto genesis_raw=bound_file(in,base,manifest.at("files").at("genesis"),"genesis.json",1024*1024);const auto genesis=Genesis::from_json(parse(genesis_raw));const auto chain=quantity(manifest.at("chainId"));const auto execution=bytes(manifest.at("executionGenesisHash"),32);require(chain==1337&&rules.transition.chain_id==chain&&genesis.chain_id==chain&&genesis.consensus=="beacon"&&genesis.evm_revision==EvmRevision::Shanghai&&genesis.block.header.hash()==execution,"chain identity");
 const auto& l=c.at("limits");const CapellaStoreSnapshotLimits limits{size_t(number(l.at("envelopeBytes"))),size_t(number(l.at("blocks"))),size_t(number(l.at("messages"))),size_t(number(l.at("equivocations"))),size_t(number(l.at("stateBytes"))),size_t(number(l.at("totalStateBytes"))),number(l.at("replaySlots")),number(l.at("syncTrials"))};require(limits.envelope_bytes>0&&limits.envelope_bytes<=128*1024*1024&&limits.blocks>0&&limits.blocks<=1024&&limits.state_bytes>0&&limits.total_state_bytes>=limits.state_bytes&&limits.replay_slots>0&&limits.sync_trials>0,"snapshot budgets");
 const auto initial_raw=bound_file(in,base,manifest.at("files").at("initial"),"initial.json",limits.envelope_bytes);const auto initial=capella_store_from_snapshot(initial_raw,sha256(Bytes(initial_raw.begin(),initial_raw.end())),rules,limits,check);const auto anchor=root(manifest.at("initialHeadRoot"));require(initial.blocks.size()==1&&initial.timed.view.anchor==anchor&&capella_network_anchor(initial)==anchor,"trusted fresh anchor");const auto seed=initial.blocks.at(anchor).lease(check);require(text(seed->header.state_root)==manifest.at("initialStateRoot").get<std::string>(),"initial state commitment");
 shape(request.at("identities"),{"voluntaryExit","slashing"});const std::array<Identity,2> ids{identity(request.at("identities").at("voluntaryExit"),manifest),identity(request.at("identities").at("slashing"),manifest)};require(ids[0].public_key!=ids[1].public_key&&ids[0].environment!=ids[1].environment,"action identities must be disjoint");
 const auto seed_context=capella_fork_block_context(seed->state,seed->header,{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes},rules.transition.consensus.state,check);require(text(seed_context.genesis_root)==manifest.at("genesisValidatorsRoot").get<std::string>()&&resolve(seed_context.registry,ids[1]).has_value(),"initial slashing identity/domain");
 require(stop.at("event")=="capella-runtime-stopped"&&stop.at("clockHealthy")==true&&stop.at("validator").at("healthy")==true&&stop.at("validator").at("failure").is_null(),"healthy independent stop");const auto primary_token=token(stop.at("restart"));const auto& witness_pins=stop.at("witnessCommitments");require(witness_pins.at("format")=="nano-isolated-stopped-external-witness-commitments-v1","external witness authority");digest(witness_pins.at("sourceCaptureSha256"));const auto witness_token=token(witness_pins.at("core"));require(primary_token!=witness_token,"independent tokens");
 const fs::path data(argv[7]),witness(argv[8]),out(argv[10]);path_guard(data);path_guard(witness);require(data!=witness&&!inside(data,witness)&&!inside(witness,data)&&out.is_absolute()&&out==out.lexically_normal()&&!inside(data,out)&&!inside(witness,out)&&!inside(out,data)&&!inside(out,witness)&&!inside(base,out)&&!inside(out,base),"independent output/root roles");const auto p=data/"chain",w=witness/"chain-witness";path_guard(p);path_guard(w);struct stat ps{},ws{};require(::lstat(p.c_str(),&ps)==0&&::lstat(w.c_str(),&ws)==0&&(ps.st_dev!=ws.st_dev||ps.st_ino!=ws.st_ino),"core roles alias");
 current_stage="physical_inventory_before";const auto before_p=checkpoint_blob_gc_copy_inventory(p,inventory_files,inventory_bytes,check),before_w=checkpoint_blob_gc_copy_inventory(w,inventory_files,inventory_bytes,check);Json result;std::exception_ptr error;const char* callback_failure_stage=nullptr;
 try{
  current_stage="read_only_closures";ChainStore primary(p,ChainStore::OpenMode::ReadOnly),independent(w,ChainStore::OpenMode::ReadOnly);primary.require_existing_chain_binding(chain,execution,"beacon");independent.require_existing_chain_binding(chain,execution,"nano-capella-recovery-v1");const auto pr=primary.fork_checkpoint(chain,execution,limits.envelope_bytes,primary_token),wr=independent.fork_checkpoint(chain,execution,2048+8*limits.envelope_bytes,witness_token);require(pr&&wr,"external checkpoints absent");const auto intent=parse(wr->snapshot);shape(intent,{"format","previous","next","snapshot","archive"});require(intent.dump()==wr->snapshot&&intent.at("format")=="nano-capella-recovery-v1"&&token(intent.at("next"))==primary_token&&intent.at("snapshot").get<std::string>()==pr->snapshot,"witness intent mismatch");if(intent.at("previous").is_null())require(primary_token.sequence==1,"initial witness sequence");else{const auto previous=token(intent.at("previous"));require(previous.sequence<UINT64_MAX&&previous.sequence+1==primary_token.sequence,"witness sequence continuity");}
  std::vector<CheckpointArchiveEntry> wa;require(intent.at("archive").is_array()&&intent.at("archive").size()<=1,"witness archive bound");for(const auto& a:intent.at("archive")){require(a.is_array()&&a.size()==3,"witness archive shape");wa.push_back({bytes(a.at(0),32),number(a.at(1)),unhex(a.at(2).get<std::string>())});}
  const auto pc=authenticate_capella_checkpoint_blob_closure(pr->snapshot,primary_token.digest,rules,limits,[&](const Bytes& id,size_t max){return primary.checkpoint_blob(chain,execution,id,max);},{},check);const auto wc=authenticate_capella_checkpoint_blob_closure(pr->snapshot,primary_token.digest,rules,limits,[&](const Bytes& id,size_t max){return independent.checkpoint_blob(chain,execution,id,max);},wa,check);require(pc.description()==wc.description()&&pc.observation()==wc.observation(),"independent complete closures");const auto context=pc.head_context();const auto observation=pc.observation();require(text(context.genesis_root)==manifest.at("genesisValidatorsRoot").get<std::string>()&&pc.description().at("identity").at("networkAnchor")==text(anchor)&&observation.at("finalized")==stop.at("finalized")&&observation.at("justified").at("epoch")==stop.at("justifiedEpoch"),"sealed domain/finality");
  current_stage="sealed_head";const auto& expected=request.at("expectedHead");shape(expected,{"root","stateRoot","slot"});require(root(expected.at("root"))==context.block_root&&expected.at("slot")==observation.at("headSlot"),"sealed HEAD root/slot");ConsensusBeaconHeader head{};if(context.block_root==anchor)head=seed->header;else{const auto entry=archive(primary,context.block_root,rules.transition.consensus.maximum_block_bytes,check);require(bool(entry),"head archive");head=signed_header(entry->payload,rules,check);require(entry->root==Bytes(context.block_root.begin(),context.block_root.end())&&entry->slot==head.slot&&consensus_beacon_header_root(head)==context.block_root,"head archive commitment");}require(root(expected.at("stateRoot"))==head.state_root,"sealed HEAD state root");
  for(const auto& id:ids)local_configuration(c,id,owner,resolve(context.registry,id));in.fence();
  private_directory(out);result=callback(in,manifest,c,request,ids,context,head,pc,initial,primary,rules,limits,out,check);
  const auto pp=primary.fork_checkpoint(chain,execution,limits.envelope_bytes,primary_token),ww=independent.fork_checkpoint(chain,execution,2048+8*limits.envelope_bytes,witness_token);require(pp&&ww&&pp->snapshot==pr->snapshot&&ww->snapshot==wr->snapshot,"checkpoint changed");
 }catch(...){error=std::current_exception();callback_failure_stage=current_stage;}
 // Do not publish a result until handles are closed and all old material is exact.
 current_stage="physical_and_input_final_fences";require(checkpoint_blob_gc_copy_inventory(p,inventory_files,inventory_bytes,check)==before_p&&checkpoint_blob_gc_copy_inventory(w,inventory_files,inventory_bytes,check)==before_w,"physical copies changed");in.fence();if(error){current_stage=callback_failure_stage;std::rethrow_exception(error);}return result;
}

inline Json network_fields(const Json& manifest,const CapellaForkStore& initial,const CapellaForkBlockRules& rules){const auto chain=quantity(manifest.at("chainId"));const auto execution=bytes(manifest.at("executionGenesisHash"),32);const auto raw=Json::array({"nano-capella-network-v1",quantity(chain),hex(execution),text(capella_network_anchor(initial)),capella_fork_profile(rules)}).dump();const auto hash=sha256(Bytes(raw.begin(),raw.end()));return {{"chainId",quantity(chain)},{"genesisHash",hex(execution)},{"consensusIdentity",hex(hash)},{"forkDigest",hex(Bytes(hash.begin(),hash.begin()+4))}};}
inline uint64_t action_policy(const Json& request,const std::array<Identity,2>& ids,const Phase0ForkSlashingContext& context,const CapellaForkBlockRules& rules){
 require(request.at("stage")=="prepare_action"&&request.at("syntheticActionAuthorized")==true&&request.at("evidence").is_null(),"synthetic action authorization");const auto kind=request.at("action").get<std::string>();require(kind=="voluntary_exit"||kind=="proposer_slashing","action kind");const auto& id=ids[kind=="voluntary_exit"?0:1];require(id.owner==request.at("owner").get<std::string>(),"local action owner");const auto selected=resolve(context.registry,id);require(selected.has_value(),"action key not registered");const auto& v=context.registry.validators.at(*selected);require(v.withdrawal_credentials[0]==1&&v.activation_epoch<=context.registry.epoch&&context.registry.epoch<v.exit_epoch&&!v.slashed,"action key eligibility/credentials");
 if(kind=="voluntary_exit"){const auto tenure=rules.transition.consensus.exit.shard_committee_period;require(request.at("intentionallyAuthorConflictingHeaders")==false&&v.activation_epoch<=UINT64_MAX-tenure&&context.registry.epoch>=v.activation_epoch+tenure,"actual exit tenure");}else require(request.at("intentionallyAuthorConflictingHeaders")==true,"intentional conflicting-header authorization");return *selected;
}
struct PreparedOperation {Bytes wire;size_t signatures=0;};
inline PreparedOperation prepare_operation(const std::string& kind,uint64_t selected,const Identity& id,const Phase0ForkSlashingContext& context,const ConsensusBeaconHeader& head,const CapellaForkBlockRules& rules,const std::function<Bytes(const ConsensusRoot&)>& sign){
 require(bool(sign),"signing callback");PreparedOperation result;
 if(kind=="voluntary_exit"){SignedConsensusVoluntaryExit exit{context.registry.epoch,selected,{}};exit.signature=sign(consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,exit.validator_index),consensus_domain({4,0,0,0},fork_version(context,exit.epoch),context.genesis_root)));++result.signatures;auto trial=context.registry;require(process_phase0_voluntary_exit(trial,rules.transition.consensus.exit,exit,fork_version(context,exit.epoch),context.genesis_root),"signed exit refused");result.wire=encode_consensus_voluntary_exit_ssz(exit);
 }else{require(kind=="proposer_slashing","operation kind");auto first=head;first.proposer_index=selected;auto second=first;second.body_root[0]^=1;const auto version=fork_version(context,head.slot/rules.transition.consensus.epoch.ffg.slots_per_epoch);SignedConsensusBeaconHeader a{first,sign(consensus_signing_root(consensus_beacon_header_root(first),consensus_domain({0,0,0,0},version,context.genesis_root)))},b{second,sign(consensus_signing_root(consensus_beacon_header_root(second),consensus_domain({0,0,0,0},version,context.genesis_root)))};result.signatures=2;require(verify_proposer_slashing(a,b,id.public_key,version,context.genesis_root),"signed conflicting headers refused");auto trial=context.registry;require(process_phase0_proposer_slashing(trial,rules.transition.consensus.epoch.slashing,a,b,version,context.genesis_root,selected,selected).has_value(),"slashing operation refused");result.wire=encode_consensus_proposer_slashing_ssz(a,b);}
 return result;
}
inline Json action(Inputs& in,std::string_view request_sha,const Json& manifest,const Json& request,const std::array<Identity,2>& ids,const Phase0ForkSlashingContext& context,const ConsensusBeaconHeader& head,const CapellaForkStore& initial,const CapellaForkBlockRules& rules,const fs::path& out){
 current_stage="action_policy";require(request.at("crossOwnerEvidenceTransferAuthorized")==false,"action has no evidence transfer");const auto selected=action_policy(request,ids,context,rules);const auto kind=request.at("action").get<std::string>();const auto& id=ids[kind=="voluntary_exit"?0:1];in.check();current_stage="selected_fresh_key";Secret key(id);current_stage="operation_construction";const auto operation=prepare_operation(kind,selected,id,context,head,rules,[&](const ConsensusRoot& m){return key.sign(m);});const auto& wire=operation.wire;const auto signatures=operation.signatures;
 auto envelope=network_fields(manifest,initial,rules);envelope["kind"]=kind;envelope["ssz"]=hex(wire);
 // AltairWireKind's stable protocol discriminants: exit=3, proposer-slashing=6.
 Bytes content{uint8_t(kind=="voluntary_exit"?3:6)};content.insert(content.end(),wire.begin(),wire.end());const auto payload_digest=hex(sha256(content));const Json private_action{{"format","nano-fresh-lifecycle-action-v1"},{"campaignId",manifest.at("campaignId")},{"requestSha256",std::string(request_sha)},{"owner",id.owner},{"kind",kind},{"publicKey",hex(id.public_key)},{"resolvedIndex",q(selected)},{"preparedHead",request.at("expectedHead")},{"payloadDigest",payload_digest},{"envelope",envelope}};const auto pin=write_private(in,out/"action.private.json",private_action.dump());
 return {{"format","nano-fresh-lifecycle-action-result-v1"},{"status","PREPARED"},{"passed",true},{"owner",id.owner},{"campaignId",manifest.at("campaignId")},{"kind",kind},{"resolvedIndex",q(selected)},{"headRoot",text(context.block_root)},{"preparedEpoch",q(context.registry.epoch)},{"actionSha256",pin.at("sha256")},{"actionBytes",pin.at("bytes")},{"payloadDigest",payload_digest},{"newSignaturesCreated",signatures},{"submitted",false},{"canonicalInclusionVerified",false},{"physicalCopiesUnchanged",true},{"productionQualified",false}};
}
} // namespace nano::test::fresh_lifecycle
#include "fresh_lifecycle_terminal.hpp"
namespace nano::test::fresh_lifecycle {
inline Json run(int argc,char** argv){
 require(argc==11,"fresh lifecycle argc");const bool signing=std::string(argv[1])=="--fresh-action";require(signing||std::string(argv[1])=="--fresh-audit","fresh mode");
 return stopped(argc,argv,[&](Inputs& in,const Json& manifest,const Json&,const Json& request,const std::array<Identity,2>& ids,const Phase0ForkSlashingContext& context,const ConsensusBeaconHeader& head,const AuthenticatedCheckpointBlobClosure& closure,const CapellaForkStore& initial,ChainStore& disk,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const fs::path& out,const std::function<void()>& check){
  if(signing)return action(in,argv[6],manifest,request,ids,context,head,initial,rules,out);
  if(request.at("stage")=="terminal")return terminal_audit(in,manifest,request,ids,context,head,closure,initial,disk,rules,limits,out,check);
  require(request.at("stage")=="observe"&&request.at("action").is_null()&&request.at("evidence").is_null()&&request.at("syntheticActionAuthorized")==false&&request.at("intentionallyAuthorConflictingHeaders")==false,"observe-only request");require(request.at("crossOwnerEvidenceTransferAuthorized")==false,"observe has no evidence transfer");current_stage="sealed_registry_observation";Json identities=Json::array();for(const auto& id:ids)identities.push_back(identity_view(context,id));
  return Json{{"format","nano-fresh-lifecycle-audit-result-v1"},{"status","OBSERVED_SCOPED_SEALED_HEAD"},{"passed",true},{"stage","observe"},{"owner",request.at("owner")},{"campaignId",manifest.at("campaignId")},{"headRoot",text(context.block_root)},{"headStateRoot",text(head.state_root)},{"headSlot",q(head.slot)},{"finalized",closure.observation().at("finalized")},{"identities",identities},{"completePrimaryWitnessClosuresVerified",true},{"externalCoreTokensMatched",true},{"canonicalSignedStateReplayVerified",false},{"canonicalInclusionVerified",false},{"historicalForkChoiceVerified",false},{"restorationProcessExecutionInferred",false},{"physicalCopiesUnchanged",true},{"newSignaturesCreated",0},{"envRead",false},{"runtimeStarted",false},{"productionQualified",false}};
 });
}
inline int main(int argc,char** argv){try{std::cout<<run(argc,argv).dump()<<'\n';return 0;}catch(...){std::cout<<Json{{"format","nano-fresh-lifecycle-refusal-v1"},{"status","REFUSED"},{"passed",false},{"detailsExported",false},{"refusedStage",current_stage},{"runtimeStarted",false},{"productionQualified",false}}.dump()<<'\n';return 1;}}
} // namespace nano::test::fresh_lifecycle
