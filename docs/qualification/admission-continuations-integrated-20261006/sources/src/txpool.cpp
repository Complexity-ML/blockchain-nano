#include "nano/txpool.hpp"
#include <memory>
namespace nano {
Int TxPool::price(const Transaction& tx){return tx.type==2?tx.max_fee:tx.gas_price;}
TxPool::TxPool(size_t capacity,size_t per_account,Int minimum,size_t max_bytes):max_size_(capacity),max_per_account_(per_account),max_bytes_(max_bytes),minimum_price_(std::move(minimum)){if(!capacity||!per_account||!max_bytes||minimum_price_<0)throw std::invalid_argument("invalid pool limits");}
bool TxPool::add(const SignedTransaction& tx){
 std::string sender,hash;size_t raw_size;try{raw_size=tx.raw().size();if(raw_size>max_bytes_)return false;sender=address(tx.sender());hash=hex(tx.hash());}catch(const std::exception&){return false;}
 if(tx.tx.nonce<0||tx.tx.gas<tx.tx.intrinsic_gas()||price(tx.tx)<minimum_price_)return false;
 std::lock_guard lock(mutex_);if(entries_.contains(hash)||mined_.contains(hash))return false;
 auto replace=entries_.end();size_t account_count=0;
 for(auto it=entries_.begin();it!=entries_.end();++it)if(it->second.sender==sender){++account_count;if(it->second.tx.tx.nonce==tx.tx.nonce)replace=it;}
 if(replace!=entries_.end()){
  const auto& old=replace->second.tx.tx;Int fee=price(tx.tx),old_fee=price(old);
  Int tip=tx.tx.type==2?tx.tx.priority_fee:tx.tx.gas_price,old_tip=old.type==2?old.priority_fee:old.gas_price;
  if(fee<=old_fee||tip<=old_tip||fee<old_fee*110/100||tip<old_tip*110/100)return false;
 }
 else if(account_count>=max_per_account_)return false;
 std::vector<decltype(entries_.begin())> victims;
 size_t remaining_count=entries_.size(),remaining_bytes=bytes_;
 if(replace!=entries_.end()){victims.push_back(replace);--remaining_count;remaining_bytes-=replace->second.raw_bytes;}
 auto full=[&]{return remaining_count>=max_size_||remaining_bytes>max_bytes_-raw_size;};
 if(full()){
  std::vector<decltype(entries_.begin())> ordered;for(auto it=entries_.begin();it!=entries_.end();++it)if(it!=replace)ordered.push_back(it);
  std::sort(ordered.begin(),ordered.end(),[](auto a,auto b){Int pa=price(a->second.tx.tx),pb=price(b->second.tx.tx);return pa==pb?a->second.arrival<b->second.arrival:pa<pb;});
  for(auto it:ordered){if(!full())break;if(price(tx.tx)<price(it->second.tx.tx))return false;victims.push_back(it);--remaining_count;remaining_bytes-=it->second.raw_bytes;}
 }
 if(full())return false;
 // Plan all removals first. A rejected candidate must not partially evict the
 // pool. Allocate its map entry before deleting existing entries as well.
 auto [inserted,added]=entries_.emplace(hash,Entry{tx,sender,arrival_,raw_size});if(!added)return false;
 for(auto it:victims){bytes_-=it->second.raw_bytes;entries_.erase(it);}
 bytes_+=raw_size;++arrival_;return true;
}
std::optional<SignedTransaction> TxPool::get(const std::string& hash)const{auto normalized=hex(unhex(hash));std::lock_guard lock(mutex_);auto it=entries_.find(normalized);return it==entries_.end()?std::nullopt:std::optional<SignedTransaction>(it->second.tx);}
bool TxPool::remove(const std::string& hash){auto normalized=hex(unhex(hash));std::lock_guard lock(mutex_);auto it=entries_.find(normalized);if(it==entries_.end())return false;bytes_-=it->second.raw_bytes;entries_.erase(it);return true;}
void TxPool::remove_stale(const State& state){
 std::lock_guard lock(mutex_);
 for(auto it=entries_.begin();it!=entries_.end();){
  const auto& tx=it->second.tx.tx;const auto& account=state.get(it->second.sender);
  if(tx.nonce<account.nonce||tx.value+tx.gas*price(tx)>account.balance){bytes_-=it->second.raw_bytes;it=entries_.erase(it);}
  else ++it;
 }
}
void TxPool::remove_mined(const std::vector<SignedTransaction>& txs){
 std::lock_guard lock(mutex_);for(const auto& tx:txs){auto hash=hex(tx.hash());auto it=entries_.find(hash);if(it!=entries_.end()){bytes_-=it->second.raw_bytes;entries_.erase(it);}
  if(mined_.insert(hash).second){mined_order_.push_back(std::move(hash));if(mined_order_.size()>max_size_){mined_.erase(mined_order_.front());mined_order_.pop_front();}}
 }
}
std::vector<TxPool::Candidate> TxPool::candidates_for_block(const Int& gas_limit)const{if(gas_limit<0)throw std::invalid_argument("negative gas limit");std::lock_guard lock(mutex_);std::vector<const Entry*> ordered;for(const auto& [h,e]:entries_)ordered.push_back(&e);std::sort(ordered.begin(),ordered.end(),[](const Entry* a,const Entry* b){Int pa=price(a->tx.tx),pb=price(b->tx.tx);return pa==pb?a->arrival<b->arrival:pa>pb;});Int used=0;std::vector<Candidate> out;for(const auto* entry:ordered)if(used+entry->tx.tx.gas<=gas_limit){out.push_back({entry->tx,entry->sender});used+=entry->tx.tx.gas;}return out;}
std::vector<SignedTransaction> TxPool::pending_for_block(const Int& gas_limit)const{auto candidates=candidates_for_block(gas_limit);std::vector<SignedTransaction> out;out.reserve(candidates.size());for(auto& candidate:candidates)out.push_back(std::move(candidate.transaction));return out;}
std::vector<SignedTransaction> TxPool::ready_for_block(const State& state,const Int& gas_limit,size_t maximum,const std::function<void()>& checkpoint)const{
 if(gas_limit<0||!maximum)throw std::invalid_argument("invalid ready transaction budget");
 std::lock_guard lock(mutex_);
 std::map<std::string,std::deque<const Entry*>> accounts;
 for(const auto& [hash,entry]:entries_){if(checkpoint)checkpoint();if(entry.tx.tx.nonce>=state.get(entry.sender).nonce)accounts[entry.sender].push_back(&entry);}
 auto priority=[](const Entry* a,const Entry* b){const auto pa=price(a->tx.tx),pb=price(b->tx.tx);if(pa!=pb)return pa>pb;if(a->arrival!=b->arrival)return a->arrival<b->arrival;return a->sender<b->sender;};
 std::set<const Entry*,decltype(priority)> ready(priority);
 for(auto& [sender,entries]:accounts){
  if(checkpoint)checkpoint();std::sort(entries.begin(),entries.end(),[](const Entry* a,const Entry* b){return a->tx.tx.nonce<b->tx.tx.nonce;});
  if(entries.front()->tx.tx.nonce==state.get(sender).nonce)ready.insert(entries.front());
 }
 Int used=0;std::vector<SignedTransaction> result;
 while(!ready.empty()&&result.size()<maximum){
  if(checkpoint)checkpoint();const auto* entry=*ready.begin();ready.erase(ready.begin());
  // An account cannot advance past a head transaction that does not fit.
  if(entry->tx.tx.gas>gas_limit-used)continue;
  result.push_back(entry->tx);used+=entry->tx.tx.gas;
  auto& account=accounts.at(entry->sender);account.pop_front();
  if(!account.empty()&&account.front()->tx.tx.nonce==entry->tx.tx.nonce+1)ready.insert(account.front());
 }
 return result;
}
std::vector<SignedTransaction> TxPool::by_sender(const std::string& addr)const{auto normalized=address(addr);std::lock_guard lock(mutex_);std::vector<SignedTransaction> out;for(const auto& [h,e]:entries_)if(e.sender==normalized)out.push_back(e.tx);std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.tx.nonce<b.tx.nonce;});return out;}
Int TxPool::reserved_cost(const std::string& sender,const Int& replacing_nonce,const Int& canonical_nonce)const{
 auto normalized=address(sender);std::lock_guard lock(mutex_);Int total=0,next=canonical_nonce;std::vector<const Entry*> ordered;
 for(const auto& [hash,entry]:entries_)if(entry.sender==normalized)ordered.push_back(&entry);
 std::sort(ordered.begin(),ordered.end(),[](const Entry* a,const Entry* b){return a->tx.tx.nonce<b->tx.tx.nonce;});
 for(const auto* entry:ordered){const auto& tx=entry->tx.tx;if(tx.nonce<next)continue;if(tx.nonce!=next)break;++next;if(tx.nonce!=replacing_nonce)total+=tx.value+tx.gas*price(tx);}
 return total;
}
Int TxPool::pending_nonce(const std::string& addr)const{auto list=by_sender(addr);return list.empty()?Int(0):Int(list.back().tx.nonce+1);}
size_t TxPool::size()const{std::lock_guard lock(mutex_);return entries_.size();}
size_t TxPool::serialized_bytes()const{std::lock_guard lock(mutex_);return bytes_;}
Json TxPool::content()const{std::lock_guard lock(mutex_);Json pending=Json::object();for(const auto& [h,e]:entries_)pending[e.sender][e.tx.tx.nonce.get_str()]=e.tx.to_json();return {{"pending",pending},{"queued",Json::object()}};}
Json TxPool::classified(bool content,const State& state)const{
 std::lock_guard lock(mutex_);std::map<std::string,std::vector<const Entry*>> accounts;
 for(const auto& [hash,entry]:entries_)accounts[entry.sender].push_back(&entry);
 Json result={{"pending",Json::object()},{"queued",Json::object()}};size_t pending=0,queued=0;
 for(auto& [sender,entries]:accounts){
  std::sort(entries.begin(),entries.end(),[](const Entry* a,const Entry* b){return a->tx.tx.nonce<b->tx.tx.nonce;});Int next=state.get(sender).nonce;
  for(const auto* entry:entries){const auto& nonce=entry->tx.tx.nonce;bool ready=nonce==next;if(ready){++next;++pending;}else ++queued;
   if(content)result[ready?"pending":"queued"][sender][nonce.get_str()]=entry->tx.to_json();
  }
 }
 return content?result:Json{{"pending",pending},{"queued",queued}};
}
Json TxPool::page(const std::string& after)const{
 if(!after.empty()&&(after.size()!=66||hex(unhex(after))!=after))throw std::invalid_argument("invalid inventory cursor");
 std::lock_guard lock(mutex_);auto it=after.empty()?entries_.begin():entries_.upper_bound(after);Json transactions=Json::array();size_t size=0;std::string last;
 for(;it!=entries_.end();++it){auto raw=hex(it->second.tx.raw());if(raw.size()>8*1024*1024)throw std::invalid_argument("inventory entry exceeds page size");if(transactions.size()>=50||size+raw.size()>8*1024*1024)break;size+=raw.size();transactions.push_back(std::move(raw));last=it->first;}
 return {{"transactions",transactions},{"next",it==entries_.end()?Json(nullptr):Json(last)}};
}
Json TxPool::status()const{return {{"pending",size()},{"queued",0}};}
void TxPool::clear(){std::lock_guard lock(mutex_);entries_.clear();mined_.clear();mined_order_.clear();bytes_=0;arrival_=0;}
}

