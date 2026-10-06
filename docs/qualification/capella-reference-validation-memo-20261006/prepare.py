from pathlib import Path
import hashlib,json
root=Path('/private/tmp/nano-reference-memo-20261006');source=root/'source'
p=source/'include/nano/consensus_capella_store_snapshot.hpp';s=p.read_text();s=s.replace('class CapellaStoreSnapshotCache {','class CapellaReferenceSnapshotCache;\nclass CapellaStoreSnapshotCache {',1)
friend=''' friend std::string capella_store_snapshot_reference_memo_impl(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*,
  const std::function<void(const CapellaColdReference&,std::string_view)>*,CapellaReferenceSnapshotCache*);
'''
s=s.replace(' Json profile_;',friend+' Json profile_;',1)
marker='// Canonical LOCAL checkpoint of an already authenticated Store'
new='''// Process-local row-validation certificates. Owner-serialized, never persisted
// or supplied by peers. No heavy SSZ/accounts retained; encoded metadata budget
// is distinct from physical RSS. Cached API commits memo changes only on success.
struct CapellaReferenceSnapshotCacheUsage {size_t records,encoded_bytes;uint64_t reuses,validated_cold_rows;};
class CapellaReferenceSnapshotCache {
 struct Entry {CapellaColdReference reference;Json metadata;size_t state_bytes;uint64_t genesis_seconds;size_t encoded_bytes;};
 Json profile_;std::map<ConsensusRoot,Entry> records_;size_t encoded_bytes_=0;uint64_t reuses_=0,validated_cold_rows_=0;
 friend std::string capella_store_snapshot_reference_memo_impl(const CapellaForkStore&,const CapellaForkBlockRules&,
  const CapellaStoreSnapshotLimits&,const std::function<void()>&,CapellaStoreSnapshotCache*,
  const std::function<void(const CapellaColdReference&,std::string_view)>*,CapellaReferenceSnapshotCache*);
public:
 CapellaReferenceSnapshotCacheUsage usage()const{return {records_.size(),encoded_bytes_,reuses_,validated_cold_rows_};}
};
'''+marker
assert s.count(marker)==1;s=s.replace(marker,new)
marker='// Authenticate the reference manifest before fetch'
new='''// Optional LOCAL writer memo: cold digest/length/root, complete retained headers,
// node epochs, justification, anchor role and profile must match a row previously
// validated by this writer. Hot rows always undergo full validation. This does
// not verify current disk availability or replace durable recovery preflight.
std::string capella_store_reference_snapshot_cached(const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const std::function<void(const CapellaColdReference&,std::string_view)>&,
 CapellaReferenceSnapshotCache&,const std::function<void()>& checkpoint={});
'''+marker
s=s.replace(marker,new,1);p.write_text(s)
p=source/'src/consensus_capella_store_snapshot.cpp';s=p.read_text();old='''std::string capella_store_snapshot_impl(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& cancel,CapellaStoreSnapshotCache* cache,
 const std::function<void(const CapellaColdReference&,std::string_view)>* stage){'''
