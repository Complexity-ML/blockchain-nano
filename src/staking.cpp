#include "nano/staking.hpp"
#include <set>
namespace nano {
namespace {
Bytes staking_call(const State& state,const Header& header,const std::string& target,const Bytes& data,const Int& chain_id,const std::function<Bytes(const Int&)>& block_hash,uint64_t& remaining,EvmRevision revision){
  if(!remaining)throw std::invalid_argument("staking query gas budget exhausted");auto copy=state;ExecutionContext ctx;ctx.revision=revision;
  ctx.address=target;ctx.code=state.get(target).code;ctx.data=data;ctx.is_static=true;ctx.gas=std::min(uint64_t(5000000),remaining);ctx.chain_id=chain_id;ctx.number=header.number;ctx.timestamp=header.timestamp;ctx.coinbase=header.coinbase;ctx.prevrandao=header.difficulty;ctx.gas_limit=header.gas_limit;ctx.base_fee=header.base_fee.value_or(Int(0));ctx.block_hash=block_hash;
  auto result=EVM(copy).execute(ctx);remaining-=result.gas_used;
  if(!result.success)throw std::invalid_argument("staking contract call failed");return result.return_data;
}
}
Int staking_contract_stake(const State& state,const Header& header,const std::string& contract,const std::string& account,const Int& chain_id,std::function<Bytes(const Int&)> block_hash,EvmRevision revision){
 auto target=address(contract);auto argument=bytes(integer(unhex(address(account))),32);
 if(state.get(target).code.empty())return 0;
 uint64_t remaining=5000000;auto result=staking_call(state,header,target,concat(unhex("7a766460"),argument),chain_id,block_hash,remaining,revision);
 if(result.size()!=32)throw std::runtime_error("invalid staking scalar result");return integer(result);
}
ProofOfStake sync_staking_contract(const State& state,const Header& header,const std::string& contract,const ProofOfStake& previous,const Int& chain_id,std::function<Bytes(const Int&)> block_hash,EvmRevision revision){
 if(contract.empty())return previous;
 auto target=address(contract);auto snapshot=previous.to_json();snapshot["validators"]=Json::array();
 if(state.get(target).code.empty()){snapshot["bootstrap"]=true;return ProofOfStake::from_json(snapshot);}
 snapshot["bootstrap"]=false;uint64_t remaining=50000000;
 auto call=[&](const Bytes& data){return staking_call(state,header,target,data,chain_id,block_hash,remaining,revision);};
 auto list=call(unhex("b7ab4db5"));
 auto word_at=[&](size_t offset){if(offset>list.size()||list.size()-offset<32)throw std::invalid_argument("truncated staking validator array");return integer(Bytes(list.begin()+offset,list.begin()+offset+32));};
 if(list.size()<64||word_at(0)!=32)throw std::invalid_argument("invalid staking validator array offset");
 auto count=u64(word_at(32));if(count>1000||list.size()!=64+count*32)throw std::invalid_argument("invalid staking validator array size");
 std::set<std::string> seen;auto records=previous.to_json()["validators"];
 for(size_t index=0;index<count;++index){
  Int encoded=word_at(64+index*32);if(encoded>=(Int(1)<<160)||encoded==0)throw std::invalid_argument("invalid staking validator address");auto addr=hex(bytes(encoded,20));if(!seen.insert(addr).second)throw std::invalid_argument("duplicate staking validator");
  auto argument=bytes(encoded,32);auto stake_data=call(concat(unhex("7a766460"),argument));auto active_data=call(concat(unhex("facd743b"),argument));
  if(stake_data.size()!=32||active_data.size()!=32||integer(active_data)>1)throw std::invalid_argument("invalid staking scalar result");
  Int stake=integer(stake_data);bool active=integer(active_data)==1&&stake>=ProofOfStake::minimum_stake;
  Json entry={{"address",addr},{"stake",quantity(stake)},{"registeredAt",quantity(header.number)},{"lockedUntil",quantity(header.number)},{"lastBlock","0x0"},{"blocksValidated","0x0"},{"active",active}};
  for(const auto& prior:records)if(address(prior["address"])==addr){entry=prior;entry["stake"]=quantity(stake);entry["active"]=active;entry["address"]=addr;break;}
  snapshot["validators"].push_back(std::move(entry));
 }
 return ProofOfStake::from_json(snapshot);
}
}