namespace nano {
std::function<void()> TxPool::prepare_canonical_change(const State& state,const std::vector<SignedTransaction>& included,const std::vector<SignedTransaction>& orphaned){
 struct Prepared {std::unique_lock<std::mutex> lock;std::unique_ptr<TxPool> candidate;bool published=false;explicit Prepared(TxPool& p):lock(p.mutex_),candidate(std::make_unique<TxPool>(p.max_size_,p.max_per_account_,p.minimum_price_,p.max_bytes_)) {}};
 auto prepared=std::make_shared<Prepared>(*this);auto& next=*prepared->candidate;
 next.mined_=mined_;next.mined_order_=mined_order_;
 for(const auto& tx:orphaned){const auto hash=hex(tx.hash());next.mined_.erase(hash);std::erase(next.mined_order_,hash);}
 // Preserve authenticated entry metadata and arrival order without repeating
 // signature recovery for every pending transaction on each new block.
 next.entries_=entries_;next.bytes_=bytes_;next.arrival_=arrival_;
 // Free canonical/stale entries before restoring orphaned transactions, so
 // already-mined transactions cannot evict useful pending entries at capacity.
 next.remove_mined(included);next.remove_stale(state);
 for(const auto& tx:orphaned){
  const auto sender=address(tx.sender());const auto& account=state.get(sender);
  if(tx.tx.nonce<account.nonce||tx.tx.value+tx.tx.gas*price(tx.tx)>account.balance)continue;
  next.add(tx);
 }
 next.remove_stale(state);
 return [this,prepared]()noexcept{if(prepared->published)return;auto& next=*prepared->candidate;entries_.swap(next.entries_);mined_.swap(next.mined_);mined_order_.swap(next.mined_order_);bytes_=next.bytes_;arrival_=next.arrival_;prepared->published=true;};
}
}
