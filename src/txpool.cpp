#include "nano/txpool.hpp"
namespace nano {
Int TxPool::price(const Transaction& tx){return tx.type==2?tx.max_fee:tx.gas_price;}
TxPool::TxPool(size_t capacity,size_t per_account,Int minimum):max_size_(capacity),max_per_account_(per_account),minimum_price_(std::move(minimum)){if(!capacity||!per_account||minimum_price_<0)throw std::invalid_argument("invalid pool limits");}
bool TxPool::add(const SignedTransaction& tx){
 std::string sender,hash;try{sender=address(tx.sender());hash=hex(tx.hash());}catch(const std::exception&){return false;}
 if(tx.tx.nonce<0||tx.tx.gas<tx.tx.intrinsic_gas()||price(tx.tx)<minimum_price_)return false;
 std::lock_guard lock(mutex_);if(known_.contains(hash))return false;
 auto replace=entries_.end();size_t account_count=0;
 for(auto it=entries_.begin();it!=entries_.end();++it)if(it->second.sender==sender){++account_count;if(it->second.tx.tx.nonce==tx.tx.nonce)replace=it;}
 if(replace!=entries_.end()){if(price(tx.tx)*10<price(replace->second.tx.tx)*11)return false;}
 else if(account_count>=max_per_account_)return false;
 if(replace!=entries_.end())entries_.erase(replace);
 else if(entries_.size()>=max_size_){auto lowest=std::min_element(entries_.begin(),entries_.end(),[](const auto& a,const auto& b){Int pa=price(a.second.tx.tx),pb=price(b.second.tx.tx);return pa==pb?a.second.arrival<b.second.arrival:pa<pb;});entries_.erase(lowest);}
 entries_.emplace(hash,Entry{tx,sender,arrival_++});known_.insert(hash);return true;
}
std::optional<SignedTransaction> TxPool::get(const std::string& hash)const{auto normalized=hex(unhex(hash));std::lock_guard lock(mutex_);auto it=entries_.find(normalized);return it==entries_.end()?std::nullopt:std::optional<SignedTransaction>(it->second.tx);}
bool TxPool::remove(const std::string& hash){auto normalized=hex(unhex(hash));std::lock_guard lock(mutex_);return entries_.erase(normalized)>0;}
void TxPool::remove_mined(const std::vector<SignedTransaction>& txs){std::lock_guard lock(mutex_);for(const auto& tx:txs)entries_.erase(hex(tx.hash()));}
std::vector<SignedTransaction> TxPool::pending_for_block(const Int& gas_limit)const{if(gas_limit<0)throw std::invalid_argument("negative gas limit");std::lock_guard lock(mutex_);std::vector<const Entry*> ordered;for(const auto& [h,e]:entries_)ordered.push_back(&e);std::sort(ordered.begin(),ordered.end(),[](const Entry* a,const Entry* b){Int pa=price(a->tx.tx),pb=price(b->tx.tx);return pa==pb?a->arrival<b->arrival:pa>pb;});Int used=0;std::vector<SignedTransaction> out;for(const auto* entry:ordered)if(used+entry->tx.tx.gas<=gas_limit){out.push_back(entry->tx);used+=entry->tx.tx.gas;}return out;}
std::vector<SignedTransaction> TxPool::by_sender(const std::string& addr)const{auto normalized=address(addr);std::lock_guard lock(mutex_);std::vector<SignedTransaction> out;for(const auto& [h,e]:entries_)if(e.sender==normalized)out.push_back(e.tx);std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.tx.nonce<b.tx.nonce;});return out;}
Int TxPool::pending_nonce(const std::string& addr)const{auto list=by_sender(addr);return list.empty()?Int(0):Int(list.back().tx.nonce+1);}
size_t TxPool::size()const{std::lock_guard lock(mutex_);return entries_.size();}
Json TxPool::content()const{std::lock_guard lock(mutex_);Json pending=Json::object();for(const auto& [h,e]:entries_)pending[e.sender][e.tx.tx.nonce.get_str()]=e.tx.to_json();return {{"pending",pending},{"queued",Json::object()}};}
Json TxPool::status()const{return {{"pending",size()},{"queued",0}};}
void TxPool::clear(){std::lock_guard lock(mutex_);entries_.clear();known_.clear();arrival_=0;}
}
