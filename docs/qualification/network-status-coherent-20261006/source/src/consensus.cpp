#include "nano/consensus.hpp"
#include <limits>
#include <condition_variable>
namespace nano {
Bytes ProofOfWork::hash(const Header& h){
 Bytes serialized;
 for(const auto& b:{h.parent_hash,h.uncle_hash,unhex(address(h.coinbase)),h.state_root,h.transactions_root,h.receipts_root,h.logs_bloom,bytes(h.difficulty,32),bytes(h.number,8),bytes(h.gas_limit,8),bytes(h.gas_used,8),bytes(h.timestamp,8),h.extra_data})serialized=concat(std::move(serialized),b);
 if(h.nonce.size()!=8)throw std::invalid_argument("invalid PoW nonce");
 auto mix=concat(keccak(serialized),h.nonce);for(unsigned i=0;i<64;++i)mix=keccak(mix);return mix;
}
bool ProofOfWork::verify(const Header& h){return integer(hash(h))<(Int(1)<<256)/std::max(h.difficulty,Int(1));}
Int ProofOfWork::adjust_difficulty(const Header& parent,const Int& timestamp){Int elapsed=timestamp-parent.timestamp,adjustment=parent.difficulty/2048+1,difficulty=parent.difficulty;if(elapsed<12)difficulty+=adjustment;else if(elapsed>24)difficulty-=adjustment;return std::max(Int(1),std::min(difficulty,Int((Int(1)<<256)-1)));}
std::optional<MiningResult> ProofOfWork::mine(Header h,std::chrono::milliseconds timeout,uint64_t nonce,std::stop_token cancel){
 stopped_=false;hashrate_=0;auto start=std::chrono::steady_clock::now();uint64_t attempts=0;Int target=(Int(1)<<256)/std::max(h.difficulty,Int(1));
 while(!stopped_&&!cancel.stop_requested()){auto elapsed=std::chrono::steady_clock::now()-start;if(timeout.count()>0&&elapsed>=timeout)return std::nullopt;h.nonce=bytes(Int(std::to_string(nonce)),8);auto mix=hash(h);++attempts;if(attempts%1000==0){auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();if(seconds>0)hashrate_=attempts/seconds;}if(integer(mix)<target){double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();hashrate_=seconds>0?attempts/seconds:0;return MiningResult{h.nonce,mix,hashrate_.load(),attempts};}if(nonce==std::numeric_limits<uint64_t>::max())return std::nullopt;++nonce;
 }return std::nullopt;
}
void Miner::start(std::function<std::optional<Block>()> get_block,std::function<void(const Block&)> submit_block){
 if(!get_block||!submit_block)throw std::invalid_argument("Mining callbacks are required");
 if(running_)throw std::logic_error("Miner is already running");
 if(worker_.joinable())worker_.join();
 {std::lock_guard lock(error_mutex_);error_.clear();}
 running_=true;
 try{worker_=std::jthread([this,get_block=std::move(get_block),submit_block=std::move(submit_block)](std::stop_token cancel){
  try{
   while(!cancel.stop_requested()){
    auto block=get_block();if(cancel.stop_requested())break;
    if(!block){std::mutex mutex;std::unique_lock lock(mutex);std::condition_variable_any ready;ready.wait_for(lock,cancel,std::chrono::seconds(1),[]{return false;});continue;}
    auto result=pow_.mine(block->header,std::chrono::seconds(30),0,cancel);
    if(result&&!cancel.stop_requested()){block->header.nonce=result->nonce;block->header.mix_hash=result->mix_hash;submit_block(*block);}
   }
  }catch(const std::exception& e){std::lock_guard lock(error_mutex_);error_=e.what();}
   catch(...){std::lock_guard lock(error_mutex_);error_="Unknown mining callback failure";}
  running_=false;
 });}catch(...){running_=false;throw;}
}
void Miner::stop(){
 if(worker_.joinable()){
  worker_.request_stop();pow_.stop();
  if(worker_.get_id()!=std::this_thread::get_id())worker_.join();
 }
}
HeaderValidation validate_pow_header(const Header& h,const std::optional<Header>& parent,const Int& now){
 try{h.encode();}catch(const std::exception& e){return {false,e.what()};}
 if(!parent)return h.number==0?HeaderValidation{true,{}}:HeaderValidation{false,"Genesis must be block 0"};
 if(h.number!=parent->number+1)return {false,"Invalid block number"};
 if(h.parent_hash!=parent->hash())return {false,"Invalid parent hash"};
 if(h.timestamp<=parent->timestamp)return {false,"Timestamp must be greater than parent"};
 if(h.timestamp>now+15)return {false,"Block is from the future"};
 if(abs(h.gas_limit-parent->gas_limit)>=parent->gas_limit/1024)return {false,"Gas limit change too large"};
 if(h.gas_limit<5000)return {false,"Gas limit too low"};
 if(h.gas_used>h.gas_limit)return {false,"Gas used exceeds gas limit"};
 Int expected=ProofOfWork::adjust_difficulty(*parent,h.timestamp);if(abs(h.difficulty-expected)>expected/10)return {false,"Invalid difficulty"};
 if(!ProofOfWork::verify(h))return {false,"Invalid proof of work"};
 return {true,{}};
}
}