new=old.replace('capella_store_snapshot_impl','capella_store_snapshot_reference_memo_impl').replace('>* stage){','>* stage,CapellaReferenceSnapshotCache* memo){');assert s.count(old)==1;s=s.replace(old,new)
s=s.replace(' require(!stage||(!cache&&bool(*stage)));',' require(!stage||(!cache&&bool(*stage)));require(!memo||stage);',1)
marker=' if(cache&&cache->profile_!=current_profile)'
new=''' if(memo){
  if(memo->profile_!=current_profile||memo->encoded_bytes_>l.envelope_bytes){memo->records_.clear();memo->encoded_bytes_=0;memo->profile_=current_profile;}
  for(auto it=memo->records_.begin();it!=memo->records_.end();)if(!s.blocks.contains(it->first)){memo->encoded_bytes_-=it->second.encoded_bytes;it=memo->records_.erase(it);}else ++it;
 }
 const auto metadata=[&](const ConsensusRoot& id,const CapellaForkRecord& entry){const auto& h=entry.header();const auto& node=v.nodes.at(id);return Json::array({Json::array({num(h.slot),num(h.proposer_index),root(h.parent_root),root(h.state_root),root(h.body_root)}),entry.execution_header().to_json(),num(node.realized_justified_epoch),num(node.unrealized_justified_epoch),cp(s.unrealized_justifications.at(id)),id==v.anchor});};
'''+marker
assert s.count(marker)==1;s=s.replace(marker,new)
marker=' for(const auto& [id,entry]:s.blocks){const auto b_pin=entry.lease(cancel);const auto& b=*b_pin;'
new=''' for(const auto& [id,entry]:s.blocks){
  if(cancel)cancel();
  if(memo&&entry.cold_reference()){
   const auto known=memo->records_.find(id);const auto& reference=*entry.cold_reference();
   if(known!=memo->records_.end()&&known->second.reference==reference&&known->second.metadata==metadata(id,entry)){
    const auto& certified=known->second;const auto& node=v.nodes.at(id);
    require(reference.root==id&&reference.bytes&&reference.bytes<=l.envelope_bytes&&certified.state_bytes>=8&&certified.state_bytes<=l.state_bytes);
    require(consensus_beacon_header_root(entry.header())==id&&node.slot==entry.header().slot&&node.parent==entry.header().parent_root);
    require(certified.genesis_seconds<=UINT64_MAX/1000&&certified.genesis_seconds*1000==c.genesis_time_ms&&entry.header().slot<=(c.time_ms-c.genesis_time_ms)/r.slot_duration_ms);
    if(memo->reuses_==UINT64_MAX)throw std::overflow_error("Capella reference memo counter exhausted");++memo->reuses_;
    blocks.push_back(Json::array({root(id),hex(reference.digest),num(reference.bytes)}));continue;
   }
  }
  const auto b_pin=entry.lease(cancel);const auto& b=*b_pin;'''
assert s.count(marker)==1;s=s.replace(marker,new)
marker='''    const auto& committed=*entry.cold_reference();require(committed.root==id&&committed.digest==digest&&committed.bytes==payload.size());'''
new=marker+'''
    if(memo){
     auto proof_metadata=metadata(id,entry);const auto charge=Json::array({root(id),hex(digest),num(payload.size()),proof_metadata,num(b.state.ssz.size()),num(genesis_seconds)}).dump().size();
     if(memo->validated_cold_rows_==UINT64_MAX)throw std::overflow_error("Capella reference memo counter exhausted");++memo->validated_cold_rows_;
     if(const auto old=memo->records_.find(id);old!=memo->records_.end()){memo->encoded_bytes_-=old->second.encoded_bytes;memo->records_.erase(old);}
     if(charge<=l.envelope_bytes){
      while(charge>l.envelope_bytes-memo->encoded_bytes_){auto victim=memo->records_.begin();memo->encoded_bytes_-=victim->second.encoded_bytes;memo->records_.erase(victim);}
      memo->records_.emplace(id,CapellaReferenceSnapshotCache::Entry{committed,std::move(proof_metadata),b.state.ssz.size(),genesis_seconds,charge});memo->encoded_bytes_+=charge;
     }
    }'''
assert s.count(marker)==1;s=s.replace(marker,new)
marker='std::string capella_store_snapshot(const CapellaForkStore& s'
new='''std::string capella_store_snapshot_impl(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void()>& check,CapellaStoreSnapshotCache* cache,
 const std::function<void(const CapellaColdReference&,std::string_view)>* stage){
 return capella_store_snapshot_reference_memo_impl(s,r,l,check,cache,stage,nullptr);
}
std::string capella_store_reference_snapshot_cached(const CapellaForkStore& s,const CapellaForkBlockRules& r,
 const CapellaStoreSnapshotLimits& l,const std::function<void(const CapellaColdReference&,std::string_view)>& stage,
 CapellaReferenceSnapshotCache& memo,const std::function<void()>& check){
 if(check)check();auto candidate=memo;
 auto result=capella_store_snapshot_reference_memo_impl(s,r,l,check,nullptr,&stage,&candidate);
 memo=std::move(candidate);return result;
}
'''+marker
assert s.count(marker)==1;s=s.replace(marker,new);p.write_text(s)
m={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()};(root/'source-sha256.json').write_text(json.dumps(m,indent=2)+'\n');print('Prepared writer-only isolated memo; runtime preflight unchanged')
