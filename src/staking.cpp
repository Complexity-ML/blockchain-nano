#include "nano/staking.hpp"
#include <set>
#include <map>
namespace nano {
namespace {
Bytes staking_call(const State& state,const Header& header,const std::string& target,const Bytes& data,const Int& chain_id,const std::function<Bytes(const Int&)>& block_hash,uint64_t& remaining,EvmRevision revision,uint64_t per_call_gas){
  if(!remaining)throw std::invalid_argument("staking query gas budget exhausted");ExecutionContext ctx;ctx.revision=revision;
  ctx.address=target;ctx.code=state.get(target).code;ctx.data=data;ctx.is_static=true;ctx.gas=std::min(per_call_gas,remaining);ctx.chain_id=chain_id;ctx.number=header.number;ctx.timestamp=header.timestamp;ctx.coinbase=header.coinbase;ctx.prevrandao=block_randomness(header,revision);ctx.gas_limit=header.gas_limit;ctx.base_fee=header.base_fee.value_or(Int(0));if(revision>=EvmRevision::Cancun)ctx.blob_base_fee=blob_base_fee(header.excess_blob_gas.value_or(Int(0)),revision);ctx.block_hash=block_hash;
  auto result=EVM(state).execute(ctx);remaining-=result.gas_used;
  if(!result.success)throw std::invalid_argument("staking contract call failed");return result.return_data;
}
}
Int staking_contract_stake(const State& state,const Header& header,const std::string& contract,const std::string& account,const Int& chain_id,std::function<Bytes(const Int&)> block_hash,EvmRevision revision,StakingQueryLimits limits){
 limits.validate();
 auto target=address(contract);auto argument=bytes(integer(unhex(address(account))),32);
 if(state.get(target).code.empty())return 0;
 uint64_t remaining=limits.per_call_gas;auto result=staking_call(state,header,target,concat(unhex("7a766460"),argument),chain_id,block_hash,remaining,revision,limits.per_call_gas);
 if(result.size()!=32)throw std::runtime_error("invalid staking scalar result");return integer(result);
}
ProofOfStake sync_staking_contract(const State& state,const Header& header,const std::string& contract,const ProofOfStake& previous,const Int& chain_id,std::function<Bytes(const Int&)> block_hash,EvmRevision revision,StakingQueryLimits limits){
 limits.validate();
 if(contract.empty())return previous;
 auto target=address(contract);auto snapshot=previous.to_json();snapshot["validators"]=Json::array();
 if(state.get(target).code.empty()){snapshot["bootstrap"]=true;return ProofOfStake::from_json(snapshot);}
 snapshot["bootstrap"]=false;uint64_t remaining=limits.sync_gas;
 auto call=[&](const Bytes& data){return staking_call(state,header,target,data,chain_id,block_hash,remaining,revision,limits.per_call_gas);};
 auto word_at=[](const Bytes& data,size_t offset){if(offset>data.size()||data.size()-offset<32)throw std::invalid_argument("truncated staking validator array");return integer(Bytes(data.begin()+offset,data.begin()+offset+32));};
 std::set<std::string> seen;std::map<std::string,Json> records;
 auto prior_snapshot=previous.to_json();for(const auto& prior:prior_snapshot.at("validators"))records.emplace(address(prior.at("address")),prior);
 auto append=[&](const Int& encoded,const Int& stake,const Int& active_word){
  if(encoded>=(Int(1)<<160)||encoded==0||active_word>1)throw std::invalid_argument("invalid staking validator entry");
  auto addr=hex(bytes(encoded,20));if(!seen.insert(addr).second)throw std::invalid_argument("duplicate staking validator");
  bool active=active_word==1&&stake>=ProofOfStake::minimum_stake;
  Json entry={{"address",addr},{"stake",quantity(stake)},{"registeredAt",quantity(header.number)},{"lockedUntil",quantity(header.number)},{"lastBlock","0x0"},{"blocksValidated","0x0"},{"active",active}};
  if(auto prior=records.find(addr);prior!=records.end()){entry=prior->second;entry["stake"]=quantity(stake);entry["active"]=active;entry["address"]=addr;}
  snapshot["validators"].push_back(std::move(entry));
 };
 if(limits.page_size){
  auto total_data=call(unhex("7071688a"));if(total_data.size()!=32)throw std::invalid_argument("invalid staking validator count");
  auto total=u64(integer(total_data));
  for(uint64_t start=0;start<total;){
   auto count=std::min(limits.page_size,total-start);
   auto page=call(concat(unhex("0d69c0c6"),concat(bytes(Int(std::to_string(start)),32),bytes(Int(std::to_string(count)),32))));
   if(page.size()<64||word_at(page,0)!=32||(page.size()-64)%96||word_at(page,32)!=Int(std::to_string(count))||(page.size()-64)/96!=count)throw std::invalid_argument("invalid staking validator page");
   for(size_t i=0;i<count;++i)append(word_at(page,64+i*96),word_at(page,96+i*96),word_at(page,128+i*96));
   start+=count;
  }
 }else{
  auto list=call(unhex("b7ab4db5"));
  if(list.size()<64||word_at(list,0)!=32)throw std::invalid_argument("invalid staking validator array offset");
  auto count=u64(word_at(list,32));if((list.size()-64)%32!=0||count!=(list.size()-64)/32)throw std::invalid_argument("invalid staking validator array size");
  for(size_t index=0;index<count;++index){
   auto encoded=word_at(list,64+index*32);if(encoded>=(Int(1)<<160)||encoded==0)throw std::invalid_argument("invalid staking validator address");
   auto argument=bytes(encoded,32);auto stake_data=call(concat(unhex("7a766460"),argument));auto active_data=call(concat(unhex("facd743b"),argument));
   if(stake_data.size()!=32||active_data.size()!=32)throw std::invalid_argument("invalid staking scalar result");
   append(encoded,integer(stake_data),integer(active_data));
  }
 }
 return ProofOfStake::from_json(snapshot);
}
}
