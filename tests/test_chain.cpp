#include "nano/chain.hpp"
#include "nano/staking.hpp"
#include "nano/rpc.hpp"
#include <iostream>
#include <unistd.h>
using namespace nano;
static int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f,const char* message){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,message);}
int main(){try{
 auto key=bytes(Int(1),32);auto sender=public_address(public_key(key));auto receiver=address("0x00000000000000000000000000000000000000ab");
 Json data={{"config",{{"chainId",1337},{"consensus","pow"}}},{"alloc",{{sender,{{"balance","0x100000000000000000"}}}}}};
 auto genesis=Genesis::from_json(data);
 auto paris_data=data;paris_data["config"]["evmFork"]="Paris";auto paris_genesis=Genesis::from_json(paris_data);
 check(paris_genesis.evm_revision==EvmRevision::Paris&&paris_genesis.block.header.hash()!=genesis.block.header.hash(),"explicit Paris rules bound to genesis identity");
 for(Json invalid:{Json("Berlin"),Json("Cancun"),Json("Prague"),Json("Osaka"),Json(1),Json(nullptr)}){auto bad=data;bad["config"]["evmFork"]=invalid;rejects([&]{Genesis::from_json(bad);},"unsupported chain execution fork rejected");}

 for(const auto* fork:{"Paris","Shanghai"}){
  auto spec=data;spec["config"]["evmFork"]=fork;auto location=std::filesystem::temp_directory_path()/("nano-nonce-gap-"+std::to_string(getpid())+fork);
  struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{location};
  Blockchain gap(location,Genesis::from_json(spec));
  auto make=[&](unsigned n){Transaction t;t.chain_id=1337;t.to=receiver;t.gas_price=2000000000;t.nonce=n;return SignedTransaction::sign(t,key);};
  auto expect=[&](unsigned pending,unsigned queued){auto status=gap.pool_info(false);check(status==Json{{"pending",pending},{"queued",queued}},"nonce gap pool classification");auto content=gap.pool_info(true);size_t pcount=0,qcount=0;for(const auto& group:content["pending"])pcount+=group.size();for(const auto& group:content["queued"])qcount+=group.size();check(pcount==pending&&qcount==queued,"content and status agree on nonce gaps");};
  check(gap.submit(make(2)),"admit future nonce");expect(0,1);check(gap.pending_nonce(sender)==0,"gap does not advance pending nonce");
  check(gap.submit(make(0)),"admit first nonce");expect(1,1);check(gap.pending_nonce(sender)==1,"pending nonce stops at missing nonce");
  auto first=gap.mine(sender,24);check(first.transactions.size()==1&&first.transactions[0].tx.nonce==0,"future nonce not mined through gap");expect(0,1);
  check(gap.submit(make(1)),"fill nonce gap");expect(2,0);check(gap.pending_nonce(sender)==3,"filled gap promotes contiguous suffix");
  auto second=gap.mine(sender,48);check(second.transactions.size()==2&&second.transactions[0].tx.nonce==1&&second.transactions[1].tx.nonce==2,"promoted suffix executes in nonce order");expect(0,0);check(gap.audit()["verified"]==true,"nonce gap history audit");
 }
 for(const auto* fork:{"Paris","Shanghai"}){
  auto spec=data;spec["config"]["evmFork"]=fork;auto location=std::filesystem::temp_directory_path()/("nano-pool-conflict-"+std::to_string(getpid())+fork);
  struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{location};
  Blockchain local(location/"local",Genesis::from_json(spec)),remote(location/"remote",Genesis::from_json(spec));
  Transaction t;t.chain_id=1337;t.to=receiver;t.gas_price=2000000000;auto original=SignedTransaction::sign(t,key);t.value=2;auto alternative=SignedTransaction::sign(t,key);t.nonce=1;auto next=SignedTransaction::sign(t,key);
  check(local.submit(original)&&local.submit(next)&&remote.submit(alternative),"conflicting nonce fixture admission");auto block=remote.mine(sender,24);
  auto invalid=block;invalid.header.state_root=Bytes(32,0);rejects([&]{local.import_block(invalid,24);},"invalid conflicting block rejected");check(local.pending_count()==2&&local.pending_transaction(original.hash()),"rejected block does not evict pool alternatives");
  local.import_block(block,24);check(!local.pending_transaction(original.hash())&&local.pending_count()==1&&local.pending_transaction(next.hash()),"canonical nonce removes stale alternative and preserves next transaction");
  rejects([&]{local.submit(original);},"stale alternative cannot reenter after canonical nonce change");check(local.pending_nonce(sender)==2,"pending nonce follows remaining suffix");
  auto mined=local.mine(sender,48);check(mined.transactions.size()==1&&mined.transactions[0].hash()==next.hash(),"remaining nonce executes after conflict");check(local.pending_count()==0&&local.audit()["verified"]==true,"conflict cleanup history audit");
 }
 {
  auto location=std::filesystem::temp_directory_path()/("nano-pool-reservation-"+std::to_string(getpid()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{location};Blockchain chain(location,paris_genesis);Int balance=chain.state().get(sender).balance;
  auto make=[&](unsigned nonce,unsigned fraction,unsigned fee){Transaction t;t.chain_id=1337;t.nonce=nonce;t.to=receiver;t.value=balance*fraction/100;t.gas_price=fee;return SignedTransaction::sign(t,key);};
  auto first=make(0,60,2000000000),next=make(1,30,2000000000);check(chain.submit(first),"reserve first transaction cost");rejects([&]{chain.submit(make(1,60,2000000000));},"cumulative unaffordable transaction rejected");check(chain.pending_count()==1&&chain.pending_transaction(first.hash()),"reservation rejection preserves original");check(chain.submit(next),"affordable successor admitted");
  auto replacement=make(0,50,2200000000);check(chain.submit(replacement)&&chain.pending_count()==2,"replacement subtracts original reservation");rejects([&]{chain.submit(make(0,80,2420000000));},"growing replacement respects remaining funds");check(chain.pending_transaction(replacement.hash())&&chain.pending_transaction(next.hash())&&chain.state().get(sender).balance==balance,"failed growing replacement preserves pool and canonical balance");
 }
 {
  auto location=std::filesystem::temp_directory_path()/("nano-future-reservation-"+std::to_string(getpid()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{location};Blockchain chain(location,paris_genesis);Int balance=chain.state().get(sender).balance;
  auto make=[&](unsigned nonce,unsigned fraction){Transaction t;t.chain_id=1337;t.nonce=nonce;t.to=receiver;t.value=balance*fraction/100;t.gas_price=2000000000;return SignedTransaction::sign(t,key);};
  check(chain.submit(make(10,60))&&chain.submit(make(20,60)),"future entries do not reserve contiguous pending budget");check(chain.submit(make(0,20)),"future costs do not block missing first nonce");check(chain.pool_info(false)==Json{{"pending",1},{"queued",2}},"future admission preserves gap classification");
 }
 for(const auto* fork:{"Paris","Shanghai"})for(unsigned type:{0u,1u,2u}){
  auto spec=data;spec["config"]["evmFork"]=fork;spec["config"]["consensus"]="pos";spec["config"]["posSignatureVersion"]=1;spec["config"]["bootstrapValidator"]=sender;spec["alloc"][sender]["balance"]=quantity(Int("1000000000000000000000000"));
  auto location=std::filesystem::temp_directory_path()/("nano-pool-balance-refresh-"+std::to_string(getpid())+fork+std::to_string(type));struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{location};Blockchain chain(location,Genesis::from_json(spec));Int balance=chain.state().get(sender).balance;
  auto make=[&](unsigned nonce,unsigned fraction){Transaction t;t.type=type;t.chain_id=1337;t.nonce=nonce;t.to=receiver;t.value=balance*fraction/100;t.gas_price=2000000000;t.max_fee=2000000000;t.priority_fee=1000000000;return SignedTransaction::sign(t,key);};
  auto first=make(0,60),expensive=make(1,60),suffix=make(2,5);check(chain.submit(expensive)&&chain.submit(suffix)&&chain.submit(first),"admit gap then overcommitted promotion");
  auto block=chain.validate_signed(key,24);check(block.transactions.size()==1&&block.transactions[0].hash()==first.hash()&&chain.state().get(receiver).balance==balance*60/100&&!chain.store().receipt(expensive.hash()),"execution excludes unfunded suffix without spending funds");
  check(!chain.pending_transaction(expensive.hash())&&chain.pending_transaction(suffix.hash())&&chain.pending_count()==1,"post-block balance refresh removes unaffordable entry and retains affordable future nonce");check(chain.pool_info(false)==Json{{"pending",0},{"queued",1}},"remaining suffix queued behind removed nonce");
  auto affordable=make(1,5);check(chain.submit(affordable),"cheaper nonce admitted after obsolete entry removed");auto resumed=chain.validate_signed(key,48);check(resumed.transactions.size()==2&&resumed.transactions[0].hash()==affordable.hash()&&resumed.transactions[1].hash()==suffix.hash(),"affordable gap fill resumes suffix in order");check(chain.state().get(receiver).balance==balance*70/100&&chain.pending_count()==0&&chain.audit()["verified"]==true,"post-refresh balances and history audit");
 }
 auto path=std::filesystem::temp_directory_path()/("nano-chain-"+std::to_string(getpid()));auto peer=path.string()+"-peer";
 {
  auto shanghai_data=data;shanghai_data["config"]["evmFork"]="Shanghai";auto rules=Genesis::from_json(shanghai_data);
  check(rules.evm_revision==EvmRevision::Shanghai&&rules.block.header.hash()!=paris_genesis.block.header.hash()&&rules.block.header.hash()!=genesis.block.header.hash(),"Shanghai has distinct genesis identity");
  auto location=path.string()+"-shanghai";auto replica=path.string()+"-shanghai-peer";
  struct Cleanup{std::string a,b;~Cleanup(){std::filesystem::remove_all(a);std::filesystem::remove_all(b);}} cleanup{location,replica};
  {Blockchain chain(location,rules),follower(replica,rules);Rpc rpc(chain);
   Transaction large;large.chain_id=1337;large.to=receiver;large.gas_price=2000000000;large.data=Bytes(1024*1024,0);large.gas=large.intrinsic_gas();
   auto encoded=SignedTransaction::sign(large,key).raw();
   auto decoded=SignedTransaction::decode(encoded);check(decoded.raw()==encoded,"large valid envelope must decode independently of pool policy");
   bool pool_limit=false;try{chain.submit(decoded);}catch(const std::invalid_argument& e){pool_limit=std::string(e.what())=="transaction exceeds Nano pool byte limit";}
   check(pool_limit&&chain.pending_count()==0&&chain.pending_nonce(sender)==0,"large pool admission rejection preserves pending state");
   auto large_rpc=rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method","eth_sendRawTransaction"},{"params",Json::array({hex(encoded)})}});
   check(large_rpc.contains("error")&&large_rpc.at("error").at("message")=="transaction exceeds Nano pool byte limit","RPC retains early pool-size rejection");
   auto invoke=[&](const std::string& method,const Json& call){return rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",Json::array({call})}});};
   Transaction tx;tx.chain_id=1337;tx.gas_price=2000000000;tx.gas=1000000;tx.data=Bytes(49153,0);
   auto call=Json{{"from",sender},{"data",hex(tx.data)},{"gas",quantity(tx.gas)}};auto before=chain.state().root();
   rejects([&]{chain.submit(SignedTransaction::sign(tx,key));},"oversized initcode rejected by Shanghai mempool");
   check(invoke("eth_call",call).contains("error")&&invoke("eth_estimateGas",call).contains("error"),"oversized initcode rejected by both simulation RPCs");
   check(chain.pending_count()==0&&chain.state().root()==before,"oversized creation leaves state and pool unchanged");
   tx.data.resize(49152);auto intrinsic=transaction_intrinsic_gas(tx,EvmRevision::Shanghai);call["data"]=hex(tx.data);
   auto estimate=invoke("eth_estimateGas",call);check(estimate.contains("result")&&quantity(estimate["result"])==intrinsic,"maximum initcode estimate includes exact word charge");
   tx.gas=intrinsic-1;rejects([&]{chain.submit(SignedTransaction::sign(tx,key));},"mempool includes Shanghai initcode charge");
   tx.gas=intrinsic;auto signed_tx=SignedTransaction::sign(tx,key);check(chain.submit(signed_tx),"maximum initcode accepted at exact gas budget");
   auto block=chain.mine(receiver,12);follower.import_block(block,12);auto receipt=chain.store().receipt(signed_tx.hash());
   check(receipt&&receipt->success&&receipt->gas_used==intrinsic&&chain.state().root()==follower.state().root(),"maximum initcode executes with estimated gas and matching peer state");
   tx.nonce=1;tx.data=unhex("5f5ff3");tx.gas=100000;call["data"]=hex(tx.data);
   estimate=invoke("eth_estimateGas",call);check(quantity(estimate.at("result"))==transaction_intrinsic_gas(tx,EvmRevision::Shanghai)+4,"RPC executes PUSH0 creation with Shanghai gas");
   signed_tx=SignedTransaction::sign(tx,key);check(chain.submit(signed_tx),"PUSH0 creation submitted");block=chain.mine(receiver,24);follower.import_block(block,24);receipt=chain.store().receipt(signed_tx.hash());
   check(receipt&&receipt->success&&receipt->gas_used==quantity(estimate.at("result"))&&chain.state().root()==follower.state().root(),"PUSH0 receipt agrees with estimation and peer execution");
  }
  rejects([&]{Blockchain wrong(location,paris_genesis);},"Shanghai database cannot reopen as Paris");
  {Blockchain reopened(location,rules);check(reopened.tip().header.number==2&&reopened.audit().at("verified")==true,"Shanghai history survives reopen and audit");}
 }
 Block mined;Bytes root;
 {Blockchain a(path,genesis),b(peer,genesis);
  check(a.tip().header.number==0&&a.state().root()==genesis.state.root(),"genesis initialization");
  Transaction tx;tx.chain_id=1337;tx.to=receiver;tx.value=123;tx.gas_price=2000000000;tx.nonce=1;auto second=SignedTransaction::sign(tx,key);check(a.submit(second),"queue future nonce");
  tx.nonce=0;tx.gas_price=1000000000;auto first=SignedTransaction::sign(tx,key);check(a.submit(first),"queue first nonce");
  auto wrong=tx;wrong.chain_id=999;rejects([&]{a.submit(SignedTransaction::sign(wrong,key));},"reject wrong network");
  mined=a.mine(receiver,24);root=a.state().root();
  check(mined.transactions.size()==2&&mined.transactions[0].tx.nonce==0&&mined.transactions[1].tx.nonce==1,"nonce order overrides fee order");
  check(a.pending_count()==0&&a.state().get(sender).nonce==2,"mined transactions removed");
  auto forged=mined;forged.header.state_root=Bytes(32);rejects([&]{b.import_block(forged,24);},"reject forged execution root");
  check(b.tip().header.number==0&&b.store().count()==1&&b.state().root()==genesis.state.root(),"invalid import leaves disk and memory intact");
  b.import_block(mined,24);check(b.state().root()==root&&b.tip().header.hash()==mined.header.hash(),"peer replay agrees with producer");
  check(b.store().receipt(second.hash())->block_hash==mined.header.hash(),"receipts use sealed block hash");
  rejects([&]{a.submit(first);},"reject mined nonce");
 }
 {Blockchain reopened(path,genesis);check(reopened.tip().header.hash()==mined.header.hash()&&reopened.state().root()==root,"restart restores canonical state");}
 rejects([&]{Blockchain wrong_fork(path,paris_genesis);},"execution rules mismatch rejected on reopen");
 auto wrong_genesis=genesis;wrong_genesis.chain_id=999;
 rejects([&]{Blockchain wrong(path,wrong_genesis);},"same genesis different chain ID rejected");
 wrong_genesis=genesis;wrong_genesis.consensus="pos";
 rejects([&]{Blockchain wrong(path,wrong_genesis);},"different consensus rejected");
 std::filesystem::remove_all(path);std::filesystem::remove_all(peer);

 {auto hash_data=data;hash_data["alloc"][receiver]={{"code","0x60004060005500"}};
  auto hash_genesis=Genesis::from_json(hash_data);
  {Blockchain producer(path,hash_genesis),consumer(peer,hash_genesis);
   Transaction tx;tx.to=receiver;tx.gas=100000;tx.gas_price=2000000000;
   auto signed_tx=SignedTransaction::sign(tx,key);check(producer.submit(signed_tx),"submit BLOCKHASH contract transaction");
   auto block=producer.mine(sender,24);consumer.import_block(block,24);
   check(producer.state().get_storage(receiver,0)==integer(hash_genesis.block.header.hash()),"mined contract stores actual ancestor hash");
   check(consumer.state().root()==producer.state().root(),"peer replay resolves identical ancestor hash");
  }
  {Blockchain reopened(path,hash_genesis);check(reopened.state().get_storage(receiver,0)==integer(hash_genesis.block.header.hash()),"BLOCKHASH derived state survives restart");}
  std::filesystem::remove_all(path);std::filesystem::remove_all(peer);
 }

 { // Test-only contract dispatching the three staking view selectors.
  Bytes code;std::vector<size_t> patches;
  auto append=[&](const std::string& hexcode){auto b=unhex(hexcode);code.insert(code.end(),b.begin(),b.end());};
  append("60003560e01c");
  for(const auto& selector:{"b7ab4db5","7a766460","facd743b"}){append("8063"+std::string(selector)+"1461");patches.push_back(code.size());append("000057");}
  append("5060003560015500");
  auto label=[&](size_t index){code[patches[index]]=uint8_t(code.size()>>8);code[patches[index]+1]=uint8_t(code.size());append("5b50");};
  label(0);append("6020600052600160205273"+address(sender).substr(2)+"60405260606000f3");
  label(1);append("60015460005260206000f3");
  label(2);append("60005460005260206000f3");
  auto staking_data=data;staking_data["config"]["consensus"]="pos";staking_data["config"]["stakingContract"]=receiver;
  staking_data["alloc"][receiver]={{"code",hex(code)},{"storage",{{"0x0","0x1"},{"0x1",quantity(ProofOfStake::minimum_stake)}}}};
  auto staking_genesis=Genesis::from_json(staking_data);
  auto registry=sync_staking_contract(staking_genesis.state,staking_genesis.block.header,receiver,ProofOfStake{});
  check(!registry.bootstrap&&registry.total_stake()==ProofOfStake::minimum_stake&&registry.select(0,Bytes(32))==address(sender),"staking contract selects funded validator");
  auto updated=staking_genesis.state;updated.set_storage(receiver,1,ProofOfStake::minimum_stake*2);
  auto changed=sync_staking_contract(updated,staking_genesis.block.header,receiver,registry);
  check(changed.total_stake()==ProofOfStake::minimum_stake*2,"staking contract refreshes changed stake");
  updated.set_storage(receiver,0,0);auto disabled=sync_staking_contract(updated,staking_genesis.block.header,receiver,changed);
  check(!disabled.select(0,Bytes(32)),"inactive contract validator disables bootstrap fallback");
  updated.get(receiver).code=unhex("6020600052600060205260406000f3");
  auto removed=sync_staking_contract(updated,staking_genesis.block.header,receiver,registry);check(removed.validators().empty()&&!removed.select(0,Bytes(32)),"removed contract validators do not remain cached");
  updated.get(receiver).code=unhex("600160005260206000f3");
  rejects([&]{sync_staking_contract(updated,staking_genesis.block.header,receiver,registry);},"malformed contract ABI rejected");
  updated.get(receiver).code=unhex("600160005500");
  rejects([&]{sync_staking_contract(updated,staking_genesis.block.header,receiver,registry);},"staking view cannot mutate state");
  {Blockchain producer(path,staking_genesis),consumer(peer,staking_genesis);
   check(producer.stake(sender)==ProofOfStake::minimum_stake,"chain reads stake from contract");
   check(producer.next_validator()==address(sender),"chain initializes contract validator at genesis");
   rejects([&]{producer.validate(ProofOfStake::bootstrap_validator,24);},"deployed staking contract disables bootstrap producer");
   Transaction update;update.to=receiver;update.gas=100000;update.gas_price=2000000000;update.data=bytes(Int(ProofOfStake::minimum_stake*2),32);
   check(producer.submit(SignedTransaction::sign(update,key)),"submit contract stake update");
   auto block=producer.validate(sender,24);consumer.import_block(block,24);
   check(producer.stake(sender)==ProofOfStake::minimum_stake*2&&consumer.stake(sender)==producer.stake(sender),"stake follows committed and replayed contract update");
   check(producer.validators()==consumer.validators()&&producer.state().root()==consumer.state().root(),"staking registry and state agree after peer replay");
   check(quantity(producer.validators()[0]["stake"])==ProofOfStake::minimum_stake*2&&producer.validators()[0]["blocks_validated"]=="0x1","contract controls stake while validation counters persist");
   check(producer.restore(0,staking_genesis.block.header.hash())&&producer.validators()[0]["blocks_validated"]=="0x0"&&quantity(producer.validators()[0]["stake"])==ProofOfStake::minimum_stake,"snapshot restore rewinds contract state and PoS registry together");
   check(producer.submit(SignedTransaction::sign(update,key)),"reverted transaction can be resubmitted");producer.validate(sender,36);

  }
  {Blockchain reopened(path,staking_genesis);check(reopened.next_validator()==address(sender)&&reopened.validators()[0]["blocks_validated"]=="0x1","staking registry survives restart");}
  auto wrong=staking_genesis;wrong.staking_contract.clear();rejects([&]{Blockchain other(path,wrong);},"database rejects changed staking configuration");
  std::filesystem::remove_all(path);std::filesystem::remove_all(peer);
 }
 ProofOfStake registry;registry.bootstrap=false;
 check(registry.register_validator(sender,ProofOfStake::minimum_stake,5),"register validator for snapshot");
 registry.add_stake(sender,123,8);registry.reward(sender,9);
 auto restored=ProofOfStake::from_json(registry.to_json());
 check(restored.to_json()==registry.to_json()&&restored.select(9,Bytes(32))==registry.select(9,Bytes(32)),"validator snapshot preserves selection and rewards");
 check(!restored.withdraw(sender,107)&&restored.withdraw(sender,108)==std::optional<Int>(ProofOfStake::minimum_stake+123+ProofOfStake::block_reward),"snapshot preserves stake lock and withdrawal amount");
 auto corrupt=registry.to_json();corrupt["validators"].push_back(corrupt["validators"][0]);
 rejects([&]{ProofOfStake::from_json(corrupt);},"duplicate validator snapshot rejected");
 auto pos_genesis=genesis;pos_genesis.consensus="pos";
 Block pos_block;
 {Blockchain a(path,pos_genesis),b(peer,pos_genesis);
  check(a.next_validator()==std::optional<std::string>(ProofOfStake::bootstrap_validator),"bootstrap producer selection");
  rejects([&]{a.validate(receiver,12);},"wrong PoS producer rejected");
  rejects([&]{a.mine(receiver,12);},"cannot mine PoS chain");
  pos_block=a.validate(ProofOfStake::bootstrap_validator,12);
  check(pos_block.header.difficulty==1&&pos_block.header.nonce==Bytes(8)&&pos_block.header.mix_hash==Bytes(32),"PoS header seal");
  check(a.state().get(ProofOfStake::bootstrap_validator).balance==ProofOfStake::block_reward,"PoS reward committed once in state root");
  auto invalid=pos_block;invalid.header.coinbase=receiver;rejects([&]{b.import_block(invalid,12);},"import rejects wrong producer");
  invalid=pos_block;invalid.header.nonce[0]=1;rejects([&]{b.import_block(invalid,12);},"import rejects invalid PoS seal");
  invalid=pos_block;invalid.header.state_root=Bytes(32);rejects([&]{b.import_block(invalid,12);},"PoS rejects state root mismatch");
  check(b.store().count()==1&&!b.store().consensus_at(1),"rejected PoS import leaves registry and chain unchanged");
  b.import_block(pos_block,12);
  check(a.state().root()==b.state().root()&&a.store().consensus_at(1)==b.store().consensus_at(1),"PoS peer state and registry agree");
 }
 {Blockchain restored_chain(path,pos_genesis);check(restored_chain.tip().header.hash()==pos_block.header.hash()&&restored_chain.state().root()==pos_block.header.state_root,"PoS restart restores canonical state");
  check(restored_chain.next_validator()==std::optional<std::string>(ProofOfStake::bootstrap_validator),"PoS selection survives restart");
 }
 std::filesystem::remove_all(path);std::filesystem::remove_all(peer);

 {ChainStore seeded(path);seeded.bind_chain(genesis.chain_id,genesis.block.header.hash(),"pos");seeded.commit(genesis.block,genesis.state,{},registry.to_json());}
 {Blockchain registered(path,pos_genesis);
  check(registered.stake(sender)==registry.total_stake()&&registered.stake(receiver)==0,"registry stake restores without contract");
  check(registered.next_validator()==std::optional<std::string>(sender),"restored registered validator supersedes bootstrap");
  auto block=registered.validate(sender,12);
  auto saved=ProofOfStake::from_json(*registered.store().consensus_at(1));
  check(saved.total_stake()==registry.total_stake()+ProofOfStake::block_reward,"registered validator reward persisted with block");
 }
 {Blockchain reopened(path,pos_genesis);check(reopened.validators()[0]["blocks_validated"]=="0x2","validator counters restored after reward");}
 std::filesystem::remove_all(path);
 std::cout<<checks<<" chain checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
