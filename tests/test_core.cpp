#include "nano/core.hpp"
#include "nano/evm.hpp"
#include "nano/precompiles.hpp"
#include "nano/consensus.hpp"
#include "nano/txpool.hpp"
#include "nano/receipt.hpp"
#include "nano/engine.hpp"
#include "nano/storage.hpp"
#include "nano/wallet.hpp"
#include "nano/hdwallet.hpp"
#include "nano/client.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include "nano/rlp.hpp"
#include "nano/crypto.hpp"
#include <iostream>
using namespace nano;
static int checks=0;
void check(bool yes,const char* name){++checks;if(!yes)throw std::runtime_error(name);}
template<class F>void rejects(F f,const char* name){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw,name);}
int main(){try{
 {
  check(u64(Int(0))==0,"u64 zero");
  check(u64(Int("18446744073709551615"))==UINT64_MAX,"u64 exact maximum");
  for(unsigned bit=0;bit<64;++bit){Int n=Int(1)<<bit;check(u64(n)==(uint64_t(1)<<bit),"u64 single-bit boundary");check(u64(n-1)==((uint64_t(1)<<bit)-1),"u64 below each bit boundary");}
  for(const auto& n:std::vector<Int>{Int(-1),-Int("18446744073709551615"),Int(1)<<64,(Int(1)<<64)+1,Int(1)<<256,Int(1)<<4096}){
   bool overflow=false;try{(void)u64(n);}catch(const std::overflow_error&){overflow=true;}check(overflow,"u64 rejects negative or wide input before narrowing");
  }
 }

 {
  State state;auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));state.get(sender).balance=Int("1000000000000000000");
  Transaction tx;tx.chain_id=0;tx.to="0x3000000000000000000000000000000000000000";
  auto sig=sign_hash(key,tx.signing_hash(false));SignedTransaction legacy{tx,Int(sig.v+27),sig.r,sig.s};Header block;
  auto initial=state.root();
  rejects([&]{ExecutionEngine(1337).apply_transaction(state,legacy,block,0,0);},"Nano requires chain-bound signatures");
  rejects([&]{ExecutionEngine(1337,{},EvmRevision::Paris).apply_transaction(state,legacy,block,0,0);},"Nano Paris network retains chain binding");
  auto wrong=tx;wrong.chain_id=9;
  rejects([&]{ExecutionEngine(1,{},EvmRevision::Berlin,ReplayPolicy::AllowLegacyUnprotected).apply_transaction(state,SignedTransaction::sign(wrong,key),block,0,0);},"Berlin still rejects wrong protected chain ID");
  check(state.root()==initial,"rejected replay domains never mutate state");
  auto receipt=ExecutionEngine(1,{},EvmRevision::Berlin,ReplayPolicy::AllowLegacyUnprotected).apply_transaction(state,legacy,block,0,0);
  check(receipt.success&&receipt.gas_used==21000,"Berlin diagnostic accepts authentic historical unprotected transaction");
 }

 for(auto revision:{EvmRevision::Berlin,EvmRevision::Paris})for(unsigned mode:{0u,1u,2u}){
  State state;ExecutionContext ctx;ctx.revision=revision;ctx.address="0x1000000000000000000000000000000000000000";
  std::string victim="0x2000000000000000000000000000000000000000",beneficiary="0x3000000000000000000000000000000000000000",middle="0x4000000000000000000000000000000000000000";
  auto call=[&](const std::string& target){return concat(concat(unhex("0x6000600060006000600073"),unhex(target)),unhex("0x61fffff150"));};
  auto destructor=concat(concat(Bytes{0x73},unhex(beneficiary)),Bytes{0xff});state.get(victim).code=destructor;state.get(victim).balance=10;state.get(beneficiary).balance=1;
  if(mode==2){state.get(middle).code=concat(call(victim),unhex("0x60006000fd"));ctx.code=call(middle);}
  else ctx.code=concat(call(victim),call(victim));
  ctx.code=concat(ctx.code,concat(concat(Bytes{0x73},unhex(victim)),unhex("0x3b60005260206000f3")));
  if(mode==1){ctx.code.resize(ctx.code.size()-1);ctx.code.push_back(0xfd);}
  auto result=EVM(state).execute(ctx);
  check(result.success==(mode!=1)&&integer(result.return_data)==destructor.size(),"SELFDESTRUCT leaves code observable within transaction");
  check(result.gas_refund==(mode==0&&revision==EvmRevision::Berlin?24000:0),"SELFDESTRUCT refund counted once and rolled back with frame");
  check(state.get(beneficiary).balance==(mode==0?11:1),"SELFDESTRUCT transfer rolled back on nested or outer revert");
  check(state.exists(victim)==(mode!=0),"SELFDESTRUCT final deletion only for committed transaction");
 }

 for(auto revision:{EvmRevision::Berlin,EvmRevision::Paris}){
  State state;ExecutionContext ctx;ctx.revision=revision;ctx.address="0x1000000000000000000000000000000000000000";
  state.set_storage(ctx.address,0,1);ctx.code=unhex("0x6000600055");
  auto result=EVM(state).execute(ctx);check(result.success&&result.gas_refund==(revision==EvmRevision::Berlin?15000:4800),"fork-specific clear refund");
  ctx.code=unhex("0x5f00");check(!EVM(state).execute(ctx).success,"PUSH0 unavailable before Shanghai");
  ctx.code=unhex("0x4800");check(EVM(state).execute(ctx).success==(revision==EvmRevision::Paris),"BASEFEE unavailable before London");
  ctx.address="0x000000000000000000000000000000000000000a";ctx.code.clear();check(EVM(state).execute(ctx).success,"address ten is ordinary before Cancun");
  auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));state.get(sender).balance=1000000000;
  Header block;block.coinbase="0x2000000000000000000000000000000000000000";block.base_fee=10;
  Transaction tx;tx.gas_price=20;tx.to="0x3000000000000000000000000000000000000000";
  auto receipt=ExecutionEngine(1337,{},revision).apply_transaction(state,SignedTransaction::sign(tx,key),block,0,0);
  check(receipt.success&&state.get(block.coinbase).balance==21000*(revision==EvmRevision::Paris?10:20),"Paris burns base fee; Berlin pays full pre-London gas price");
  tx.nonce=1;tx.to.reset();tx.data=unhex("0x60ef60005360016000f3");tx.gas=100000;
  receipt=ExecutionEngine(1337,{},revision).apply_transaction(state,SignedTransaction::sign(tx,key),block,1,21000);
  check(receipt.success==(revision==EvmRevision::Berlin),"EIP-3541 code prefix rejected in Paris only");
 }

 // EIP-2200 transition table with EIP-2929/3529 prices (Paris).
 struct StorageVector{unsigned original;std::vector<unsigned> writes;uint64_t gas;int refund;};
 for(const auto& v:std::vector<StorageVector>{
  {0,{0,0},2312,0},{0,{0,1},22212,0},{0,{1,0},22212,19900},
  {0,{1,2},22212,0},{0,{1,1},22212,0},{1,{0,0},5112,4800},
  {1,{0,1},5112,2800},{1,{0,2},5112,0},{1,{2,0},5112,4800},
  {1,{2,3},5112,0},{1,{2,1},5112,2800},{1,{2,2},5112,0},
  {1,{1,0},5112,4800},{1,{1,2},5112,0},{1,{1,1},2312,0},
  {0,{1,0,1},42218,19900},{1,{0,1,0},8018,7600}
 })for(bool warm:{false,true}){
  State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";
  state.set_storage(ctx.address,0,v.original);
  if(warm)ctx.access_list.push_back({ctx.address,{Bytes(32)}});
  for(auto value:v.writes)ctx.code=concat(ctx.code,Bytes{0x60,uint8_t(value),0x60,0,0x55});
  EVM vm(state);auto result=vm.execute(ctx);
  check(result.success&&result.gas_used==v.gas-(warm?2100:0)&&result.gas_refund==v.refund,"SSTORE original/current transition costs and refunds");
  check(state.get_storage(ctx.address,0)==v.writes.back(),"SSTORE publishes final value");
  ctx.code=unhex("0x6000600055");result=vm.execute(ctx);
  check(result.success&&result.gas_refund==(v.writes.back()==0?0:4800),"SSTORE original-value cache resets between executions");
 }
 for(uint64_t budget:{2306u,2307u}){
  State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";ctx.gas=budget;ctx.code=unhex("0x6000600055");ctx.access_list.push_back({ctx.address,{Bytes(32)}});
  auto result=EVM(state).execute(ctx);
  check(result.success==(budget==2307),"SSTORE requires more than 2300 gas before executing");
  check(result.gas_used==(budget==2307?106:budget),"SSTORE stipend violation consumes frame budget");
 }
 for(bool revert:{false,true}){
  State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";
  std::string callee="0x2000000000000000000000000000000000000000";
  state.set_storage(ctx.address,0,1);state.get(callee).code=unhex(revert?"0x600160005560006000fd":"0x6001600055");
  ctx.code=concat(unhex("0x6000600055600060006000600073"),unhex(callee));ctx.code=concat(ctx.code,unhex("0x61fffff450"));
  auto result=EVM(state).execute(ctx);
  check(result.success&&result.gas_refund==(revert?4800:2800),"nested negative refund merges only on successful delegatecall");
  check(state.get_storage(ctx.address,0)==(revert?0:1),"nested SSTORE restores state on revert");
 }

 for(unsigned size:{0u,1u,31u,32u,33u,64u,65u}){
  for(bool cold:{false,true}){
   State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";
   auto target=cold?"0x2000000000000000000000000000000000000000":ctx.address;
   state.get(target).code=Bytes(100,0xab);
   ctx.code=concat(Bytes{0x60,uint8_t(size),0x60,0,0x60,0,0x73},unhex(target));ctx.code.push_back(0x3c);
   auto result=EVM(state).execute(ctx);
   check(result.success&&result.gas_used==(cold?2612:112)+6*((size+31)/32),"EXTCODECOPY charges copying plus warm/cold access and memory");
  }
  State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";
  ctx.code=Bytes{0x60,0,0x60,0,0x60,uint8_t(size),0x60,0,0x60,0,0x60,4,0x61,0xff,0xff,0xf1,0x50,0x60,uint8_t(size),0x60,0,0x60,0,0x3e};
  auto result=EVM(state).execute(ctx);
  check(result.success&&result.gas_used==150+9*((size+31)/32),"RETURNDATACOPY charges rounded copying cost after identity call");
 }

 // A 33-byte exponent is priced using its first 32 bytes, not its tail.
 for(bool high_head:{false,true}){
  Bytes input=concat(concat(bytes(Int(128),32),bytes(Int(33),32)),bytes(Int(128),32));
  input.resize(96+128+33+128);input[96+128]=high_head?0x80:0;input[96+128+32]=0xff;
  uint64_t gas=high_head?22442:682;
  auto result=precompile(5,input,gas);
  check(result.success&&result.gas_used==gas&&result.return_data==Bytes(128),"MODEXP prices long exponent leading word exactly");
  auto low=precompile(5,input,gas-1);check(!low.success&&low.gas_used==gas-1,"MODEXP rejects one gas below required cost");
 }

 // CREATE failure has no CALL stipend; CREATE2 still pays init-code hashing.
 for(unsigned opcode:{0xf0u,0xf5u}){
  State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";ctx.gas=100000;
  if(opcode==0xf5)ctx.code={0x60,0};
  auto args=Bytes{0x60,1,0x60,0,0x60,1,uint8_t(opcode),0x00};ctx.code.insert(ctx.code.end(),args.begin(),args.end());
  auto result=EVM(state).execute(ctx);
  check(result.success&&result.gas_used==(opcode==0xf5?32021:32012),"insufficient CREATE balance preserves exact parent gas without stipend");
  check(state.get(ctx.address).nonce==0,"insufficient CREATE balance does not advance nonce");
  state.get(ctx.address).balance=1;state.get(ctx.address).nonce=Int("18446744073709551615");
  result=EVM(state).execute(ctx);
  check(result.success&&state.get(ctx.address).nonce==Int("18446744073709551615"),"CREATE refuses creator nonce overflow");
 }

 // EVM memory grows in complete 32-byte words, including byte writes.
 for(unsigned offset: {0u,1u,30u,31u,32u,33u,63u,64u}) {
  State state;ExecutionContext ctx;ctx.address="0x1000000000000000000000000000000000000000";
  // MSTORE8(offset, 0xff); MSIZE; MSTORE(0, size); RETURN(0,32).
  ctx.code=Bytes{0x60,0xff,0x60,uint8_t(offset),0x53,0x59,0x60,0,0x52,0x60,32,0x60,0,0xf3};
  auto result=EVM(state).execute(ctx);unsigned words=offset/32+1;
  check(result.success&&integer(result.return_data)==words*32,"MSIZE rounds byte-write expansion to whole words");
  check(result.gas_used==23+3*words,"word-rounded memory expansion charged once");
 }
 {std::ifstream file("tests/fixtures/precompiles.json");Json fixture;file>>fixture;
  for(const auto& vector:fixture.at("vectors")){
   auto input=unhex(vector.at("input"));auto number=vector.at("address").get<unsigned>();auto gas=vector.at("gas").get<uint64_t>();
   auto label=vector.at("name").get<std::string>();
   auto result=precompile(number,input,gas);
   if(vector.contains("divergence")){check(!result.success&&result.return_data.empty(),(label+" native rejects invalid curve coordinates").c_str());continue;}
   check(result.success&&!vector.at("output").is_null(),(label+" precompile success").c_str());
   check(result.gas_used==gas&&hex(result.return_data)==vector.at("output").get<std::string>(),(label+" gas and output").c_str());
   auto exhausted=precompile(number,input,gas-1);
   check(!exhausted.success&&exhausted.gas_used==gas-1&&exhausted.return_data.empty(),(label+" below exact gas fails").c_str());
  }
 }
 for(uint8_t opcode:{uint8_t(0xf0),uint8_t(0xf5)})for(unsigned scenario=0;scenario<4;++scenario){
  State state;EVM vm(state);ExecutionContext context;context.address="0xabababababababababababababababababababab";
  context.gas=(scenario==0||scenario==3)?100000:scenario==1?10000000:1000000;state.get(context.address).balance=100;
  unsigned runtime_size=scenario==1?24577:scenario==3?335:1000;
  Bytes init=unhex(scenario==3?"0x61":"0x602a60005560006000a061");init.push_back(uint8_t(runtime_size>>8));init.push_back(uint8_t(runtime_size));init=concat(init,unhex("0x6000f3"));
  Bytes code{uint8_t(0x5f+init.size())};code=concat(code,init);code=concat(code,unhex("0x600052"));
  if(opcode==0xf5)code=concat(code,unhex("0x6001"));
  code=concat(code,Bytes{0x60,uint8_t(init.size()),0x60,uint8_t(32-init.size()),0x60,5,opcode});
  context.code=concat(code,unhex("0x6000523d60205260406000f3"));
  auto target=opcode==0xf0?create_address(context.address,0):create2_address(context.address,bytes(Int(1),32),keccak(init));
  auto result=vm.execute(context);
  check(result.success&&result.gas_used<context.gas,"CREATE failure preserves gas for parent continuation");
  check(result.return_data.size()==64&&integer(Bytes(result.return_data.begin()+32,result.return_data.end()))==0,"CREATE clears return data after success or deposit failure");
  auto returned=integer(Bytes(result.return_data.begin(),result.return_data.begin()+32));
  check(state.get(context.address).nonce==1,"CREATE advances creator nonce even on child failure");
  if(scenario!=2){
   check(returned==0&&result.logs.empty(),"failed code deposit returns zero and discards constructor logs");
   check(state.get(context.address).balance==100&&state.get(target).balance==0&&state.get(target).nonce==0&&state.get(target).code.empty()&&state.get_storage(target,0)==0,"failed code deposit rolls back transfer and constructor state");
  }else{
   check(returned==integer(unhex(target))&&result.logs.size()==1,"successful creation publishes address and logs");
   check(state.get(context.address).balance==95&&state.get(target).balance==5&&state.get(target).nonce==1&&state.get(target).code==Bytes(runtime_size)&&state.get_storage(target,0)==42,"successful deposit publishes complete constructor state");
  }
 }
 {std::ifstream input("tests/fixtures/python-evm.json");Json fixture;input>>fixture;
  check(fixture.at("vectors").size()==451,"Python EVM fixture has all reference cases");
  for(const auto& vector:fixture.at("vectors")){
   State state;EVM vm(state);ExecutionContext context;
   context.origin=context.caller=context.address="0xabababababababababababababababababababab";
   context.origin=vector.value("origin",context.origin);context.caller=vector.value("caller",context.caller);
   context.value=vector.value("value",0);context.gas_price=vector.value("gasPrice",0);
   if(vector.contains("environment")){
    const auto& env=vector.at("environment");context.coinbase=env.at("coinbase").get<std::string>();
    context.timestamp=Int(env.at("timestamp").get<std::string>());context.number=Int(env.at("number").get<std::string>());
    context.prevrandao=Int(env.at("prevrandao").get<std::string>());context.gas_limit=Int(env.at("gas_limit").get<std::string>());
    context.chain_id=Int(env.at("chain_id").get<std::string>());context.base_fee=Int(env.at("base_fee").get<std::string>());
   }
   context.gas=vector.value("gas",1000000);context.code=unhex(vector.at("code"));
   context.data=unhex(vector.value("data",std::string("0x")));context.is_static=vector.value("static",false);
   if(vector.contains("initialStorage"))state.set_storage(context.address,0,Int(vector.at("initialStorage").get<long>()));
   state.get(context.address).balance=vector.value("balance",0);
   if(vector.contains("accounts"))for(const auto& account:vector.at("accounts")){
    auto addr=account.at("address").get<std::string>();state.get(addr).code=unhex(account.at("code"));
    state.set_storage(addr,0,Int(account.at("storage").get<long>()));
   }
   auto result=vm.execute(context);const auto& expected=vector.at("result");
   auto name=vector.at("name").get<std::string>();
   if(name=="return-oog"){
    check(expected.at("success").get<bool>()&&expected.at("gasUsed").get<unsigned>()>context.gas,"Python RETURN exceeds available gas");
    check(!result.success&&result.gas_used==context.gas&&result.return_data.empty(),"Native RETURN rejects memory expansion without gas");
    check(state.get_storage(context.address,0)==7,"RETURN out of gas preserves storage");
    continue;
   }
   check(result.success==expected.at("success"),(name+" Python EVM success").c_str());
   if(name=="call-revert"||name=="staticcall-revert"||name=="delegatecall-revert"){
    check(expected.at("returnData").get<std::string>()==hex(Bytes(160)),"Python omits reverted call output memory copy");
    Bytes output(160);output[0]=0xab;
    check(result.return_data==output,"Native copies revert payload to caller output memory");
   }else if(name=="memory-size"){
    check(integer(unhex(expected.at("returnData")))==34,"prototype fixture records unrounded memory size");
    check(integer(result.return_data)==64,"native MSIZE follows 32-byte memory words");
   }else check(hex(result.return_data)==expected.at("returnData").get<std::string>(),(name+" Python EVM return bytes").c_str());
   // Correct the prototype's duplicate storage charges and full-gas REVERT.
   // Values follow opcode sums: cold reset 5000, warm read 100, REVERT
   // charges only its executed prefix. Unrelated fixture behavior stays checked.
   const std::map<std::string,unsigned> corrected_gas{
    {"storage-set-read",5124},{"storage-clear",5124},{"storage-repeat-read",2223},
    {"storage-revert",5024},{"log-revert",387},{"call-storage",7886},
    {"delegatecall-storage",7983},{"call-revert",2780},
    {"staticcall-revert",2877},{"delegatecall-revert",2877}};
   auto corrected=corrected_gas.find(name);
   unsigned expected_gas=corrected==corrected_gas.end()?expected.at("gasUsed").get<unsigned>():corrected->second;
   // Remove duplicated call base cost and the extra cold charge inherited
   // by STATICCALL/DELEGATECALL in the prototype fixtures.
   if(name.starts_with("call-")||name.ends_with("-call"))expected_gas-=100;
   else if(name.starts_with("staticcall-")||name.starts_with("delegatecall-")||name.ends_with("-staticcall")||name.ends_with("-delegatecall"))expected_gas-=200;
   check(result.gas_used==expected_gas,(name+" gas accounting").c_str());
   check(result.gas_refund==expected.at("gasRefund").get<long>(),(name+" Python EVM refund").c_str());
   Json logs=Json::array();for(const auto& log:result.logs)logs.push_back(log.to_json());
   check(logs==expected.at("logs"),(name+" Python EVM logs").c_str());
   if(vector.contains("storage"))check(state.get_storage(context.address,0)==vector.at("storage").get<long>(),(name+" Python EVM storage").c_str());
   if(vector.contains("targetStorage"))check(state.get_storage("0xcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd",0)==vector.at("targetStorage").get<long>(),(name+" Python EVM target storage").c_str());
  }
 }
 {std::ifstream input("tests/fixtures/bn256Pairing.json");Json fixture;input>>fixture;
  for(const auto& vector:fixture){auto data=unhex(vector["Input"]);uint64_t gas=45000+34000*(data.size()/192);auto result=precompile(8,data,gas);
   check(result.success&&hex(result.return_data).substr(2)==vector["Expected"].get<std::string>(),"BN128 pairing matches go-ethereum vector");
   check(result.gas_used==gas&&!precompile(8,data,gas-1).success,"BN128 pairing gas boundary");
  }
  check(!precompile(8,Bytes(191),100000).success,"pairing rejects incomplete tuple");
  auto data=Bytes(192);auto prime=bytes(Int("21888242871839275222246405745257275088696311157297823662689037894645226208583"),32);std::copy(prime.begin(),prime.end(),data.begin());
  check(!precompile(8,data,100000).success,"pairing rejects field modulus coordinate");
  data=Bytes(192);data[31]=1;data[63]=1;check(!precompile(8,data,100000).success,"pairing validates G1 even with G2 infinity");
  data=Bytes(192);data[127]=1;data[191]=1;check(!precompile(8,data,100000).success,"pairing validates G2 even with G1 infinity");
  check(precompile(8,Bytes(192),79000).return_data==bytes(Int(1),32),"pairing accepts both points at infinity");
 }

 check(nano_amount("0.000000000000000001")==1,"CLI amount preserves one wei");
 check(nano_amount("123456789.123456789123456789")==Int("123456789123456789123456789"),"CLI amount avoids floating point rounding");
 rejects([]{nano_amount("1.0000000000000000001");},"CLI rejects subwei precision");
 rejects([]{nano_amount("-1");},"CLI rejects negative amount");
 rejects([]{nano_amount("1e3");},"CLI rejects ambiguous scientific amount");

 {std::ifstream input("tests/fixtures/hdwallet.json");Json fixture;input>>fixture;
  for(const auto& v:fixture["vectors"]){
   auto phrase=v["mnemonic"].get<std::string>(),passphrase=v["passphrase"].get<std::string>();
   check(mnemonic_from_entropy(unhex(v["entropy"]))==phrase,"BIP39 entropy to phrase matches Python");
   check(hex(mnemonic_entropy(phrase)).substr(2)==v["entropy"].get<std::string>(),"BIP39 phrase to entropy");
   auto seed=mnemonic_seed(phrase,passphrase);check(hex(seed).substr(2)==v["seed"].get<std::string>(),"BIP39 seed and Unicode normalization match Python");
   for(const auto& child:v["children"])check(hex(derive_private_key(seed,child["path"])).substr(2)==child["key"].get<std::string>(),"BIP32 key matches Python hardened and normal derivation");
   HDWallet wallet(phrase,passphrase);check(wallet.derive_account().address()==v["children"][2]["address"].get<std::string>(),"BIP44 Ethereum address matches Python");
  }
  std::string invalid;for(int i=0;i<12;++i)invalid+=(i?" ":"")+std::string("abandon");
  rejects([&]{HDWallet wallet(invalid);},"mnemonic checksum rejected");
  rejects([]{HDWallet wallet("unknown words");},"invalid mnemonic rejected");
  rejects([]{derive_private_key(Bytes(16),"m/2147483648");},"HD index overflow rejected");
  rejects([]{derive_private_key(Bytes(16),"m//1");},"empty HD path segment rejected");
  rejects([]{HDWallet::create(13);},"unsupported mnemonic word count rejected");
  for(unsigned count:{12,15,18,21,24}){auto wallet=HDWallet::create(count);check(mnemonic_entropy(wallet.mnemonic()).size()==count/3*4,"generated HD phrase has valid checksum");}
 }

 {State state;EVM vm(state);ExecutionContext context;context.number=300;context.gas=1000;
  int reads=0;context.block_hash=[&](const Int& n){++reads;return bytes(Int(n+123),32);};
  for(int number:{43,44,299,300,301}){
   context.code={0x61,uint8_t(number>>8),uint8_t(number),0x40,0x60,0x00,0x52,0x60,0x20,0x60,0x00,0xf3};
   auto result=vm.execute(context);Int expected=number>=44&&number<300?Int(number+123):Int(0);
   check(result.success&&result.return_data==bytes(expected,32),"BLOCKHASH enforces exact 256 ancestor window");
  }
  check(reads==2,"BLOCKHASH out of range avoids storage reads");
  state.get("0x00000000000000000000000000000000000000ab").code=unhex("0x602c4060005260206000f3");
  context.gas=100000;context.code=unhex("0x602060006000600060ab6103e8f45060206000f3");
  auto nested=vm.execute(context);
  check(nested.success&&nested.return_data==bytes(Int(167),32),"DELEGATECALL inherits ancestor resolver");

 }

 check(hex(keccak({}))=="0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470","keccak empty");
 check(hex(keccak(Bytes{'a','b','c'}))=="0x4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45","keccak abc");
 check(hex(Rlp(Bytes{'d','o','g'}).encode())=="0x83646f67","RLP dog");
 check(hex(Rlp(std::vector<Rlp>{Rlp(Bytes{'c','a','t'}),Rlp(Bytes{'d','o','g'})}).encode())=="0xc88363617483646f67","RLP list");
 check(Rlp::decode(Rlp(Int(1024)).encode()).number()==1024,"RLP integer");
 check(trie_root({})==keccak(Bytes{128}),"empty trie");
 rejects([]{Rlp::decode(Bytes{0x81,0x01});},"noncanonical byte");
 rejects([]{Rlp::decode(Bytes{0xb8,0x01,0xff});},"noncanonical length");
 rejects([]{Rlp::decode(Bytes{0xc1,0xc1});},"truncated nested list");
 rejects([]{Rlp::decode(Bytes{0,0});},"trailing RLP");
 rejects([]{Rlp(Bytes{0}).number();},"noncanonical integer");
 check(checksum_address("0x52908400098527886e0f7030069857d2e4169ee7")=="0x52908400098527886E0F7030069857D2E4169EE7","EIP55");
 auto key=bytes(Int(1),32);auto pub=public_key(key);check(public_address(pub)=="0x7E5F4552091A69125d5DfCb7b8C2659029395Bdf","key one address");

 {Wallet wallet(key);KeyStore keys;auto addr=keys.add_wallet(wallet,"primary");
  check(addr==public_address(pub)&&keys.get_default()->address()==addr,"wallet default and address");
  keys.add_wallet(wallet,"alias");check(keys.addresses().size()==1&&keys.get_wallet("alias")->address()==addr,"wallet aliases do not duplicate accounts");
  auto other=Wallet(bytes(Int(2),32));auto second=keys.add_wallet(other,"second");
  rejects([&]{keys.add_wallet(other,"primary");},"wallet alias collision rejected");
  check(keys.set_default("second")&&keys.get_default()->address()==second,"named default wallet");
  check(keys.remove_wallet("primary")&&!keys.get_wallet("alias")&&!keys.get_wallet(addr),"removal clears every wallet alias");
  check(keys.remove_wallet(second)&&!keys.get_default()&&keys.addresses().empty(),"last wallet removal clears default");
  auto signature=wallet.sign_message(Bytes{'n','a','n','o'});check(recover_message(Bytes{'n','a','n','o'},signature)==addr,"wallet personal signature recovery");
  Transaction transfer;transfer.to=addr;check(wallet.sign_transaction(transfer).sender()==addr,"wallet signs transaction");
  auto file=std::filesystem::temp_directory_path()/("nano-wallet-"+std::to_string(getpid())+".json");
  wallet.save_keyfile(file,"test-password");struct stat info{};check(::stat(file.c_str(),&info)==0&&(info.st_mode&0777)==0600,"wallet keyfile private permissions");
  check(Wallet::from_keyfile(file,"test-password").address()==addr,"encrypted wallet reopens");
  rejects([&]{Wallet::from_keyfile(file,"incorrect");},"incorrect wallet password rejected");
  rejects([&]{other.save_keyfile(file,"test-password");},"wallet cannot overwrite existing keyfile");
  check(Wallet::from_keyfile(file,"test-password").address()==addr,"failed overwrite preserves wallet");
  std::filesystem::remove(file);
 }

 {KeyStore keys;auto first=keys.add_wallet(Wallet(bytes(Int(1),32)),"one");
  auto third=keys.add_wallet(Wallet(bytes(Int(3),32)),"three");
  auto second=keys.add_wallet(Wallet(bytes(Int(2),32)),"two");
  check(keys.addresses()==std::vector<std::string>{first,third,second},"keystore preserves Python insertion order");
  keys.add_wallet(Wallet(bytes(Int(1),32)),"new-alias");
  check(keys.addresses()==std::vector<std::string>{first,third,second},"readding wallet does not reorder accounts");
  keys.remove_wallet(first);check(keys.get_default()->address()==third,"default fallback selects earliest remaining wallet");
  keys.add_wallet(Wallet(bytes(Int(1),32)));check(keys.addresses()==std::vector<std::string>{third,second,first},"removed and readded wallet appears last");
  auto created=keys.create_wallet("generated");check(keys.get_wallet("generated")->address()==created.address()&&keys.addresses().back()==created.address(),"keystore creates and registers named wallet");
  auto before=keys.addresses();rejects([&]{keys.create_wallet("generated");},"creating duplicate alias rejected");
  check(keys.addresses()==before&&keys.get_default()->address()==third,"failed creation leaves registry and default unchanged");
 }

 {std::ifstream fixture("tests/fixtures/typed_data.json");Json fixtures;fixture>>fixtures;
  for(const auto& vector:fixtures["vectors"]){
   check(hex(typed_data_hash(vector["data"]))==vector["hash"].get<std::string>(),"EIP712 hash matches eth-account");
   check(hex(Wallet(key).sign_typed_data(vector["data"]))==vector["signature"].get<std::string>(),"EIP712 signature matches eth-account");
  }
  auto data=fixtures["vectors"][1]["data"];
  data["message"]["small"]=-129;rejects([&]{typed_data_hash(data);},"EIP712 rejects signed overflow");
  data["message"]["small"]=-128;data["message"]["high"]=-1;rejects([&]{typed_data_hash(data);},"EIP712 rejects negative unsigned value");
  data=fixtures["vectors"][3]["data"];data["message"]["grid"][0].push_back(3);rejects([&]{typed_data_hash(data);},"EIP712 rejects fixed array length mismatch");
  data=fixtures["vectors"][0]["data"];data["types"]["Mail"][0]["type"]="Missing";rejects([&]{typed_data_hash(data);},"EIP712 rejects unknown struct");
  data=fixtures["vectors"][2]["data"];data["message"]["yes"]="false";rejects([&]{typed_data_hash(data);},"EIP712 does not coerce boolean strings");
 }
 auto hash=keccak(Bytes{'t','e','s','t'});auto sig=sign_hash(key,hash);check(recover_public_key(hash,sig)==pub,"recover signature");auto again=sign_hash(key,hash);check(sig.r==again.r&&sig.s==again.s&&sig.v==again.v,"deterministic RFC6979");
 auto message=Bytes{'h','e','l','l','o'};check(recover_message(message,sign_message(key,message))==public_address(pub),"personal signature");
 rejects([]{public_key(Bytes(32));},"reject zero key");
 auto encrypted=encrypt_key(key,"test-only-password");check(decrypt_key(encrypted,"test-only-password")==key,"keystore roundtrip");
 rejects([&]{decrypt_key(encrypted,"wrong");},"bad password");
 std::ifstream fixtures("tests/fixtures/python-core.json"); Json vectors;fixtures>>vectors;
 for(const auto& h:vectors["hashes"])check(hex(keccak(unhex(h["input"])))==h["hash"].get<std::string>(),"Python keccak differential");
 for(const auto& fixture:vectors["tries"]){std::map<Bytes,Bytes> entries;for(const auto& [k,v]:fixture["entries"].items())entries[unhex(k)]=unhex(v);check(hex(trie_root(entries))==fixture["root"].get<std::string>(),"Python MPT differential");}
 for(const auto& fixture:vectors["states"]){auto state=State::from_json(fixture["alloc"]);check(hex(state.root())==fixture["root"].get<std::string>(),"Python state differential");check(State::from_json(state.to_json()).root()==state.root(),"state roundtrip");}
 for(const auto& fixture:vectors["transactions"]){auto tx=Transaction::from_json(fixture["tx"]);check(hex(tx.signing_hash())==fixture["signingHash"].get<std::string>(),"Python signing hash");auto st=SignedTransaction::sign(tx,key);check(hex(st.raw())==fixture["raw"].get<std::string>(),"Python signed raw bytes");auto decoded=SignedTransaction::decode(unhex(fixture["raw"]));check(hex(decoded.hash())==fixture["hash"].get<std::string>(),"Python transaction hash");check(decoded.sender()==fixture["sender"].get<std::string>(),"Python sender recovery");check(decoded.raw()==st.raw(),"transaction decode roundtrip");}
 {
  Header parent,child;parent.gas_limit=10240000;parent.gas_used=parent.gas_limit/2;parent.base_fee=100;child=parent;
  for(int delta:{-9999,0,9999}){child.gas_limit=parent.gas_limit+delta;validate_execution_header(child,parent);}
  for(int delta:{-10000,10000}){child.gas_limit=parent.gas_limit+delta;rejects([&]{validate_execution_header(child,parent);},"gas limit boundary must be strict");}
  child=parent;child.base_fee=99;rejects([&]{validate_execution_header(child,parent);},"parent base fee mismatch rejected");
  child=parent;child.base_fee.reset();rejects([&]{validate_execution_header(child,parent);},"missing active base fee rejected");
  parent.base_fee.reset();child=parent;validate_execution_header(child,parent);child.base_fee=0;rejects([&]{validate_execution_header(child,parent);},"base fee before activation rejected");
  parent.gas_limit=5000;parent.gas_used=0;child=parent;validate_execution_header(child,parent);child.gas_limit=4999;rejects([&]{validate_execution_header(child,parent);},"minimum gas limit rejected");
  child=parent;child.gas_used=5001;rejects([&]{validate_execution_header(child,parent);},"gas used above limit rejected");
 }
 Header genesis;std::string extra="NanoPy Genesis";genesis.extra_data=Bytes(extra.begin(),extra.end());check(genesis.to_json()==Header::from_json(vectors["genesis"]).to_json(),"Python genesis header");
 State state;std::string alice="0x"+std::string(40,'a'),bob="0x"+std::string(40,'b');state.get(alice).balance=100;auto initial=state.root();auto snap=state.snapshot();check(state.transfer(alice,bob,30),"transfer");state.set_storage(bob,0,123);state.revert(snap);check(state.root()==initial&&state.get(bob).balance==0,"snapshot restores all state");check(!state.transfer(alice,bob,101),"insufficient balance");rejects([&]{state.transfer(alice,bob,-1);},"negative transfer");
 check(create2_address("0x"+std::string(40,'0'),Bytes(32),keccak(Bytes{0}))=="0x4d1a2e2bb4f88f0250f26ffff098b0b30b26bf38","EIP1014 vector");
 EVM vm(state);ExecutionContext ctx;ctx.address=bob;ctx.code=unhex("0x600260030160005260206000f3");auto result=vm.execute(ctx);check(result.success&&integer(result.return_data)==5,"EVM add and return");check(result.gas_used==24,"EVM arithmetic gas");
 // Exercise owned stack operands: subtraction wraps negative results, products
 // exceed 256 bits, and DUP must preserve an independent original operand.
 for(unsigned i=0;i<64;++i)for(uint8_t opcode:{uint8_t(0x03),uint8_t(0x02)}){
  Int a=(Int(1)<<256)-1-i,b=(Int(1)<<255)+i;
  ctx.code=concat(Bytes{0x7f},bytes(a,32));ctx.code=concat(ctx.code,Bytes{0x7f});ctx.code=concat(ctx.code,bytes(b,32));
  ctx.code=concat(ctx.code,Bytes{opcode,0x80,0x01});ctx.code=concat(ctx.code,unhex("0x60005260206000f3"));
  Int operand=opcode==0x03?Int(b-a):Int(a*b);Int expected=word(2*word(operand));result=vm.execute(ctx);
  check(result.success&&integer(result.return_data)==expected,"owned EVM stack wraps and duplicates 256-bit operands");
  check(result.gas_used==(opcode==0x03?30u:32u),"owned EVM stack preserves arithmetic gas");
 }
 ctx.code.clear();for(unsigned i=0;i<1024;++i)ctx.code=concat(ctx.code,Bytes{0x60,0x01});
 check(vm.execute(ctx).success,"owned EVM stack accepts exactly 1024 operands");ctx.code.push_back(0x80);
 result=vm.execute(ctx);check(!result.success&&result.error=="Stack overflow","owned EVM stack rejects the 1025th operand");
 for(unsigned width=1;width<=32;++width){
  Bytes immediate(width,0xff),expected(32-width,0);expected.insert(expected.end(),immediate.begin(),immediate.end());
  ctx.code=concat(Bytes{uint8_t(0x5f+width)},immediate);ctx.code=concat(ctx.code,unhex("0x60005260206000f3"));
  result=vm.execute(ctx);check(result.success&&result.return_data==expected&&result.gas_used==18,"PUSH widths preserve unsigned big-endian output and gas");
  ctx.gas=17;result=vm.execute(ctx);check(!result.success&&result.gas_used==17,"PUSH memory return still exhausts one-short gas");ctx.gas=1000000;
  for(unsigned available:{0u,width-1}){ctx.code=concat(Bytes{uint8_t(0x5f+width)},Bytes(available,0xff));result=vm.execute(ctx);check(result.success&&result.gas_used==3,"truncated PUSH is padded rather than rejected");}
 }
 ctx.code=unhex("0x602a60005560005460005260206000f3");result=vm.execute(ctx);check(result.success&&integer(result.return_data)==42&&state.get_storage(bob,0)==42,"EVM storage");
 auto before=state.root();ctx.code=unhex("0x600160005560006000fd");result=vm.execute(ctx);check(!result.success&&state.root()==before&&result.logs.empty(),"EVM revert rolls back storage");
 ctx.is_static=true;ctx.code=unhex("0x6001600055");check(!vm.execute(ctx).success&&state.root()==before,"EVM static rejects storage");ctx.is_static=false;
 ctx.code=unhex("0x600356605b");check(!vm.execute(ctx).success,"EVM invalid jump into PUSH data");
 ctx.code=unhex("0x01");check(!vm.execute(ctx).success,"EVM stack underflow");
 ctx.code=unhex("0x6002600301");ctx.gas=8;check(!vm.execute(ctx).success,"EVM gas exhaustion");
 ctx.gas=1000000;std::string child="0x"+std::string(40,'c');state.get(bob).balance=100;
 auto call_code=[&](uint8_t opcode,unsigned value){Bytes code=unhex("0x6020600060006000");if(opcode==0xf1||opcode==0xf2)code=concat(code,Bytes{0x60,uint8_t(value)});code.push_back(0x73);code=concat(code,unhex(child));code=concat(code,unhex("0x620186a0"));code.push_back(opcode);return code;};
 state.get(child).code=unhex("0x602a60005260206000f3");ctx.code=concat(call_code(0xf1,7),unhex("0x5060206000f3"));result=vm.execute(ctx);check(result.success&&integer(result.return_data)==42,"CALL returns child output");check(state.get(bob).balance==93&&state.get(child).balance==7,"CALL transfers value");
 state.get(child).code=unhex("0x606360005560006000a060006000fd");auto parent_before=state.root();ctx.code=concat(call_code(0xf1,5),unhex("0x60005260206000f3"));result=vm.execute(ctx);check(result.success&&integer(result.return_data)==0,"failed child CALL status");check(state.root()==parent_before&&result.logs.empty(),"failed child rolls back transfer storage and logs");
 state.get(child).code=unhex("0x600960015500");ctx.is_static=true;ctx.code=call_code(0xf1,0);result=vm.execute(ctx);check(result.success&&state.get_storage(child,1)==0,"static context propagates through CALL");ctx.is_static=false;
 ctx.code=call_code(0xf4,0);result=vm.execute(ctx);check(result.success&&state.get_storage(bob,1)==9&&state.get_storage(child,1)==0,"DELEGATECALL uses caller storage");
 state.get(child).code=unhex("0x60006000a000");ctx.code=concat(call_code(0xf1,1),unhex("0x60006000fd"));parent_before=state.root();result=vm.execute(ctx);check(!result.success&&state.root()==parent_before&&result.logs.empty(),"parent revert rolls back successful child");
 Bytes init=unhex("0x600160005360016000f3");Bytes creator=concat(Bytes{uint8_t(0x5f+init.size())},init);creator=concat(creator,unhex("0x600052"));creator=concat(creator,Bytes{0x60,uint8_t(init.size()),0x60,uint8_t(32-init.size()),0x60,0,0xf0});creator=concat(creator,unhex("0x60005260206000f3"));auto expected_contract=create_address(bob,state.get(bob).nonce);ctx.code=creator;result=vm.execute(ctx);check(result.success&&integer(result.return_data)==integer(unhex(expected_contract)),"CREATE derived address");check(state.get(expected_contract).code==Bytes{1}&&state.get(expected_contract).nonce==1,"CREATE installs runtime code");
 state.get(child).balance=20;ctx.address=child;ctx.code=concat(concat(Bytes{0x73},unhex(bob)),Bytes{0xff});Int old_balance=state.get(bob).balance;result=vm.execute(ctx);check(result.success&&state.get(bob).balance==old_balance+20&&!state.exists(child),"SELFDESTRUCT transfers full balance");
 auto pre=precompile(2,Bytes{'a','b','c'},72);check(pre.success&&pre.gas_used==72&&pre.return_data==sha256(Bytes{'a','b','c'}),"SHA256 precompile");check(!precompile(2,Bytes{'a','b','c'},71).success,"precompile gas cap");
 check(precompile(4,Bytes{1,2,3},100).return_data==Bytes({1,2,3}),"identity precompile");
 Bytes ec_input=concat(concat(concat(hash,bytes(Int(sig.v+27),32)),bytes(sig.r,32)),bytes(sig.s,32));pre=precompile(1,ec_input,3000);check(pre.success&&integer(pre.return_data)==integer(unhex(public_address(pub))),"ECRECOVER precompile");
 Bytes mod_input=concat(concat(concat(bytes(Int(1),32),bytes(Int(1),32)),bytes(Int(1),32)),Bytes{2,5,13});pre=precompile(5,mod_input,200);check(pre.success&&pre.return_data==Bytes{6},"MODEXP precompile");
 Bytes generator=concat(bytes(Int(1),32),bytes(Int(2),32));pre=precompile(6,concat(generator,Bytes(64)),150);check(pre.success&&pre.return_data==generator,"BN128 add identity");auto doubled=precompile(6,concat(generator,generator),150);auto multiplied=precompile(7,concat(generator,bytes(Int(2),32)),6000);check(doubled.success&&multiplied.success&&doubled.return_data==multiplied.return_data,"BN128 scalar multiplication");check(!precompile(6,Bytes(128,255),150).success,"reject invalid BN128 curve point");
 ctx.address=bob;ctx.code=unhex("0x602060006000600060046103e8f460005260206000f3");result=vm.execute(ctx);check(result.success&&integer(result.return_data)==1,"DELEGATECALL resolves precompile by code address");
 // EIP-152 official vector 5, BLAKE2b("abc").
 Bytes blake=unhex("0000000c48c9bdf267e6096a3ba7ca8485ae67bb2bf894fe72f36e3cf1361d5f3af54fa5d182e6ad7f520e511f6c3e2b8c68059b6bbd41fbabd9831f79217e1319cde05b");Bytes block(128);block[0]='a';block[1]='b';block[2]='c';blake=concat(blake,block);blake=concat(blake,unhex("0300000000000000000000000000000001"));pre=precompile(9,blake,12);
 check(pre.success&&hex(pre.return_data)=="0xba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d17d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923","EIP152 BLAKE2F vector 5");
 check(!precompile(9,blake,11).success,"BLAKE2F gas exhaustion");blake[3]=0;pre=precompile(9,blake,0);check(pre.success&&hex(pre.return_data)=="0x08c9bcf367e6096a3ba7ca8485ae67bb2bf894fe72f36e3cf1361d5f3af54fa5d282e6ad7f520e511f6c3e2b8c68059b9442be0454267ce079217e1319cde05b","EIP152 zero round vector 4");blake.back()=2;check(!precompile(9,blake,12).success,"BLAKE2F invalid flag");check(!precompile(9,{},12).success,"BLAKE2F invalid length");
 std::ifstream pow_file("tests/fixtures/python-pow.json");Json pow_vectors;pow_file>>pow_vectors;auto pow_header=Header::from_json(pow_vectors["header"]);check(hex(ProofOfWork::hash(pow_header))==pow_vectors["powHash"].get<std::string>(),"Python PoW hash differential");for(const auto& [time,difficulty]:pow_vectors["adjustments"].items())check(ProofOfWork::adjust_difficulty(pow_header,Int(time))==quantity(difficulty),"Python difficulty adjustment");ProofOfWork miner;auto mined=miner.mine(pow_header,std::chrono::milliseconds(1000));check(mined.has_value()&&mined->attempts==1,"mine difficulty one");pow_header.nonce=mined->nonce;check(ProofOfWork::verify(pow_header),"verify mined PoW");
 Header successor=pow_header;successor.number=1;successor.timestamp=12;successor.parent_hash=pow_header.hash();check(validate_pow_header(successor,pow_header,Int(12)).valid,"PoW header acceptance");successor.gas_used=successor.gas_limit+1;check(!validate_pow_header(successor,pow_header,Int(12)).valid,"PoW rejects excess gas");
 for(int direction:{-1,1})for(int offset:{-1,0,1}){
  auto candidate=successor;candidate.gas_used=0;
  candidate.gas_limit=pow_header.gas_limit+direction*(pow_header.gas_limit/1024+offset);
  auto validation=validate_pow_header(candidate,pow_header,Int(12));
  check(offset<0?validation.valid:(!validation.valid&&validation.error=="Gas limit change too large"),"PoW gas adjustment strictly excludes both EIP-1559 boundaries");
 }
 std::ifstream pos_file("tests/fixtures/python-pos.json");Json pos_vectors;pos_file>>pos_vectors;ProofOfStake pos;for(const auto& v:pos_vectors["validators"])check(pos.register_validator(v["address"],quantity(v["stake"]),0),"PoS registration");for(const auto& item:pos_vectors["selections"]){auto selected=pos.select(quantity(item["block"]),unhex(item["hash"]));check(selected&&*selected==item["selected"].get<std::string>(),"Python weighted PoS selection");}
 auto validator=pos_vectors["validators"][0]["address"].get<std::string>();check(!pos.withdraw(validator,99),"stake locked before 100 blocks");Int stake_before=pos.total_stake();pos.reward(validator,1);check(pos.total_stake()==stake_before+ProofOfStake::block_reward,"PoS reward compound");check(!pos.add_stake(validator,-1,1),"PoS rejects negative stake");check(pos.withdraw(validator,100).has_value(),"stake unlock");ProofOfStake bootstrap;check(bootstrap.select(0,Bytes(32))==ProofOfStake::bootstrap_validator,"PoS bootstrap");bootstrap.bootstrap=false;check(!bootstrap.select(0,Bytes(32)),"PoS no validators without bootstrap");
 TxPool pool(2,2);Transaction pending;pending.to=bob;pending.gas_price=100;auto first=SignedTransaction::sign(pending,key);check(pool.add(first)&&!pool.add(first),"pool add and duplicate");pending.gas_price=109;check(!pool.add(SignedTransaction::sign(pending,key)),"pool underpriced replacement");pending.gas_price=110;auto replacement=SignedTransaction::sign(pending,key);check(pool.add(replacement)&&!pool.get(hex(first.hash()))&&pool.size()==1,"pool exact ten percent replacement");pending.nonce=1;pending.gas_price=500;auto expensive=SignedTransaction::sign(pending,key);check(pool.add(expensive),"pool second nonce");check(pool.pending_nonce(public_address(pub))==2,"pool pending nonce");check(pool.pending_for_block(21000).front().hash()==expensive.hash(),"pool gas priority and block cap");pending.nonce=2;check(!pool.add(SignedTransaction::sign(pending,key)),"pool per account limit");auto other=SignedTransaction::sign(pending,bytes(Int(2),32));check(pool.add(other)&&!pool.get(hex(replacement.hash()))&&pool.get(hex(expensive.hash())),"pool evicts lowest fee");pool.remove_mined({expensive});check(pool.size()==1&&!pool.add(expensive),"pool remove mined and known hash");pool.clear();check(pool.size()==0&&pool.add(first),"pool clear resets history");
 std::ifstream receipt_file("tests/fixtures/receipts.json");Json receipt_vectors;receipt_file>>receipt_vectors;Receipt receipt;receipt.success=true;receipt.gas_used=21000;receipt.cumulative_gas_used=21000;receipt.logs.push_back({bob,{bytes(Int(1),32)},Bytes{'a','b','c'}});check(hex(receipt.encode())==receipt_vectors["legacy"].get<std::string>(),"receipt RLP and bloom reference");std::vector<Receipt> receipts;for(unsigned type=0;type<3;++type){receipt.type=type;receipts.push_back(receipt);check(Receipt::from_json(receipt.to_json()).encode()==receipt.encode(),"receipt JSON roundtrip");}check(hex(receipts_root(receipts))==receipt_vectors["root"].get<std::string>(),"typed receipts trie reference");auto corrupt=receipt.to_json();corrupt["logsBloom"]=hex(Bytes(256));rejects([&]{Receipt::from_json(corrupt);},"receipt rejects corrupt bloom");
 State chain_state;auto funding=address(public_address(pub));chain_state.get(funding).balance=10000000;ExecutionEngine engine(1337);Block candidate;candidate.header.base_fee=0;candidate.header.coinbase=alice;candidate.header.number=1;Transaction transfer;transfer.to=bob;transfer.value=100;transfer.gas_price=2;candidate.transactions.push_back(SignedTransaction::sign(transfer,key));State before_block=chain_state;auto executed=engine.execute_block(chain_state,candidate,false);check(executed.size()==1&&executed[0].success&&executed[0].gas_used==21000,"engine simple transfer gas");check(chain_state.get(bob).balance==100&&chain_state.get(funding).balance==10000000-42100&&chain_state.get(alice).balance==42000,"engine balances and fees");check(executed[0].block_hash==candidate.header.hash()&&candidate.header.state_root==chain_state.root(),"engine final roots and receipt hash");auto replayed=engine.verify_block(before_block,candidate,false);check(before_block.root()==chain_state.root()&&replayed[0].encode()==executed[0].encode(),"engine deterministic block replay");
 State tamper_state;tamper_state.get(funding).balance=10000000;auto untouched=tamper_state.root();Block tampered=candidate;tampered.header.state_root=Bytes(32);rejects([&]{engine.verify_block(tamper_state,tampered,false);},"engine rejects false commitment");check(tamper_state.root()==untouched,"invalid block rollback");
 Block invalid;invalid.header.base_fee=0;invalid.header.coinbase=alice;invalid.transactions={candidate.transactions[0],candidate.transactions[0]};rejects([&]{engine.execute_block(tamper_state,invalid,false);},"engine rejects duplicate nonce in block");check(tamper_state.root()==untouched,"whole block rollback after second transaction fails");
 chain_state.get(bob).code=unhex("0x606360005560006000fd");transfer.nonce=1;transfer.gas=60000;transfer.value=10;auto failing=SignedTransaction::sign(transfer,key);auto prior_recipient=chain_state.get(bob).balance;auto failed_receipt=engine.apply_transaction(chain_state,failing,candidate.header,0,0);check(!failed_receipt.success&&chain_state.get(bob).balance==prior_recipient&&chain_state.get_storage(bob,0)==0,"engine reverted call value and storage");check(chain_state.get(funding).nonce==2&&failed_receipt.gas_used==43112,"engine reverted call retains nonce and charges only consumed gas");
 auto database=std::filesystem::temp_directory_path()/("nano-store-test-"+std::to_string(getpid())+".rocksdb");State persistent_initial;persistent_initial.get(funding).balance=10000000;Block persistent_genesis;persistent_genesis.header.state_root=persistent_initial.root();persistent_genesis.header.base_fee=0;Block persistent_block; persistent_block.header.number=1;persistent_block.header.timestamp=12;persistent_block.header.base_fee=0;persistent_block.header.coinbase=alice;persistent_block.header.parent_hash=persistent_genesis.header.hash();persistent_block.transactions=candidate.transactions;auto persistent_state=persistent_initial;auto persistent_receipts=engine.execute_block(persistent_state,persistent_block,false);
 {ChainStore db(database);check(db.count()==0&&!db.latest(),"empty database");rejects([&]{ChainStore second(database);},"RocksDB prevents competing database owners");db.commit(persistent_genesis,persistent_initial,{});db.commit(persistent_block,persistent_state,persistent_receipts);check(db.count()==2&&db.total_difficulty()==2,"atomic block persistence");rejects([&]{db.commit(persistent_block,persistent_state,persistent_receipts);},"database rejects duplicate height");check(db.count()==2,"failed database write leaves chain intact");}
 {ChainStore db(database);check(db.latest()->header.hash()==persistent_block.header.hash(),"restart restores chain tip");check(db.state_at(1)->root()==persistent_state.root()&&db.state_at(0)->root()==persistent_initial.root(),"restart restores historical state");auto txhash=persistent_block.transactions[0].hash();check(db.receipt(txhash)->encode()==persistent_receipts[0].encode()&&db.transaction(txhash).has_value(),"restart restores receipt and transaction");check(db.block_by_hash(persistent_genesis.header.hash())->header.number==0&&!db.block_by_number(99),"database lookup indexes");}
 {ChainStore db(database);Block duplicate_tx=persistent_block;duplicate_tx.header.number=2;duplicate_tx.header.parent_hash=persistent_block.header.hash();auto duplicate_receipts=persistent_receipts;for(auto& receipt:duplicate_receipts){receipt.block_number=2;receipt.block_hash=duplicate_tx.header.hash();}rejects([&]{db.commit(duplicate_tx,persistent_state,duplicate_receipts);},"duplicate transaction rejects prepared RocksDB batch");check(!db.block_by_number(2)&&!db.block_by_hash(duplicate_tx.header.hash())&&!db.state_at(2)&&db.count()==2&&db.total_difficulty()==2,"failed batch leaves no partial block state or indexes");}
 {ChainStore db(database);Block with_uncle;with_uncle.header.number=2;with_uncle.header.timestamp=24;with_uncle.header.base_fee=0;with_uncle.header.parent_hash=persistent_block.header.hash();with_uncle.uncles.push_back(persistent_genesis.header);auto uncle_state=persistent_state;auto uncle_receipts=engine.execute_block(uncle_state,with_uncle,true);db.commit(with_uncle,uncle_state,uncle_receipts);auto loaded=db.block_by_number(2);check(loaded&&loaded->uncles.size()==1&&loaded->uncles[0].hash()==persistent_genesis.header.hash(),"persist full uncle headers");}
 std::filesystem::remove_all(database);
 std::cout<<checks<<" checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAILED: "<<e.what()<<"\n";return 1;}}
