#pragma once
#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace nano::test::lifecycle {
namespace canonical_stream_detail {
inline void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class T>Bytes bytes(const T& value){return {value.begin(),value.end()};}
inline uint64_t little(std::span<const uint8_t> b,size_t at,size_t width=8){require(at<=b.size()&&width<=b.size()-at,"stream truncated integer");uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
inline ConsensusRoot root(std::span<const uint8_t> b,size_t at){require(at<=b.size()&&32<=b.size()-at,"stream truncated root");ConsensusRoot out{};std::copy_n(b.begin()+at,32,out.begin());return out;}
inline bool same(const struct stat& a,const struct stat& b){
#ifdef __APPLE__
 const auto am=a.st_mtimespec,bm=b.st_mtimespec,ac=a.st_ctimespec,bc=b.st_ctimespec;
#else
 const auto am=a.st_mtim,bm=b.st_mtim,ac=a.st_ctim,bc=b.st_ctim;
#endif
 return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_nlink==b.st_nlink&&a.st_size==b.st_size&&am.tv_sec==bm.tv_sec&&am.tv_nsec==bm.tv_nsec&&ac.tv_sec==bc.tv_sec&&ac.tv_nsec==bc.tv_nsec;
}
struct Fd{int fd=-1;explicit Fd(int f):fd(f){}Fd(const Fd&)=delete;~Fd(){if(fd>=0)::close(fd);}};
inline constexpr size_t record_bytes=canonical_stream_index_record_bytes;
using Record=std::array<uint8_t,record_bytes>;
inline void put(Record& r,size_t at,uint64_t n){for(size_t i=0;i<8;++i)r[at+i]=uint8_t(n>>(8*i));}
inline Record record(const ConsensusRoot& id,const CheckpointArchiveEntry& e){Record r{};std::copy(id.begin(),id.end(),r.begin());const auto parent=root(e.payload,116);std::copy(parent.begin(),parent.end(),r.begin()+32);put(r,64,e.slot);put(r,72,e.payload.size());const auto hash=sha256(e.payload);std::copy(hash.begin(),hash.end(),r.begin()+80);return r;}
inline Bytes roll(const Bytes& previous,const Record& row){Bytes b=previous;b.insert(b.end(),row.begin(),row.end());return sha256(b);}

// Write only while NEW. One successful seal freezes known ordered bytes/size
// AND a full-nine-field FD/path authority; it is never refreshed or resealed.
// No raw wire enters the index. Caller uses private OS tmp/owned-volume output.
class Index {
 Fd parent_,file_;std::filesystem::path path_;struct stat identity_{},sealed_material_{};bool seal_attempted_=false,sealed_=false;
 uint64_t count_=0,maximum_;Bytes commitment_=Bytes(32,0);
 void identity(const struct stat& s)const{require(S_ISREG(s.st_mode)&&s.st_nlink==1&&s.st_uid==geteuid()&&(s.st_mode&07777)==0600&&s.st_dev==identity_.st_dev&&s.st_ino==identity_.st_ino&&s.st_gid==identity_.st_gid&&s.st_size>=0&&uint64_t(s.st_size)==count_*record_bytes,"stream private index identity/size changed");}
 void sealed_fence()const{require(sealed_&&std::filesystem::canonical(path_.parent_path())==path_.parent_path(),"stream sealed index parent/authority required");struct stat a{},b{};require(::fstat(file_.fd,&a)==0&&::lstat(path_.c_str(),&b)==0&&same(a,b)&&same(a,sealed_material_),"stream sealed index material changed");identity(a);}
public:
 Index(const std::filesystem::path& p,uint64_t maximum):parent_(-1),file_(-1),path_(p),maximum_(maximum){
  require(p.is_absolute()&&p.lexically_normal()==p&&p.filename()!="."&&p.filename()!=".."&&!p.filename().empty(),"stream canonical new index required");
  const auto dir=p.parent_path();require(std::filesystem::canonical(dir)==dir,"stream linked index parent refused");
  parent_.fd=::open(dir.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);require(parent_.fd>=0,"stream private index parent open");struct stat d{},path{};require(::fstat(parent_.fd,&d)==0&&::lstat(dir.c_str(),&path)==0&&same(d,path)&&S_ISDIR(d.st_mode)&&d.st_uid==geteuid()&&(d.st_mode&07777)==0700,"stream owner0700 index parent required");
  file_.fd=::openat(parent_.fd,p.filename().c_str(),O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);require(file_.fd>=0,"stream exclusive index create");
  require(::fstat(file_.fd,&identity_)==0&&::fstatat(parent_.fd,p.filename().c_str(),&path,AT_SYMLINK_NOFOLLOW)==0&&same(identity_,path),"stream private index creation identity");identity(identity_);require(::fsync(parent_.fd)==0,"stream index directory fsync");
 }
 void append(const Record& r,const std::function<void()>& check){
  require(!seal_attempted_,"stream index append after seal refused");require(count_<=maximum_/record_bytes&&record_bytes<=maximum_-count_*record_bytes,"stream private index byte bound");size_t at=0;while(at<r.size()){check();const auto n=::write(file_.fd,r.data()+at,r.size()-at);if(n<0&&errno==EINTR)continue;require(n>0,"stream private index write");at+=size_t(n);}++count_;commitment_=roll(commitment_,r);
 }
 void seal(){require(!seal_attempted_,"stream index reseal refused");seal_attempted_=true;require(::fsync(file_.fd)==0,"stream private index fsync");struct stat before{},after{},path{};require(::fstat(file_.fd,&before)==0&&::lstat(path_.c_str(),&path)==0&&same(before,path),"stream index seal FD/path differs");identity(before);require(::fstat(file_.fd,&after)==0&&::lstat(path_.c_str(),&path)==0&&same(before,after)&&same(after,path),"stream index seal material unstable");sealed_material_=after;sealed_=true;}
 Record read(uint64_t ordinal,const std::function<void()>& check)const{
  check();require(sealed_&&ordinal<count_&&ordinal<=uint64_t(std::numeric_limits<off_t>::max())/record_bytes,"stream sealed index read bound");require(std::filesystem::canonical(path_.parent_path())==path_.parent_path(),"stream linked index parent refused");struct stat before{},after{},path{};require(::fstat(file_.fd,&before)==0&&::lstat(path_.c_str(),&path)==0&&same(before,path)&&same(before,sealed_material_),"stream sealed index material changed");identity(before);
  Record r{};size_t at=0;while(at<r.size()){check();const auto n=::pread(file_.fd,r.data()+at,r.size()-at,off_t(ordinal*record_bytes+at));if(n<0&&errno==EINTR)continue;require(n>0,"stream index short read");at+=size_t(n);}
  require(::fstat(file_.fd,&after)==0&&::lstat(path_.c_str(),&path)==0&&same(before,after)&&same(after,path)&&same(after,sealed_material_),"stream sealed index changed during FD read");return r;
 }
 void verify(const std::function<void()>& check)const{check();sealed_fence();Bytes digest(32,0);for(uint64_t i=0;i<count_;++i)digest=roll(digest,read(i,check));require(digest==commitment_,"stream known private index bytes changed");check();sealed_fence();}
 uint64_t count()const{return count_;}
};
inline ConsensusBeaconHeader header(const CheckpointArchiveEntry& e,const ConsensusRoot& id,const CapellaForkBlockRules& r,size_t maximum,const std::function<void()>& check){
 const auto& t=r.transition;require(e.root==bytes(id)&&e.payload.size()>=404&&e.payload.size()<=maximum&&little(e.payload,0,4)==100&&little(e.payload,180,4)==84&&little(e.payload,100)==e.slot,"stream archive root/slot/envelope differs");
 ConsensusBeaconHeader h{e.slot,little(e.payload,108),root(e.payload,116),root(e.payload,148),capella_beacon_body_root(std::span<const uint8_t>(e.payload).subspan(184),t.consensus.body,t.consensus.state.sync_committee_size,t.execution,t.withdrawals.maximum_per_payload,t.maximum_bls_changes,t.consensus.maximum_block_bytes,check)};
 require(consensus_beacon_header_root(h)==id,"stream archive signed header root differs");return h;
}
inline size_t live_bytes(const CapellaForkStore& s,size_t maximum,const std::function<void()>& check){size_t total=0;for(const auto& [_,entry]:s.blocks){check();const auto p=entry.lease(check);require(p->state.ssz.size()<=maximum-total,"stream live state byte bound");total+=p->state.ssz.size();}return total;}
}

inline CanonicalStreamResult stream_capella_canonical_history(const CapellaForkStore& trusted,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& snapshot,const CanonicalStreamLimits& bound,const CanonicalStreamBoundary& boundary,const CanonicalStreamSource& source,const std::filesystem::path& index_path,const CanonicalStreamVisitor& visit,const std::function<void()>& check){
 using namespace canonical_stream_detail;
 require(bool(check)&&bool(source.archive)&&bool(source.verify_execution)&&bool(visit),"stream explicit source/visitor/cancellation required");check();
 const auto& t=rules.transition;const auto slots=t.consensus.epoch.ffg.slots_per_epoch;
 require(slots&&rules.slot_duration_ms&&snapshot.blocks&&snapshot.total_state_bytes&&bound.maximum_live_blocks>=2&&bound.maximum_live_blocks<=256&&bound.maximum_live_blocks<=snapshot.blocks&&bound.maximum_live_state_bytes>0&&bound.maximum_live_state_bytes<=snapshot.total_state_bytes&&t.maximum_state_bytes<=snapshot.state_bytes&&t.maximum_state_bytes<=bound.maximum_live_state_bytes&&bound.maximum_total_blocks>0&&bound.maximum_total_archive_bytes>0&&bound.maximum_private_index_bytes>=record_bytes,"stream configured bounds refused");
 require(bound.maximum_total_blocks<=bound.maximum_private_index_bytes/record_bytes&&bound.maximum_private_index_bytes<=uint64_t(std::numeric_limits<off_t>::max()),"stream total/index bounds differ");
 require(trusted.blocks.size()==1&&trusted.blocks.contains(trusted.timed.view.anchor)&&capella_network_anchor(trusted)==trusted.timed.view.anchor,"stream single original trusted anchor required");
 const auto input_state_bytes=trusted.blocks.at(trusted.timed.view.anchor).lease(check)->state.ssz.size();require(input_state_bytes<=bound.maximum_live_state_bytes&&input_state_bytes<=bound.maximum_live_state_bytes-input_state_bytes,"stream input plus validated anchor byte bound");
 auto replay=[&]{const auto encoded=capella_store_snapshot(trusted,rules,snapshot,check);return capella_store_from_snapshot(encoded,sha256(Bytes(encoded.begin(),encoded.end())),rules,snapshot,check);}();
 const auto anchor=replay.timed.view.anchor,head=consensus_beacon_header_root(boundary.stopped_head);const auto anchor_header=replay.blocks.at(anchor).header();
 require(boundary.stopped_head.slot>=anchor_header.slot&&boundary.required_finalized.epoch<=boundary.stopped_finalized.epoch&&boundary.stopped_finalized.epoch<=boundary.stopped_head.slot/slots,"stream checkpoint/head bounds refused");
 std::array<ConsensusCheckpoint,2> checkpoints{boundary.required_finalized,boundary.stopped_finalized};std::array<bool,2> seen{};for(const auto& cp:checkpoints)require(cp.epoch<=UINT64_MAX/slots,"stream checkpoint slot overflow");
 auto bind_checkpoints=[&](const ConsensusBeaconHeader& h,const ConsensusRoot& id){for(size_t i=0;i<2;++i)if(!seen[i]&&h.slot<=checkpoints[i].epoch*slots){require(id==checkpoints[i].root,"stream external finalized boundary differs");seen[i]=true;}};
 Index index(index_path,bound.maximum_private_index_bytes);CanonicalStreamResult result;auto tip=head;uint64_t child=UINT64_MAX;
 while(tip!=anchor){
  check();require(index.count()<bound.maximum_total_blocks,"stream total block bound");const auto e=source.archive(tip,t.consensus.maximum_block_bytes,check);require(bool(e),"stream canonical signed archive missing");const auto h=header(*e,tip,rules,t.consensus.maximum_block_bytes,check);
  require(h.slot<child&&h.slot>anchor_header.slot&&h.slot<=boundary.stopped_head.slot,"stream ancestry does not descend");require(e->payload.size()<=bound.maximum_total_archive_bytes-result.archive_bytes,"stream total archive byte bound");result.archive_bytes+=e->payload.size();bind_checkpoints(h,tip);index.append(record(tip,*e),check);child=h.slot;tip=h.parent_root;
 }
 bind_checkpoints(anchor_header,anchor);for(size_t i=0;i<2;++i)if(!seen[i]&&checkpoints[i].epoch==trusted.timed.view.finalized.epoch&&checkpoints[i].root==anchor)seen[i]=true;
 require(seen[0]&&seen[1],"stream external finalized ancestor outside anchor");index.seal();index.verify(check);result.index_bytes=index.count()*record_bytes;
 auto measure=[&]{require(replay.blocks.size()<bound.maximum_live_blocks,"stream live record bound");const auto n=live_bytes(replay,bound.maximum_live_state_bytes-input_state_bytes,check)+input_state_bytes;result.peak_live_blocks=std::max(result.peak_live_blocks,replay.blocks.size()+1);result.peak_live_state_ssz_bytes=std::max(result.peak_live_state_ssz_bytes,n);return n;};
 auto prune=[&]{const auto& v=replay.timed.view;const auto lookahead=t.consensus.attestation.duties.min_seed_lookahead;const auto at=v.finalized.root;if(at==v.anchor||lookahead>=v.current_epoch)return;const auto retained_epoch=v.current_epoch-lookahead-1;require(retained_epoch<=UINT64_MAX/slots,"stream pruning epoch overflow");if(!replay.blocks.contains(at)||replay.blocks.at(at).header().slot>retained_epoch*slots)return;auto next=prepare_capella_pruned_store(replay,at,rules,check);require(next.blocks.size()<=replay.blocks.size()&&capella_network_anchor(next)==anchor,"stream pruning identity differs");replay=std::move(next);++result.prunes;};
 {const auto first=replay.blocks.at(anchor).lease(check);source.verify_execution(*first,check);visit({nullptr,*first,{},anchor,0},check);}measure();
 for(uint64_t ordinal=0;ordinal<index.count();++ordinal){
  check();prune();const auto n=measure();require(replay.blocks.size()+1<bound.maximum_live_blocks&&t.maximum_state_bytes<=bound.maximum_live_state_bytes-n,"stream finalized window unavailable before next state");result.peak_live_state_ssz_plus_next_reserve_bytes=std::max(result.peak_live_state_ssz_plus_next_reserve_bytes,n+t.maximum_state_bytes);
  const auto row=index.read(index.count()-ordinal-1,check);const auto id=root(row,0);const auto e=source.archive(id,t.consensus.maximum_block_bytes,check);require(e&&e->payload.size()>=404&&e->payload.size()<=t.consensus.maximum_block_bytes&&record(id,*e)==row,"stream archive changed since ancestry indexing");const auto h=header(*e,id,rules,t.consensus.maximum_block_bytes,check);require(replay.blocks.contains(h.parent_root),"stream replay parent unavailable");
  require(h.slot<=(UINT64_MAX-replay.timed.clock.genesis_time_ms)/rules.slot_duration_ms,"stream replay clock overflow");replay=prepare_capella_resumed_store_tick(replay,replay.timed.clock.genesis_time_ms+h.slot*rules.slot_duration_ms,rules,snapshot.replay_slots,snapshot.sync_trials,check);
  auto next=prepare_capella_resumed_fork_block(replay,e->payload,rules,snapshot.replay_slots,snapshot.sync_trials,check);require(next&&next->blocks.contains(id)&&next->blocks.size()==replay.blocks.size()+1,"stream canonical signed transition rejected");
  {const auto parent=replay.blocks.at(h.parent_root).lease(check),post=next->blocks.at(id).lease(check);require(post->header==h,"stream admitted signed header differs");source.verify_execution(*post,check);visit({parent.get(),*post,e->payload,id,ordinal+1},check);}
  replay=std::move(*next);++result.verified_signed_blocks;measure();
 }
 index.verify(check);require(replay.blocks.contains(head),"stream stopped head unavailable");result.head=replay.blocks.at(head).lease(check);require(result.head->header==boundary.stopped_head,"stream stopped head differs from authenticated replay");result.required_finalized_ancestor_verified=true;check();return result;
}
}
