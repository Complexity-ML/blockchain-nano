#include "nano/engine.hpp"
namespace nano {
Bytes execution_requests_hash(const std::vector<Bytes>& requests){
 Bytes hashes;int previous=-1;
 for(const auto& request:requests){
  if(request.empty()||int(request[0])<=previous)throw std::invalid_argument("execution requests not in type order");
  previous=request[0];if(request.size()==1)continue;
  auto hash=sha256(request);hashes.insert(hashes.end(),hash.begin(),hash.end());
 }
 return sha256(hashes);
}
Bytes deposit_requests(const std::vector<Log>& logs){
 static const auto topic=unhex("649bbc62d0e31342afea4e5cd82d4049e7e1ee912fc0889aa790803be39038c5");
 Bytes requests{0};
 for(const auto& log:logs){
  if(log.address!="0x00000000219ab540356cbb839cbe05303d7705fa"||log.topics.empty()||log.topics[0]!=topic)continue;
  const auto& data=log.data;
  if(data.size()!=576)throw std::invalid_argument("invalid deposit event layout");
  constexpr size_t offsets[]={160,256,320,384,512},sizes[]={48,32,8,96,8};
  for(size_t i=0;i<5;++i){
   auto word=[&](size_t at){return integer(Bytes(data.begin()+at,data.begin()+at+32));};
   if(word(i*32)!=Int(offsets[i])||word(offsets[i])!=Int(sizes[i]))throw std::invalid_argument("invalid deposit event layout");
   requests.insert(requests.end(),data.begin()+offsets[i]+32,data.begin()+offsets[i]+32+sizes[i]);
  }
 }
 return requests;
}
Bytes ExecutionEngine::system_call(State& state,const Header& h,const std::string& target,const Bytes& data,bool required)const{
 ExecutionContext ctx;ctx.checkpoint=checkpoint_;ctx.revision=revision_;ctx.origin=ctx.caller="0xfffffffffffffffffffffffffffffffffffffffe";ctx.address=target;
 ctx.code=static_cast<const State&>(state).get(target).code;
 if(ctx.code.empty()){if(required)throw std::invalid_argument("required system contract is empty");return {};}
 ctx.data=data;ctx.gas=30000000;ctx.timestamp=h.timestamp;ctx.number=h.number;ctx.coinbase=h.coinbase;
 ctx.prevrandao=block_randomness(h,revision_);ctx.gas_limit=h.gas_limit;ctx.chain_id=chain_id_;
 ctx.base_fee=h.base_fee.value_or(Int(0));ctx.blob_base_fee=blob_base_fee(h.excess_blob_gas.value_or(Int(0)),revision_);ctx.block_hash=block_hash_;
 auto result=EVM(state).execute(ctx);
 if(!result.success&&required)throw std::invalid_argument("required system contract call failed");
 return result.success?result.return_data:Bytes{};
}
}
