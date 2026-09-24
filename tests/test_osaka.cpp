#include "nano/engine.hpp"
#include "nano/precompiles.hpp"
#include <iostream>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Bytes modexp_input(const Int& b,const Int& e,const Int& m,const Bytes& body={}){return concat(concat(concat(bytes(b,32),bytes(e,32)),bytes(m,32)),body);}
int main(){try{
 for(unsigned bit=0;bit<=256;++bit){
  Int value=bit==256?Int(0):Int(Int(1)<<bit);State state;ExecutionContext ctx;ctx.address="0x0000000000000000000000000000000000002000";ctx.revision=EvmRevision::Osaka;
  ctx.code=concat(concat(Bytes{0x7f},bytes(value,32)),unhex("1e5f5260205ff3"));
  auto result=EVM(state).execute(ctx);check(result.success&&integer(result.return_data)==(bit==256?256:255-bit)&&result.gas_used==21,"CLZ bit boundary or gas differs");
  ctx.revision=EvmRevision::Prague;check(!EVM(state).execute(ctx).success,"CLZ activated before Osaka");
 }
 auto simple=modexp_input(1,1,1,Bytes{2,3,5});
 for(auto revision:{EvmRevision::Prague,EvmRevision::Osaka}){auto result=precompile(5,simple,1000,revision);check(result.success&&result.return_data==Bytes{3}&&result.gas_used==(revision==EvmRevision::Osaka?500:200),"MODEXP revision pricing differs");}
 auto long_exponent=modexp_input(1,1048577,1);auto legacy=precompile(5,long_exponent,3000000,EvmRevision::Prague);
 check(legacy.success&&legacy.return_data==Bytes{0}&&legacy.gas_used==2796120,"Legacy MODEXP retained arbitrary 1 MiB cap");
 auto bounded=precompile(5,long_exponent,3000000,EvmRevision::Osaka);check(!bounded.success&&bounded.gas_used==3000000,"Osaka MODEXP bound not applied");
 for(unsigned length:{1024u,1025u}){auto result=precompile(5,modexp_input(0,0,length),100000,EvmRevision::Osaka);check(result.success==(length==1024),"MODEXP 1024-byte boundary differs");if(result.success)check(result.return_data==Bytes(1024)&&result.gas_used==32768,"MODEXP boundary result or gas differs");}
 const Int gx("6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",16),gy("4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5",16),order("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551",16);
 auto input=[&](Int h,Int s){return concat(concat(concat(concat(bytes(h,32),bytes(gx,32)),bytes(s,32)),bytes(gx,32)),bytes(gy,32));};
 for(bool high:{false,true}){auto result=precompile(256,input(0,high?Int(order-gx):gx),6900,EvmRevision::Osaka);check(result.success&&integer(result.return_data)==1&&result.gas_used==6900,"P256 valid/high-s signature rejected");}
 for(auto invalid:{Bytes{},Bytes(160),input(order-gx,1)}){auto result=precompile(256,invalid,7000,EvmRevision::Osaka);check(result.success&&result.return_data.empty()&&result.gas_used==6900,"Invalid P256 signature must return empty without reverting");}
 check(!precompile(256,input(0,gx),6899,EvmRevision::Osaka).success,"P256 insufficient gas accepted");
 State state;ExecutionContext ctx;ctx.address="0x0000000000000000000000000000000000000100";ctx.data=input(0,gx);ctx.revision=EvmRevision::Prague;
 auto before=EVM(state).execute(ctx);check(before.success&&before.return_data.empty()&&before.gas_used==0,"P256 activated before Osaka");ctx.revision=EvmRevision::Osaka;check(integer(EVM(state).execute(ctx).return_data)==1,"P256 Osaka dispatch failed");
 auto key=bytes(Int(1),32);Transaction tx;tx.chain_id=1337;tx.to="0x0000000000000000000000000000000000002000";tx.gas_price=10;Header header;header.base_fee=10;
 for(unsigned gas:{16777216u,16777217u})for(auto revision:{EvmRevision::Prague,EvmRevision::Osaka}){
  tx.gas=gas;auto signed_tx=SignedTransaction::sign(tx,key);State fresh;fresh.get(signed_tx.sender()).balance=10000000000;auto initial=fresh.to_json();bool rejected=false;
  try{auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(fresh,signed_tx,header,0,0);check(receipt.success,"Gas cap boundary execution failed");}catch(const std::invalid_argument& e){rejected=std::string(e.what())=="transaction gas limit exceeds Osaka maximum";if(!rejected)throw;}
  check(rejected==(revision==EvmRevision::Osaka&&gas==16777217),"Transaction gas cap activated incorrectly");if(rejected)check(fresh.to_json()==initial,"Gas cap rejection mutated state");
 }
 // EIP-7918 uses a strict reserve comparison, only in Osaka.
 for(unsigned fee:{15u,16u,17u}){
  Header parent;parent.base_fee=fee;parent.excess_blob_gas=0;parent.blob_gas_used=786432;
  check(next_excess_blob_gas(parent,EvmRevision::Prague)==0,"Reserve rule activated before Osaka");
  check(next_excess_blob_gas(parent,EvmRevision::Osaka)==Int(fee>16?262144:0),"Reserve equality boundary differs");
 }
 Header low;low.base_fee=1000000000;low.excess_blob_gas=0;low.blob_gas_used=655360;
 check(next_excess_blob_gas(low,EvmRevision::Osaka)==0,"Reserve rule bypassed below-target clamp");
 for(size_t size:{size_t(8388607),size_t(8388608),size_t(8388609)}){
  validate_execution_block_size(size,EvmRevision::Prague);bool rejected=false;
  try{validate_execution_block_size(size,EvmRevision::Osaka);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected==(size>8388608),"RLP size bound or fork gate differs");
 }
 // Compare calculated size with actual RLP for both transaction envelopes.
 for(unsigned type:{0u,2u}){
  Transaction tx;tx.type=type;tx.to="0x0000000000000000000000000000000000000011";tx.data=Bytes(56,1);auto signed_tx=SignedTransaction::sign(tx,bytes(Int(1),32));
  Block b;b.transactions={signed_tx};b.header.withdrawals_root=withdrawals_root(Json::array());
  auto raw=signed_tx.raw();Rlp encoded_tx=type?Rlp(raw):Rlp::decode(raw);
  Rlp encoded(std::vector<Rlp>{Rlp::decode(b.header.encode()),Rlp(std::vector<Rlp>{encoded_tx}),Rlp(std::vector<Rlp>{}),Rlp(std::vector<Rlp>{})});
  check(execution_rlp_size(b)==encoded.encode().size(),"Calculated RLP block size differs");
 }
 Block oversized;Transaction huge;huge.type=2;huge.to="0x0000000000000000000000000000000000000011";huge.data=Bytes(8388608,1);oversized.transactions.push_back(SignedTransaction{huge,0,1,1});
 State unchanged;auto original=unchanged.to_json();bool size_rejected=false;
 try{ExecutionEngine(1,{},EvmRevision::Osaka).verify_block(unchanged,oversized,false);}catch(const std::invalid_argument& e){size_rejected=std::string(e.what())=="execution block RLP size limit exceeded";}
 check(size_rejected&&unchanged.to_json()==original,"Oversized block was executed or mutated state");
 std::cout<<"Osaka CLZ, MODEXP pricing/bounds, P256 validity/infinity, fork gates and transaction gas cap passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
