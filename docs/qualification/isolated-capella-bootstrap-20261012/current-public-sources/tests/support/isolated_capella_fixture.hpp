#pragma once
// Opt-in private fixture tooling. This does not change consensus or signing journals.
#include "nano/consensus_beacon_state.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/crypto.hpp"
#include "blst.h"
#include <openssl/crypto.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace test::isolated_capella {
using nano::Bytes;using nano::ConsensusRoot;using nano::Json;
namespace fs=std::filesystem;
inline constexpr char info_prefix[]="Nano/Test/IsolatedCapella/v1";
inline void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
class Descriptor {
 int value_=-1;
public:
 explicit Descriptor(int value):value_(value){}
 ~Descriptor(){if(value_>=0)::close(value_);}
 Descriptor(const Descriptor&)=delete;Descriptor& operator=(const Descriptor&)=delete;
 int get()const{return value_;}
};
inline void write_all(int fd,std::span<const uint8_t> bytes){
 while(!bytes.empty()){
  const auto n=::write(fd,bytes.data(),bytes.size());
  if(n<0&&errno==EINTR)continue;
  require(n>0,"isolated fixture write failed");bytes=bytes.subspan(size_t(n));
 }
}
inline void sync(int fd){require(::fsync(fd)==0,"isolated fixture durable sync failed");}
inline Bytes read_fd(int fd,size_t size){
 Bytes result(size);size_t done=0;
 while(done<size){const auto n=::read(fd,result.data()+done,size-done);if(n<0&&errno==EINTR)continue;require(n>0,"isolated fixture bounded read failed");done+=size_t(n);}
 uint8_t extra=0;require(::read(fd,&extra,1)==0,"isolated fixture file grew during read");return result;
}
inline bool beneath(const fs::path& path,const fs::path& prefix){
 auto a=path.begin(),b=prefix.begin();for(;b!=prefix.end();++a,++b)if(a==path.end()||*a!=*b)return false;return true;
}
inline void canonical_components(const fs::path& path,bool missing_leaf=false){
 require(path.is_absolute()&&path==path.lexically_normal(),"isolated fixture path must be absolute and canonical");
 fs::path current;
 for(auto it=path.begin();it!=path.end();++it){
  current/=*it;struct stat state{};
  if(::lstat(current.c_str(),&state)<0){require(missing_leaf&&std::next(it)==path.end()&&errno==ENOENT,"isolated fixture path component unavailable");return;}
  require(!S_ISLNK(state.st_mode),"isolated fixture symlink path refused");
 }
}
inline void durable_path(const fs::path& path,bool missing_leaf=false){
 canonical_components(path,missing_leaf);
 const auto actual=missing_leaf?fs::canonical(path.parent_path())/path.filename():fs::canonical(path);
 for(const auto& temporary:std::array<fs::path,4>{"/tmp","/private/tmp","/var/tmp","/private/var/tmp"})require(!beneath(actual,fs::weakly_canonical(temporary)),"isolated fixture temporary output refused");
 require(!beneath(actual,fs::canonical(fs::temp_directory_path())),"isolated fixture system temporary output refused");
 if(const auto* temporary=std::getenv("TMPDIR");temporary&&*temporary)require(!beneath(actual,fs::weakly_canonical(fs::path(temporary))),"isolated fixture temporary output refused");
}
inline void private_directory(const fs::path& path){
 canonical_components(path);Descriptor fd(::open(path.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
 require(fd.get()>=0,"isolated fixture private directory open failed");struct stat state{};
 require(::fstat(fd.get(),&state)==0&&S_ISDIR(state.st_mode)&&state.st_uid==::geteuid()&&(state.st_mode&07777)==0700,"isolated fixture directory requires owner and mode 0700");
}
inline void new_file_at(int directory,const std::string& name,std::span<const uint8_t> bytes,mode_t mode,const char* conflict=nullptr){
 require(!name.empty()&&name.find('/')==std::string::npos&&name!="."&&name!="..","isolated fixture output name refused");
 Descriptor file(::openat(directory,name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,mode));
 if(file.get()<0&&errno==EEXIST&&conflict)throw std::runtime_error(conflict);
 require(file.get()>=0,"isolated fixture output already exists or cannot be created");
 require(::fchmod(file.get(),mode)==0,"isolated fixture output permission failed");write_all(file.get(),bytes);sync(file.get());sync(directory);
}
inline void new_file(const fs::path& directory,const std::string& name,std::span<const uint8_t> bytes,mode_t mode){private_directory(directory);Descriptor dir(::open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));require(dir.get()>=0,"isolated fixture output directory open failed");new_file_at(dir.get(),name,bytes,mode);}
inline void new_json(const fs::path& directory,const std::string& name,const Json& value,mode_t mode){const auto text=value.dump();new_file(directory,name,Bytes(text.begin(),text.end()),mode);}
inline Bytes scalar_bytes(const blst_scalar& scalar){Bytes result(32);blst_bendian_from_scalar(result.data(),&scalar);return result;}
inline Bytes scalar_public_key(const blst_scalar& scalar){blst_p1 point;blst_sk_to_pk_in_g1(&point,&scalar);Bytes result(48);blst_p1_compress(result.data(),&point);return result;}
inline Bytes legacy_public_key(uint64_t index){require(index<UINT64_MAX,"isolated fixture legacy key overflow");uint64_t words[4]{index+1,0,0,0};blst_scalar scalar;blst_scalar_from_uint64(&scalar,words);const auto result=scalar_public_key(scalar);OPENSSL_cleanse(&scalar,sizeof(scalar));return result;}
inline std::string root_text(const ConsensusRoot& root){return nano::hex(Bytes(root.begin(),root.end()));}
inline std::string digest_text(const Bytes& bytes){return nano::hex(nano::sha256(bytes)).substr(2);}
inline std::string digest_json(const Json& value){const auto encoded=value.dump();return digest_text(Bytes(encoded.begin(),encoded.end()));}
inline ConsensusRoot chunk(std::span<const uint8_t> bytes){require(bytes.size()<=32,"isolated fixture SSZ chunk width");ConsensusRoot out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
inline ConsensusRoot merkle(std::vector<ConsensusRoot> nodes,uint64_t limit){
 require(limit&&nodes.size()<=limit,"isolated fixture SSZ list limit");ConsensusRoot zero{};if(nodes.empty())nodes.push_back(zero);
 for(unsigned level=0;level<std::bit_width(limit-1);++level){
  if(nodes.size()%2)nodes.push_back(zero);
  for(size_t i=0;i<nodes.size()/2;++i)nodes[i]=nano::consensus_signing_root(nodes[2*i],nodes[2*i+1]);
  nodes.resize(nodes.size()/2);zero=nano::consensus_signing_root(zero,zero);
 }
 require(nodes.size()==1,"isolated fixture SSZ Merkle width");return nodes[0];
}
inline ConsensusRoot validator_root(std::span<const uint8_t> row){
 require(row.size()==121&&row[88]<=1,"isolated fixture Validator encoding");
 const auto key=nano::consensus_signing_root(chunk(row.first(32)),chunk(row.subspan(32,16)));
 return merkle({key,chunk(row.subspan(48,32)),chunk(row.subspan(80,8)),chunk(row.subspan(88,1)),chunk(row.subspan(89,8)),chunk(row.subspan(97,8)),chunk(row.subspan(105,8)),chunk(row.subspan(113,8))},8);
}
inline ConsensusRoot registry_root(std::span<const uint8_t> bytes,uint64_t limit){
 require(bytes.size()%121==0&&bytes.size()/121<=limit,"isolated fixture Validator list encoding");std::vector<ConsensusRoot> roots;
 for(size_t at=0;at<bytes.size();at+=121)roots.push_back(validator_root(bytes.subspan(at,121)));
 ConsensusRoot count{};const auto n=uint64_t(bytes.size()/121);for(unsigned i=0;i<8;++i)count[i]=uint8_t(n>>(8*i));return nano::consensus_signing_root(merkle(std::move(roots),limit),count);
}
inline uint64_t number(const Bytes& bytes,size_t at,size_t width=8){require(width<=8&&at<=bytes.size()&&width<=bytes.size()-at,"isolated fixture SSZ integer bounds");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(bytes[at+i])<<(8*i);return n;}

class Session {
 struct PrivateSeed {std::array<uint8_t,32> value{};~PrivateSeed(){OPENSSL_cleanse(value.data(),value.size());}};
 PrivateSeed seed_;fs::path destination_,ledger_;std::string fingerprint_,ledger_binding_;
 std::unique_ptr<Descriptor> ledger_fd_,destination_fd_;
 Json public_keys_=Json::array(),withdrawal_keys_=Json::array(),sender_;Bytes validators_,genesis_state_;
 Json bootstrap_history_;std::map<std::pair<uint64_t,uint64_t>,ConsensusRoot> proposals_;
 ConsensusRoot genesis_root_{},genesis_state_root_{};uint64_t registry_limit_=0;
 std::string key_set_,campaign_;bool prepared_=false,reserved_=false,state_ready_=false;uint64_t signatures_started_=0;
 static void directory_binding(const fs::path& path,int fd){
  canonical_components(path);struct stat actual{},named{};
  require(::fstat(fd,&actual)==0&&::lstat(path.c_str(),&named)==0&&S_ISDIR(actual.st_mode)&&S_ISDIR(named.st_mode)&&actual.st_dev==named.st_dev&&actual.st_ino==named.st_ino&&actual.st_uid==::geteuid()&&(actual.st_mode&07777)==0700,"isolated fixture private directory identity changed");
 }
 void directories_bound()const{directory_binding(ledger_,ledger_fd_->get());directory_binding(destination_,destination_fd_->get());}
 static void seed_file(const fs::path& path,std::array<uint8_t,32>& out){
  canonical_components(path);Descriptor fd(::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));require(fd.get()>=0,"isolated fixture private seed open failed");struct stat before{},after{};
  require(::fstat(fd.get(),&before)==0&&S_ISREG(before.st_mode)&&before.st_uid==::geteuid()&&(before.st_mode&07777)==0600&&before.st_size==32,"isolated fixture seed requires owner, mode 0600 and exactly 32 bytes");
  size_t done=0;while(done<out.size()){const auto n=::read(fd.get(),out.data()+done,out.size()-done);if(n<0&&errno==EINTR)continue;require(n>0,"isolated fixture seed bounded read failed");done+=size_t(n);}
  uint8_t extra=0;ssize_t tail;do{tail=::read(fd.get(),&extra,1);}while(tail<0&&errno==EINTR);require(tail==0,"isolated fixture seed grew during read");
  require(::fstat(fd.get(),&after)==0&&before.st_dev==after.st_dev&&before.st_ino==after.st_ino&&before.st_size==after.st_size&&before.st_uid==after.st_uid&&before.st_mode==after.st_mode,"isolated fixture seed changed while reading");
#ifdef __APPLE__
  require(before.st_mtimespec.tv_sec==after.st_mtimespec.tv_sec&&before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec&&before.st_ctimespec.tv_sec==after.st_ctimespec.tv_sec&&before.st_ctimespec.tv_nsec==after.st_ctimespec.tv_nsec,"isolated fixture seed timestamps changed while reading");
#else
  require(before.st_mtim.tv_sec==after.st_mtim.tv_sec&&before.st_mtim.tv_nsec==after.st_mtim.tv_nsec&&before.st_ctim.tv_sec==after.st_ctim.tv_sec&&before.st_ctim.tv_nsec==after.st_ctim.tv_nsec,"isolated fixture seed timestamps changed while reading");
#endif
 }
public:
 Session(const fs::path& destination,const fs::path& seed,const fs::path& ledger):destination_(destination),ledger_(ledger){
  durable_path(destination_,true);durable_path(ledger_);private_directory(ledger_);struct stat absent{};
  require(::lstat(destination_.c_str(),&absent)<0&&errno==ENOENT,"isolated fixture destination must be new");
  seed_file(seed,seed_.value);fingerprint_=digest_text(Bytes(seed_.value.begin(),seed_.value.end()));
  Descriptor parent(::open(destination_.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));require(parent.get()>=0,"isolated fixture destination parent unavailable");
  require(::mkdirat(parent.get(),destination_.filename().c_str(),0700)==0,"isolated fixture destination creation failed");private_directory(destination_);sync(parent.get());
  ledger_fd_=std::make_unique<Descriptor>(::open(ledger_.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));destination_fd_=std::make_unique<Descriptor>(::open(destination_.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));require(ledger_fd_->get()>=0&&destination_fd_->get()>=0,"isolated fixture private directory open failed");directories_bound();struct stat identity{};require(::fstat(ledger_fd_->get(),&identity)==0,"isolated fixture ledger identity failed");
  ledger_binding_=digest_json(Json{{"format","nano-isolated-capella-ledger-binding-v1"},{"ledgerPath",ledger_.string()},{"device",std::to_string(identity.st_dev)},{"inode",std::to_string(identity.st_ino)}});
 }
 ~Session()=default;
 Session(const Session&)=delete;Session& operator=(const Session&)=delete;
 static std::unique_ptr<Session> from_environment(const fs::path& destination){
  const auto* seed=std::getenv("NANO_TEST_ISOLATED_CAPELLA_SEED_FILE"),*ledger=std::getenv("NANO_TEST_ISOLATED_CAPELLA_LEDGER_DIR");
  require(seed&&*seed&&ledger&&*ledger,"isolated fixture seed file and persistent ledger paths required");return std::make_unique<Session>(destination,fs::path(seed),fs::path(ledger));
 }
 blst_scalar derive(uint8_t role,uint64_t index)const{
  require((role==0&&index<=64)||(role==1&&index<64)||(role==2&&index==0),"isolated fixture key role/index bounds");
  Bytes info(info_prefix,info_prefix+sizeof(info_prefix)-1);info.push_back(role);for(unsigned i=0;i<8;++i)info.push_back(uint8_t(index>>(8*(7-i))));
  blst_scalar scalar;blst_keygen(&scalar,seed_.value.data(),seed_.value.size(),info.data(),info.size());require(blst_sk_check(&scalar),"isolated fixture derived scalar invalid");return scalar;
 }
 blst_scalar indexed_key(uint64_t index)const{return index>=1000?derive(1,index-1000):derive(0,index);}
 Bytes validator_secret(uint64_t index)const{require_reserved();auto scalar=derive(0,index);auto result=scalar_bytes(scalar);OPENSSL_cleanse(&scalar,sizeof(scalar));return result;}
 Bytes sender_secret()const{require_reserved();auto scalar=derive(2,0);auto result=scalar_bytes(scalar);OPENSSL_cleanse(&scalar,sizeof(scalar));return result;}
 void require_reserved()const{require(reserved_,"isolated fixture signing refused before durable identity reservation");directories_bound();}
 void before_signature(){require_reserved();require(signatures_started_<UINT64_MAX,"isolated fixture signature counter exhausted");++signatures_started_;}
 uint64_t signatures_started()const{return signatures_started_;}
 void authorize_proposal(uint64_t index,uint64_t slot,const ConsensusRoot& signing_root){require_reserved();require(index<64,"isolated fixture bootstrap proposer bounds");const auto [at,inserted]=proposals_.emplace(std::pair{index,slot},signing_root);require(inserted||at->second==signing_root,"isolated fixture conflicting proposal refused before signature");}
 void prepare_genesis(Bytes& state,const nano::AltairStateLimits& limits){
  require(!prepared_&&!reserved_,"isolated fixture genesis identity already prepared");
  require(limits.slots_per_historical_root<=UINT32_MAX/64&&limits.epochs_per_historical_vector<=UINT32_MAX/32&&limits.epochs_per_slashings_vector<=UINT32_MAX/8&&limits.sync_committee_size<=UINT32_MAX/96,"isolated fixture layout bounds");
  const size_t history=176+64*limits.slots_per_historical_root,previous=history+96+32*limits.epochs_per_historical_vector+8*limits.epochs_per_slashings_vector,current=previous+133,width=48*(limits.sync_committee_size+1),next=current+width;
  const auto start=number(state,history+88,4),end=number(state,history+92,4);
  require(number(state,40)==0&&end>=start&&end<=state.size()&&end-start==64*121&&state.size()>=40&&next<=state.size()&&width<=state.size()-next,"isolated fixture requires bounded 64-validator genesis state");
  const ConsensusRoot old_root=chunk(std::span<const uint8_t>(state).subspan(8,32));
  std::map<Bytes,size_t> old_members;std::set<Bytes> old_keys,new_keys;std::array<Bytes,64> keys;
  for(uint64_t i=0;i<65;++i)old_keys.insert(legacy_public_key(i));for(uint64_t i=0;i<64;++i)old_keys.insert(legacy_public_key(1000+i));
  for(size_t i=0;i<64;++i){const auto at=size_t(start)+121*i;Bytes old(state.begin()+at,state.begin()+at+48);require(old_members.emplace(old,i).second&&state[at+48]==0,"isolated fixture duplicate initial key or non-BLS credentials");old_keys.insert(std::move(old));}
  for(uint64_t i=0;i<65;++i){auto scalar=derive(0,i);auto key=scalar_public_key(scalar);OPENSSL_cleanse(&scalar,sizeof(scalar));require(!old_keys.contains(key)&&new_keys.insert(key).second,"isolated fixture validator key isolation failed");public_keys_.push_back({{"index",i},{"publicKey",nano::hex(key)}});if(i<64)keys[i]=std::move(key);}
  for(uint64_t i=0;i<64;++i){auto scalar=derive(1,i);auto key=scalar_public_key(scalar);OPENSSL_cleanse(&scalar,sizeof(scalar));require(!old_keys.contains(key)&&new_keys.insert(key).second,"isolated fixture withdrawal key isolation failed");withdrawal_keys_.push_back({{"index",i},{"publicKey",nano::hex(key)}});auto credentials=nano::sha256(key);credentials[0]=0;const auto at=size_t(start)+121*i;require(!std::equal(credentials.begin(),credentials.end(),state.begin()+at+48),"isolated fixture withdrawal credentials reused");std::copy(keys[i].begin(),keys[i].end(),state.begin()+at);std::copy(credentials.begin(),credentials.end(),state.begin()+at+48);}
  for(const auto begin:{current,next}){std::vector<Bytes> members;for(size_t i=0;i<limits.sync_committee_size;++i){const size_t at=begin+48*i;const auto found=old_members.find(Bytes(state.begin()+at,state.begin()+at+48));require(found!=old_members.end(),"isolated fixture unknown sync committee member");const auto& key=keys[found->second];std::copy(key.begin(),key.end(),state.begin()+at);members.push_back(key);}const auto aggregate=nano::aggregate_consensus_bls_keys(members);require(bool(aggregate),"isolated fixture sync committee aggregate failed");std::copy(aggregate->begin(),aggregate->end(),state.begin()+begin+48*limits.sync_committee_size);}
  validators_=Bytes(state.begin()+start,state.begin()+end);registry_limit_=limits.validator_registry_limit;genesis_root_=registry_root(validators_,registry_limit_);require(genesis_root_!=old_root,"isolated fixture genesis validators root reused");std::copy(genesis_root_.begin(),genesis_root_.end(),state.begin()+8);
  auto sender_scalar=derive(2,0);auto secret=scalar_bytes(sender_scalar);OPENSSL_cleanse(&sender_scalar,sizeof(sender_scalar));const auto public_key=nano::public_key(secret);OPENSSL_cleanse(secret.data(),secret.size());const auto address=nano::address(nano::public_address(public_key));require(address!=nano::address(nano::public_address(nano::public_key(nano::bytes(nano::Int(777),32)))),"isolated fixture transaction sender reused");sender_={{"address",address},{"publicKey",nano::hex(public_key)}};
  key_set_=digest_json(Json{{"publicKeys",public_keys_},{"withdrawalPublicKeys",withdrawal_keys_}});
  campaign_=digest_json(Json{{"format","nano-isolated-capella-key-identity-v1"},{"genesisValidatorsRoot",root_text(genesis_root_)},{"keySetSha256",key_set_},{"transactionSenderAddress",address}});prepared_=true;
 }
 void reserve(){
  require(prepared_&&!reserved_,"isolated fixture reservation requires prepared unique identity");directories_bound();
  const Json record{{"format","nano-isolated-capella-private-reservation-v1"},{"seedFingerprint",fingerprint_},{"campaignId",campaign_},{"keySetSha256",key_set_},{"genesisValidatorsRoot",root_text(genesis_root_)},{"ledgerBindingSha256",ledger_binding_},{"ledgerPath",ledger_.string()},{"status","consumed-before-signing"}};
  // O_EXCL is the cross-process decision. Do not delete this marker on any failure.
  const auto text=record.dump();const Bytes bytes(text.begin(),text.end());new_file_at(ledger_fd_->get(),fingerprint_+".json",bytes,0600,"isolated fixture seed already reserved in selected ledger");directories_bound();new_file_at(destination_fd_->get(),"reservation-receipt.private.json",bytes,0600);directories_bound();reserved_=true;
 }
 void record_genesis_state(const Bytes& state,const nano::AltairStateLimits& limits,const nano::ExecutionPayloadLimits& execution,size_t budget){
  require_reserved();require(!state_ready_,"isolated fixture genesis state already recorded");const size_t history=176+64*limits.slots_per_historical_root;const auto start=number(state,history+88,4),end=number(state,history+92,4);
  require(end>=start&&end<=state.size()&&Bytes(state.begin()+start,state.begin()+end)==validators_&&chunk(std::span<const uint8_t>(state).subspan(8,32))==genesis_root_,"isolated fixture pretransition registry changed");
  genesis_state_root_=nano::capella_beacon_state_root(state,limits,execution,budget);genesis_state_=state;state_ready_=true;
 }
 void record_bootstrap(const Bytes& signed_block,uint64_t slot,uint64_t proposer,const ConsensusRoot& block_root,uint64_t epoch,const ConsensusRoot& randao_root,const Bytes& randao,uint64_t changed_index,const Bytes& withdrawal_key,const ConsensusRoot& change_root,const Bytes& change_signature){
  require_reserved();require(bootstrap_history_.is_null()&&slot==1&&proposer<64&&changed_index==0&&signed_block.size()>=100&&randao.size()==96&&change_signature.size()==96&&signatures_started_==3&&proposals_.size()==1&&proposals_.at({proposer,slot})==block_root,"isolated fixture bootstrap signing history is not unique");
  bootstrap_history_={{"block",{{"slot",slot},{"validatorIndex",proposer},{"signingRoot",root_text(block_root)},{"signature",nano::hex(Bytes(signed_block.begin()+4,signed_block.begin()+100))},{"ssz",nano::hex(signed_block)}}},{"randao",{{"epoch",epoch},{"validatorIndex",proposer},{"signingRoot",root_text(randao_root)},{"signature",nano::hex(randao)}}},{"blsToExecutionChange",{{"validatorIndex",changed_index},{"publicKey",nano::hex(withdrawal_key)},{"signingRoot",root_text(change_root)},{"signature",nano::hex(change_signature)}}},{"attestations",Json::array()},{"syncMessages",Json::array()}};
 }
 void record_deposit(const ConsensusRoot& root,const Bytes& public_key,const Bytes& signature,const Bytes& credentials,uint64_t amount){
  require_reserved();require(!bootstrap_history_.is_null()&&!bootstrap_history_.contains("deposit")&&signatures_started_==4&&nano::hex(public_key)==public_keys_.at(64).at("publicKey").get<std::string>()&&signature.size()==96&&credentials.size()==32,"isolated fixture deposit signing history differs");
  bootstrap_history_["deposit"]={{"validatorIndex",64},{"publicKey",nano::hex(public_key)},{"signingRoot",root_text(root)},{"signature",nano::hex(signature)},{"withdrawalCredentials",nano::hex(credentials)},{"amountGwei",amount}};
 }
 void save_public_file(const std::string& name,const Bytes& value)const{require_reserved();new_file_at(destination_fd_->get(),name,value,0644);directories_bound();}
 void save_private_json(const std::string& name,const Json& value)const{require_reserved();new_json(destination_,name,value,0600);}
 void replace_private_json(const std::string& name,const Json& value)const{
  require_reserved();private_directory(destination_);require(name.find('/')==std::string::npos,"isolated fixture private replacement name refused");
  Descriptor dir(::open(destination_.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));require(dir.get()>=0,"isolated fixture replacement directory open failed");struct stat prior{};
  require(::fstatat(dir.get(),name.c_str(),&prior,AT_SYMLINK_NOFOLLOW)==0&&S_ISREG(prior.st_mode)&&prior.st_uid==::geteuid()&&(prior.st_mode&07777)==0600,"isolated fixture replacement requires private existing file");
  const auto next=name+".replacement";new_json(destination_,next,value,0600);require(::renameat(dir.get(),next.c_str(),dir.get(),name.c_str())==0,"isolated fixture private replacement failed");sync(dir.get());
 }
 void check_destination(const fs::path& selected)const{require_reserved();require(selected==destination_,"isolated fixture destination changed");private_directory(destination_);}
 Json export_domain(const std::string& execution_genesis,const std::string& initial_head,const std::string& initial_state){
  require_reserved();require(state_ready_&&!bootstrap_history_.is_null()&&bootstrap_history_.contains("deposit")&&signatures_started_==4,"isolated fixture pretransition state or clean bootstrap signing history unavailable");
  new_file(destination_,"genesis-validators.ssz",validators_,0644);new_file(destination_,"genesis-state.ssz",genesis_state_,0644);
  Json files=Json::object();for(const auto& [kind,name]:std::array<std::pair<const char*,const char*>,5>{{{"genesisValidators","genesis-validators.ssz"},{"genesisState","genesis-state.ssz"},{"genesis","genesis.json"},{"profile","profile.json"},{"initial","initial.json"}}}){
   canonical_components(destination_/name);Descriptor fd(::open((destination_/name).c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC));require(fd.get()>=0,"isolated fixture public file open failed");struct stat state{};require(::fstat(fd.get(),&state)==0&&S_ISREG(state.st_mode)&&state.st_size>=0&&uint64_t(state.st_size)<=128*1024*1024,"isolated fixture public file budget");const auto bytes=read_fd(fd.get(),size_t(state.st_size));files[kind]={{"path",name},{"sha256",digest_text(bytes)},{"bytes",bytes.size()}};
  }
  Json result{{"format","nano-isolated-capella-fixture-domain-v1"},{"synthetic",true},{"productionQualified",false},{"fork","Capella/Shanghai"},{"chainId","0x539"},{"campaignId",campaign_},{"keySetSha256",key_set_},{"genesisValidatorsRoot",root_text(genesis_root_)},{"genesisStateRoot",root_text(genesis_state_root_)},{"executionGenesisHash",execution_genesis},{"initialHeadRoot",initial_head},{"initialStateRoot",initial_state},{"registryLimit",registry_limit_},{"registryCount",64},{"publicKeys",public_keys_},{"withdrawalPublicKeys",withdrawal_keys_},{"transactionSender",sender_},{"keyDerivation",{{"algorithm","blst-keygen"},{"infoPrefix",info_prefix},{"infoEncoding","tag-bytes||role-u8||index-u64-be"},{"validatorRole",0},{"withdrawalRole",1},{"senderRole",2}}},{"legacyKeyDisjointnessVerified",true},{"files",files},{"reservation",{{"format","nano-isolated-capella-reservation-v1"},{"campaignId",campaign_},{"keySetSha256",key_set_},{"genesisValidatorsRoot",root_text(genesis_root_)},{"status","consumed-before-signing"},{"ledgerScope","explicit-operator-managed-ledger"},{"ledgerBindingSha256",ledger_binding_}}}};
  result["bootstrapSigningHistory"]=bootstrap_history_;result["bootstrapDutyPolicy"]={{"anchorSlot",1},{"maximumPresignedProposalSlot",1},{"minimumNewProposalSlot",2},{"historicalProposalSigningAllowed",false},{"requiresClockAtOrAfterAnchor",true},{"driverMayRegeneratePresignedHistory",false}};new_json(destination_,"isolated-domain.json",result,0644);return result;
 }
};
inline std::unique_ptr<Session> active;
inline void require_reservation(){if(active)active->before_signature();}
}
