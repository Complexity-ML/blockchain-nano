#include "nano/consensus_capella_cold_records.hpp"
#include <algorithm>
#include <chrono>
namespace nano {
namespace {
void require(bool ok){if(!ok)throw std::invalid_argument("Invalid Capella cold record");}
uint64_t number(const Json& j){require(j.is_string());const auto n=u64(quantity(j));require(quantity(Int(std::to_string(n)))==j.get<std::string>());return n;}
ConsensusRoot root(const Json& j){require(j.is_string());const auto text=j.get<std::string>();const auto bytes=unhex(text);require(bytes.size()==32&&hex(bytes)==text);ConsensusRoot out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;}
CapellaColdRecord decode(const CapellaColdReference& reference,std::string_view payload,const CapellaForkBlockRules& rules,const CapellaStoreSnapshotLimits& limits,const std::function<void()>& check){
 const auto row=Json::parse(payload);require(row.is_array()&&row.size()==6&&row.dump()==payload&&root(row[0])==reference.root);
 const auto& h=row[1];require(h.is_array()&&h.size()==5);ConsensusBeaconHeader header{number(h[0]),number(h[1]),root(h[2]),root(h[3]),root(h[4])};require(consensus_beacon_header_root(header)==reference.root);
 const auto& state_row=row[2];require(state_row.is_array()&&state_row.size()==2&&state_row[0].is_string());const auto& encoded=state_row[0].get_ref<const std::string&>();require(encoded.size()>=18&&encoded.size()%2==0&&(encoded.size()-2)/2<=limits.state_bytes&&(encoded.size()-2)/2<=limits.total_state_bytes);
 CapellaSlotState state{unhex(encoded),std::nullopt};require(hex(state.ssz)==encoded);if(!state_row[1].is_null())state.last_processed_epoch=number(state_row[1]);
 const auto& execution=row[5];require(execution.is_array()&&execution.size()==(rules.deposit_contract_code_hash.empty()?3:4)&&execution[2].is_array());
 auto block=Block::from_json(execution[0]);auto accounts=State::from_json(execution[1]);std::vector<Receipt> receipts;for(const auto& receipt:execution[2]){if(check)check();receipts.push_back(Receipt::from_json(receipt));}
 std::shared_ptr<const ExecutionDepositHistory> deposits;if(!rules.deposit_contract_code_hash.empty())deposits=std::make_shared<const ExecutionDepositHistory>(ExecutionDepositHistory::from_snapshot(execution[3],block.header,accounts,block.header.hash(),rules.deposit_contract_code_hash,UINT32_MAX,check));
 CapellaColdRecord out{reference,{header,std::move(state),std::move(block),std::move(accounts),std::move(receipts),std::move(deposits)},{header.slot,header.parent_root,number(row[3]),number(row[4])}};
 require(capella_beacon_state_root(out.block.state.ssz,rules.transition.consensus.state,rules.transition.execution,rules.transition.maximum_state_bytes,check)==header.state_root);
 validate_capella_execution_record(out.block,rules,check);
 if(!reference.anchor){const CapellaEpochRules epoch{rules.transition.consensus.epoch,rules.transition.execution,rules.transition.maximum_state_bytes};const auto ffg=capella_fork_checkpoints(out.block.state,epoch,rules.transition.consensus.state,check);require(out.node.realized_justified_epoch==ffg.realized.current_justified.epoch&&out.node.unrealized_justified_epoch==ffg.unrealized.current_justified.epoch);}
 if(check)check();return out;
}
}
CapellaColdRecordCache::CapellaColdRecordCache(CapellaForkBlockRules rules,CapellaStoreSnapshotLimits limits,size_t records,size_t bytes):rules_(std::move(rules)),limits_(limits),maximum_records_(records),maximum_encoded_bytes_(bytes){
 if(!records||!bytes||bytes>limits.envelope_bytes)throw std::invalid_argument("Invalid Capella cold cache budget");
}
std::shared_ptr<const CapellaColdRecord> CapellaColdRecordCache::load(const CapellaColdReference& reference,const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,const std::function<void()>& check){
 if(check)check();
 std::unique_lock lock(mutex_,std::defer_lock);
 // Cancellation callbacks run without ownership while waiting. Recursive
 // ownership permits usage() in fetch/check callbacks, never nested loads.
 while(!lock.try_lock_for(std::chrono::milliseconds(10)))if(check)check();
 if(check)check();
 if(loading_)throw std::logic_error("Reentrant Capella cold load");
 struct Loading {bool& flag;explicit Loading(bool& f):flag(f){flag=true;}~Loading(){flag=false;}} loading(loading_);
 require(reference.digest.size()==32&&reference.bytes&&reference.bytes<=limits_.envelope_bytes&&reference.bytes<=maximum_encoded_bytes_);
 if(clock_==UINT64_MAX)throw std::overflow_error("Capella cold cache clock exhausted");const auto key=hex(reference.digest)+(reference.anchor?"/anchor":"/block");
 if(const auto known=entries_.find(key);known!=entries_.end()){require(known->second.record->reference==reference);known->second.used=++clock_;return known->second.record;}
 // Select victims without mutating the cache; pinned records are never victims.
 std::vector<std::pair<uint64_t,std::string>> possible;for(const auto& [id,entry]:entries_)if(entry.record.use_count()==1)possible.emplace_back(entry.used,id);std::sort(possible.begin(),possible.end());
 size_t count=entries_.size(),bytes=encoded_bytes_,victims=0;while(count>=maximum_records_||reference.bytes>maximum_encoded_bytes_-bytes){if(victims==possible.size())throw std::length_error("Capella cold cache records pinned");const auto& entry=entries_.at(possible[victims++].second);--count;bytes-=entry.record->reference.bytes;}
 require(bool(fetch));const auto payload=fetch(reference.digest,reference.bytes);require(payload&&payload->size()==reference.bytes&&sha256(Bytes(payload->begin(),payload->end()))==reference.digest);if(check)check();
 auto record=std::make_shared<const CapellaColdRecord>(decode(reference,*payload,rules_,limits_,check));
 // Allocation may fail, so install before erasing victims. No user callback can
 // observe the transient extra entry; failures leave the prior cache intact.
 entries_.emplace(key,Entry{record,clock_+1});for(size_t i=0;i<victims;++i)entries_.erase(possible[i].second);encoded_bytes_=bytes+reference.bytes;++clock_;return record;
}
}
