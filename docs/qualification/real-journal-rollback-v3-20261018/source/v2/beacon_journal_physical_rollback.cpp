#include "nano/consensus_beacon_signer.hpp"
#include "nano/consensus_beacon_journal_auth.hpp"
#include "../../tools/storage-maintenance/bootstrap-readonly-replica.hpp"
#include "ckzg/blst/bindings/blst.h"
#include <openssl/crypto.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/resource.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace nano;
namespace fs=std::filesystem;
namespace rd=nano::maintenance::readonly_replica_detail;
namespace {
constexpr uint64_t maximum_store_bytes=32*1024*1024,maximum_entries=4096,maximum_rss_kib=128*1024;
constexpr uint64_t proposal_slot=10,slots_per_epoch=8;
const Int chain_id=1337;
const auto started=std::chrono::steady_clock::now();
volatile std::sig_atomic_t interrupted=0;
uint64_t provider_calls=0,signatures=0,signer_constructions=0,readonly_opens=0,auth_calls=0,positives=0,refusals=0;
uint64_t admission_samples=0,admission_streak=0,admission_elapsed_ms=0,admission_ctime_changes=0,admission_source_fences=0;
const char* phase="arguments";
void stop(int s){interrupted=s;}
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint64_t rss(){
 struct rusage r{};need(::getrusage(RUSAGE_SELF,&r)==0,"read process RSS");
#ifdef __APPLE__
 return (uint64_t(r.ru_maxrss)+1023)/1024;
#else
 return uint64_t(r.ru_maxrss);
#endif
}
void guard(){need(!interrupted,"test interrupted");need(std::chrono::steady_clock::now()-started<std::chrono::seconds(180),"180 second test budget");need(rss()<=maximum_rss_kib,"128 MiB test RSS budget");}
Bytes bytes(std::string_view s){return Bytes(s.begin(),s.end());}
ConsensusRoot root(std::string_view label){const auto b=sha256(bytes(label));ConsensusRoot r{};std::copy(b.begin(),b.end(),r.begin());return r;}
const Bytes execution_genesis=sha256(bytes("Nano/Test/PhysicalJournalRollback/execution/v1"));
const ConsensusRoot validators_genesis=root("Nano/Test/PhysicalJournalRollback/validators/v1");
const ConsensusBytes4 fork_version{9,0,0,1};
struct Tokens{ForkCheckpointToken primary,witness;};
Json token_json(const ForkCheckpointToken& t){return {{"sequence",t.sequence},{"digest",hex(t.digest)}};}
Json tokens_json(const Tokens& t){return {{"primary",token_json(t.primary)},{"witness",token_json(t.witness)}};}
ForkCheckpointToken token(const Json& j){
 need(j.is_object()&&j.size()==2&&j.contains("sequence")&&j.contains("digest")&&j.at("sequence").is_number_unsigned()&&j.at("digest").is_string(),"committed token shape");
 const auto d=unhex(j.at("digest").get<std::string>());need(d.size()==32&&hex(d)==j.at("digest").get<std::string>()&&j.at("sequence").get<uint64_t>()>0,"canonical committed token");return {j.at("sequence").get<uint64_t>(),d};
}
Tokens tokens(const Json& j){need(j.is_object()&&j.size()==2&&j.contains("primary")&&j.contains("witness"),"committed pair shape");return {token(j.at("primary")),token(j.at("witness"))};}
void directory(const fs::path& p){guard();need(::mkdir(p.c_str(),0700)==0,"new directory only");rd::Directory held(p);rd::sync(held.fd());rd::Fd parent(::open(p.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));rd::sync(parent.n);}
struct Seal{
 rd::Directory directory;std::vector<rd::Entry> entries;
 explicit Seal(const fs::path& p):directory(p),entries(rd::capture(directory.fd(),maximum_entries,maximum_store_bytes,guard)){}
 Seal(const fs::path& p,std::vector<rd::Entry> admitted):directory(p),entries(std::move(admitted)){fence();}
 uint64_t payload_bytes()const{uint64_t n=0;for(const auto& e:entries)if(S_ISREG(e.stamp.st_mode))n+=uint64_t(e.stamp.st_size);return n;}
 void fence()const{directory.recheck();need(rd::same_entries(entries,rd::capture(directory.fd(),maximum_entries,maximum_store_bytes,guard)),"complete stopped tree FD9/SHA unchanged");directory.recheck();}
};
void sync_tree(const Seal& tree){
 // Only called during production, before the immutable check boundary.
 for(auto i=tree.entries.rbegin();i!=tree.entries.rend();++i){guard();if(i->name.empty()){rd::sync(tree.directory.fd());continue;}auto f=rd::relative_fd(tree.directory.fd(),i->name,(S_ISDIR(i->stamp.st_mode)?O_RDONLY|O_DIRECTORY:O_RDWR)|O_NONBLOCK);rd::sync(f.n);}
 tree.fence();
}
// Quiet comparisons are confined to unadmitted NEW inodes; a Seal still uses
// the original strict full-FD9/SHA helper, including ctime, without relaxation.
bool eight(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const bool mtime=a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec&&a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec;
#else
 const bool mtime=a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec;
#endif
 return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_nlink==b.st_nlink&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_size==b.st_size&&mtime;
}
bool ctime_equal(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 return a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec&&a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
bool created_directory(const struct stat& born,const struct stat& current){return born.st_dev==current.st_dev&&born.st_ino==current.st_ino&&born.st_mode==current.st_mode&&born.st_uid==current.st_uid&&born.st_gid==current.st_gid&&S_ISDIR(current.st_mode);}
std::unique_ptr<Seal> admit_copy(const Seal& source,const rd::Directory& out,const std::vector<rd::Entry>& created){
 phase="admit-new-cold-copy";const auto began=std::chrono::steady_clock::now();auto since=began;std::vector<rd::Entry> first,previous;
 for(;;){
  guard();out.recheck();source.fence();++admission_source_fences;auto sample=rd::capture(out.fd(),maximum_entries,maximum_store_bytes,guard);++admission_samples;source.fence();++admission_source_fences;
  need(sample.size()==created.size()&&sample.size()==source.entries.size(),"new-copy exact creation/source entry counts");
  for(size_t i=0;i<sample.size();++i){
   const auto& now=sample[i];const auto& born=created[i];const auto& original=source.entries[i];need(now.name==born.name&&now.name==original.name&&S_ISDIR(now.stamp.st_mode)==S_ISDIR(original.stamp.st_mode)&&now.sha==born.sha&&now.sha==original.sha,"new-copy fixed names/types/known SHA");
   if(S_ISDIR(now.stamp.st_mode)){need(created_directory(born.stamp,now.stamp),"new directory creation identity unchanged");if(!first.empty())need(eight(first[i].stamp,now.stamp),"completed new directory eight fields changed");}
   else need(eight(born.stamp,now.stamp),"new file creation eight fields changed");
  }
  if(first.empty())first=sample;
  bool equal=!previous.empty();for(size_t i=0;i<previous.size();++i){const bool unchanged=ctime_equal(previous[i].stamp,sample[i].stamp);equal=equal&&unchanged;if(!unchanged)++admission_ctime_changes;}
  const auto now=std::chrono::steady_clock::now();admission_elapsed_ms=uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(now-began).count());guard();need(now-began<std::chrono::seconds(30),"new-copy admission 30 second deadline");
  if(equal)++admission_streak;else{admission_streak=1;since=now;}
  // Adopt the exact last capture. The adopting constructor fences immediately;
  // it never replaces these entries with a fresh post-admission baseline.
  if(admission_streak>=3&&now-since>=std::chrono::seconds(2)){need(rd::same_contents(source.entries,sample),"complete admitted independent-inode P/W copy");return std::make_unique<Seal>(out.path,std::move(sample));}
  previous=std::move(sample);std::this_thread::sleep_for(std::chrono::milliseconds(1050));
 }
}
std::unique_ptr<Seal> cold_copy(const Seal& source,const fs::path& target){
 need(source.payload_bytes()<=maximum_store_bytes/2&&source.entries.size()<=maximum_entries/2,"initial pair fits complete two-pair copy budget");rd::Directory out(target);std::vector<rd::Entry> created{{"",out.stamps.back(),{}}};
 for(const auto& e:source.entries){
  if(e.name.empty())continue;guard();auto parent=rd::relative_parent(out.fd(),e.name);const auto name=fs::path(e.name).filename().string();
  if(S_ISDIR(e.stamp.st_mode)){need(::mkdirat(parent.n,name.c_str(),0700)==0,"exclusive physical copy directory");auto fd=rd::relative_fd(out.fd(),e.name,O_RDONLY|O_DIRECTORY);struct stat born{},linked{};need(::fstat(fd.n,&born)==0&&::fstatat(parent.n,name.c_str(),&linked,AT_SYMLINK_NOFOLLOW)==0&&created_directory(born,linked),"new directory creation FD/path identity");created.push_back({e.name,born,{}});continue;}
  auto from=rd::relative_fd(source.directory.fd(),e.name,O_RDONLY|O_NONBLOCK);struct stat before{},after{};need(::fstat(from.n,&before)==0&&rd::stable(e.stamp,before)&&S_ISREG(before.st_mode)&&before.st_nlink==1,"held original single-link file before copy");
  rd::Fd to(::openat(parent.n,name.c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));rd::Hash digest;std::array<unsigned char,65536> buffer{};uint64_t at=0;
  while(at<uint64_t(before.st_size)){
   guard();const auto n=::pread(from.n,buffer.data(),size_t(std::min<uint64_t>(buffer.size(),uint64_t(before.st_size)-at)),off_t(at));if(n<0&&errno==EINTR)continue;need(n>0,"bounded complete physical source read");digest.add(buffer.data(),size_t(n));size_t written=0;
   while(written<size_t(n)){guard();const auto m=::write(to.n,buffer.data()+written,size_t(n)-written);if(m<0&&errno==EINTR)continue;need(m>0,"complete exclusive physical copy write");written+=size_t(m);}at+=uint64_t(n);
  }
  need(::fstat(from.n,&after)==0&&rd::stable(before,after)&&digest.finish()==e.sha,"original bytes/FD9 unchanged during copy");rd::sync(to.n);struct stat copied{};need(::fstat(to.n,&copied)==0&&copied.st_size==before.st_size&&S_ISREG(copied.st_mode)&&(copied.st_mode&0777)==0600&&copied.st_uid==::geteuid()&&copied.st_nlink==1&&rd::file_hash(to.n,copied,guard)==e.sha,"durable copied file SHA/creation identity");created.push_back({e.name,copied,e.sha});
 }
 // Finish every durability operation before the first admission observation.
 for(auto i=created.rbegin();i!=created.rend();++i)if(S_ISDIR(i->stamp.st_mode)){guard();if(i->name.empty())rd::sync(out.fd());else{auto fd=rd::relative_fd(out.fd(),i->name,O_RDONLY|O_DIRECTORY);rd::sync(fd.n);}}
 source.fence();return admit_copy(source,out,created);
}
// A fresh public test key exists only during production, never during checking.
struct TestKey{
 blst_scalar secret{};Bytes public_key;
 TestKey(){uint64_t words[4]{4200001,0,0,0};blst_scalar_from_uint64(&secret,words);OPENSSL_cleanse(words,sizeof(words));blst_p1 point;blst_sk_to_pk_in_g1(&point,&secret);public_key.resize(48);blst_p1_compress(public_key.data(),&point);OPENSSL_cleanse(&point,sizeof(point));}
 TestKey(const TestKey&)=delete;TestKey& operator=(const TestKey&)=delete;
 ~TestKey(){OPENSSL_cleanse(&secret,sizeof(secret));}
 Bytes sign(const ConsensusRoot& r){
  guard();need(signatures==0,"one public test signature only");++signatures;
  static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 hash,signed_point;blst_hash_to_g2(&hash,r.data(),r.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&signed_point,&hash,&secret);Bytes out(96);blst_p2_compress(out.data(),&signed_point);OPENSSL_cleanse(&hash,sizeof(hash));OPENSSL_cleanse(&signed_point,sizeof(signed_point));return out;
 }
};
Tokens retained(ChainStore& p,ChainStore& w,const ForkCheckpointToken& expected){
 const auto a=p.fork_checkpoint(chain_id,execution_genesis,4096,expected),b=w.fork_checkpoint(chain_id,execution_genesis,16384);need(a&&b,"both actual durable journal records present");return {a->token,b->token};
}
struct Produced{Bytes public_key;Tokens old,recent;ConsensusRoot signing_root;std::unique_ptr<Seal> old_seal;};
Produced produce(const fs::path& current,const fs::path& old){
 phase="produce-initial-pair";TestKey key;Tokens initial,recent;std::unique_ptr<Seal> old_seal;
 {
  ChainStore p(current/"primary"),w(current/"witness");++signer_constructions;BeaconSigningJournal journal(p,chain_id,execution_genesis,validators_genesis,key.public_key,slots_per_epoch,{},&w);initial=retained(p,w,journal.commitment());need(initial.primary.sequence==1&&initial.witness.sequence==1&&provider_calls==0&&signatures==0,"actual empty pair at sequence one, zero signing");
 }
 // Every DB/signing handle is gone before either complete physical copy.
 phase="cold-copy-initial-pair";
 {
  Seal source(current);sync_tree(source);old_seal=cold_copy(source,old);source.fence();
 }
 ConsensusBeaconHeader header{proposal_slot,0,{},{},{}};header.parent_root=root("Nano/Test/PhysicalJournalRollback/parent/v1");header.state_root=root("Nano/Test/PhysicalJournalRollback/state/v1");header.body_root=root("Nano/Test/PhysicalJournalRollback/body/v1");
 const auto signing_root=consensus_signing_root(consensus_beacon_header_root(header),consensus_domain({0,0,0,0},fork_version,validators_genesis));
 phase="reopen-and-sign-one-public-proposal";
 {
  ChainStore p(current/"primary"),w(current/"witness");++signer_constructions;BeaconSigningJournal journal(p,chain_id,execution_genesis,validators_genesis,key.public_key,slots_per_epoch,initial.primary,&w);
  const auto sig=journal.sign_proposal(header,fork_version,[&](const ConsensusRoot& r){guard();need(++provider_calls==1&&r==signing_root,"one exact proposal root reached public provider");return key.sign(r);});need(verify_consensus_beacon_header(header,fork_version,validators_genesis,key.public_key,sig),"actual BLS proposal signature verifies");recent=retained(p,w,journal.commitment());
 }
 need(recent.primary.sequence==initial.primary.sequence+1&&recent.witness.sequence==initial.witness.sequence+1&&recent.primary.digest!=initial.primary.digest&&recent.witness.digest!=initial.witness.digest,"both real journal commitments advanced exactly once");
 old_seal->fence();need(provider_calls==1&&signatures==1&&signer_constructions==2,"exact producer counts");return {key.public_key,initial,recent,signing_root,std::move(old_seal)};
}
struct Commitment{
 rd::Directory parent;std::optional<rd::Fd> file;struct stat stamp{};std::string expected,sha;
 Commitment(const fs::path& p,const Json& body):parent(p),expected(body.dump()){
  need(!expected.empty()&&expected.size()<=16384,"small independent commitment");
  {
   rd::Fd output(::openat(parent.fd(),"commitments.json",O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));size_t n=0;
   while(n<expected.size()){guard();const auto z=::write(output.n,expected.data()+n,expected.size()-n);if(z<0&&errno==EINTR)continue;need(z>0,"write independent commitment");n+=size_t(z);}
   need(::fchmod(output.n,0400)==0,"readonly external commitment mode");rd::sync(output.n);need(::fstat(output.n,&stamp)==0,"capture independent commitment identity");sha=rd::file_hash(output.n,stamp,guard);
  }
  rd::sync(parent.fd());file.emplace(::openat(parent.fd(),"commitments.json",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));(void)read();
 }
 Json read()const{
  guard();parent.recheck();struct stat a{},linked{};need(::fstat(file->n,&a)==0&&::fstatat(parent.fd(),"commitments.json",&linked,AT_SYMLINK_NOFOLLOW)==0&&rd::stable(stamp,a)&&rd::stable(stamp,linked)&&S_ISREG(a.st_mode)&&a.st_uid==::geteuid()&&(a.st_mode&0777)==0400&&a.st_nlink==1&&a.st_size==off_t(expected.size()),"external commitment exact FD9/path");
  std::string raw(expected.size(),'\0');size_t n=0;while(n<raw.size()){guard();const auto z=::pread(file->n,raw.data()+n,raw.size()-n,off_t(n));if(z<0&&errno==EINTR)continue;need(z>0,"read complete external commitment");n+=size_t(z);}
  need(raw==expected&&rd::file_hash(file->n,stamp,guard)==sha,"external commitment known bytes/SHA unchanged");const auto j=Json::parse(raw);need(j.dump()==raw,"canonical external commitment JSON");parent.recheck();return j;
 }
};
struct Records{ForkCheckpointRecord primary,witness;};
Records read_pair(const fs::path& tree,const Tokens& expected){
 phase="readonly-cold-pair";guard();ChainStore p(tree/"primary",ChainStore::OpenMode::ReadOnly),w(tree/"witness",ChainStore::OpenMode::ReadOnly);readonly_opens+=2;
 p.require_existing_chain_binding(chain_id,execution_genesis,"nano-beacon-signing-v1");w.require_existing_chain_binding(chain_id,execution_genesis,"nano-beacon-signing-witness-v1");
 const auto a=p.fork_checkpoint(chain_id,execution_genesis,4096,expected.primary),b=w.fork_checkpoint(chain_id,execution_genesis,16384,expected.witness);need(a&&b&&a->token==expected.primary&&b->token==expected.witness,"actual readonly records equal independent current pins");return {*a,*b};
}
BeaconJournalPairInput input(const Records& r,const Tokens& floors){return {r.primary,r.witness,r.primary.token,r.witness.token,floors.primary,floors.witness};}
AuthenticatedBeaconJournalPair authenticate(const Records& r,const Tokens& floors,const BeaconJournalIdentity& id){guard();++auth_calls;return authenticate_beacon_signing_journal_pair(input(r,floors),id,guard);}
void positive(const Records& records,const Tokens& floors,const BeaconJournalIdentity& id,bool has_proposal,const ConsensusRoot& signing_root){
 const auto pair=authenticate(records,floors,id);need(pair.disposition()==BeaconJournalPairDisposition::Completed&&pair.primary_floor().supplied&&pair.witness_floor().supplied&&pair.primary_floor().current_equals_floor&&pair.witness_floor().current_equals_floor,"completed physical pair with exact independent floors");
 for(const auto& r:{pair.primary(),pair.candidate()})need(!r.retired&&!r.attestation&&bool(r.proposal)==has_proposal&&(!r.proposal||(r.proposal->slot==proposal_slot&&r.proposal->root==std::optional<ConsensusRoot>(signing_root))),"real old/current reservations match signed transition");
 need(!pair.historical_digest_ancestry_verified()&&!pair.delivered_signature_history_verified(),"numeric floors do not authenticate ancestry/delivery");++positives;
}
void rejected(const Records& old,const Tokens& floors,const BeaconJournalIdentity& id){
 bool refused=false;try{(void)authenticate(old,floors,id);}catch(const std::invalid_argument& e){need(std::string_view(e.what())=="Invalid beacon signing journal","exact Core journal refusal class/reason");refused=true;}
 need(refused,"Core accepted an actual old pair below recent floor");++refusals;
}
Json run(const fs::path& path){
 phase="create-owned-root";need(path.is_absolute()&&path==path.lexically_normal()&&path!=path.root_path()&&fs::canonical(path.parent_path())==path.parent_path(),"canonical existing physical parent and new absolute root");directory(path);directory(path/"current");directory(path/"old");
 const auto made=produce(path/"current",path/"old");
 // TestKey, provider closure, all signer instances and RW stores are destroyed.
 phase="seal-stopped-pairs";const Seal& old=*made.old_seal;Seal current(path/"current");sync_tree(current);old.fence();need(old.entries.size()+current.entries.size()<=maximum_entries&&old.payload_bytes()+current.payload_bytes()<=maximum_store_bytes,"aggregate four-store 32 MiB/4096 entry budget");
 phase="commit-independent-recent-floors";
 const Json authority={{"format","nano-real-physical-journal-rollback-commitments-v1"},{"chainId",uint64_t(1337)},{"executionGenesis",hex(execution_genesis)},{"genesisValidatorsRoot",hex(Bytes(validators_genesis.begin(),validators_genesis.end()))},{"publicKey",hex(made.public_key)},{"slotsPerEpoch",slots_per_epoch},{"proposalSlot",proposal_slot},{"proposalSigningRoot",hex(Bytes(made.signing_root.begin(),made.signing_root.end()))},{"old",tokens_json(made.old)},{"recent",tokens_json(made.recent)},{"newTestOnlySignaturesCreated",uint64_t(1)},{"runtimeDutiesAuthorized",false},{"productionQualified",false}};
 Commitment committed(path,authority);const auto providers_before=provider_calls,signatures_before=signatures,constructors_before=signer_constructions;const BeaconJournalIdentity identity{made.public_key,validators_genesis,slots_per_epoch};
 for(unsigned pass=0;pass<2;++pass){
  phase="check-external-commitment";const auto held=committed.read();need(held==authority,"independent retained authority remains exact");const auto old_floors=tokens(held.at("old")),recent_floors=tokens(held.at("recent"));
  const auto before=read_pair(path/"old",old_floors),now=read_pair(path/"current",recent_floors);old.fence();current.fence();
  phase="check-valid-old-and-recent";positive(before,old_floors,identity,false,made.signing_root);positive(now,recent_floors,identity,true,made.signing_root);
  if(pass==0){
   need(before.primary.token.sequence<recent_floors.primary.sequence&&before.witness.token.sequence<recent_floors.witness.sequence,"real old tokens are not substituted with floors");
   phase="reject-recent-primary-floor";rejected(before,{recent_floors.primary,old_floors.witness},identity);
   phase="reject-recent-witness-floor";rejected(before,{old_floors.primary,recent_floors.witness},identity);
   phase="reject-both-recent-floors";rejected(before,recent_floors,identity);
  }
  old.fence();current.fence();(void)committed.read();need(provider_calls==providers_before&&signatures==signatures_before&&signer_constructions==constructors_before,"readonly check never invokes or constructs a signer/provider");
 }
 phase="final-fences";old.fence();current.fence();(void)committed.read();guard();need(provider_calls==1&&signatures==1&&signer_constructions==2&&readonly_opens==8&&auth_calls==7&&positives==4&&refusals==3,"exact complete physical rollback test counts");
 return {{"scenario","beacon-journal-real-physical-rollback"},{"result","PASS"},{"physicalPairs",2},{"databases",4},{"independentReadonlyPasses",2},{"readonlyDatabaseOpens",readonly_opens},{"CoreAuthentications",auth_calls},{"positiveAuthentications",positives},{"oldPairRecentPrimaryFloorRefused",true},{"oldPairRecentWitnessFloorRefused",true},{"oldPairBothRecentFloorsRefused",true},{"expectedCoreRefusals",refusals},{"newTestOnlySignaturesCreated",signatures},{"signingProviderCalls",provider_calls},{"signerConstructions",signer_constructions},{"newSignaturesInCheck",uint64_t(0)},{"signersConstructedInCheck",uint64_t(0)},{"fourClosedStoresFD9ShaUnchanged",true},{"completeIndependentPhysicalCopy",true},{"externalCommitmentOutsideBothTrees",true},{"externalCommitmentSha256",committed.sha},{"externalCommitmentBytes",committed.expected.size()},{"externalCommitmentFD9ShaUnchanged",true},{"maximumStoreBytes",maximum_store_bytes},{"storeBytes",old.payload_bytes()+current.payload_bytes()},{"maximumEntries",maximum_entries},{"storeEntries",old.entries.size()+current.entries.size()},{"maximumRssKiB",maximum_rss_kib},{"actualPeakRssKiB",rss()},{"maximumWallSeconds",180},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count()},{"independentExternalRollbackProtectionQualified",false},{"historicalDigestAncestryVerified",false},{"deliveredSignatureHistoryVerified",false},{"automaticRecoveryUsed",false},{"operatorRecoveryQualified",false},{"privateInputsRead",false},{"runtimeDutiesAuthorized",false},{"networkTransport",false},{"commonFinalityQualified",false},{"productionQualified",false}};
}
}
int main(int argc,char** argv){
 ::umask(0077);std::signal(SIGINT,stop);std::signal(SIGTERM,stop);std::signal(SIGHUP,stop);
 try{need(argc==2,"usage: nano-beacon-journal-physical-rollback RUN_ABSENT_ROOT");const auto result=run(fs::path(argv[1]));std::cout<<result.dump()<<std::endl;need(bool(std::cout),"public result output");return 0;}
 catch(const std::exception& e){std::string reason=e.what();if(reason.size()>512)reason.resize(512);std::cerr<<Json{{"diagnostic","beacon-journal-real-physical-rollback-refused-v1"},{"result","FAIL"},{"phase",phase},{"reason",reason},{"newTestOnlySignaturesCreated",signatures},{"signingProviderCalls",provider_calls},{"readonlyDatabaseOpens",readonly_opens},{"CoreAuthentications",auth_calls},{"expectedCoreRefusals",refusals},{"newCopyAdmissionSamples",admission_samples},{"newCopyAdmissionStableSamples",admission_streak},{"newCopyAdmissionElapsedMs",admission_elapsed_ms},{"newCopyPreAdmissionCtimeChanges",admission_ctime_changes},{"newCopyAdmissionSourceFences",admission_source_fences},{"productionQualified",false}}.dump()<<std::endl;return 1;}
}
