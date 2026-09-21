#include <rocksdb/db.h>
#include "nano/rpc.hpp"
#include "nano/staking.hpp"
#include "nano/block_auth.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
using namespace nano;
static unsigned checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
 check(argc==1||(argc==2&&std::string(argv[1])=="--paris"),"expected optional --paris");
 auto parse_genesis=[&](Json data){if(argc==2)data["config"]["evmFork"]="Paris";return Genesis::from_json(data);};
 std::ifstream file("contracts/NanoStaking.json");Json artifact;file>>artifact;
 auto first_key=bytes(Int(1),32),second_key=bytes(Int(2),32);
 auto first=public_address(public_key(first_key)),second=public_address(public_key(second_key));
 auto target=create_address(first,0);Int minimum=ProofOfStake::minimum_stake;
 auto root=std::filesystem::temp_directory_path()/("nano-staking-"+std::to_string(getpid()));
 std::filesystem::create_directory(root);
 auto genesis=parse_genesis({{"config",{{"chainId",1337}}},{"alloc",{{first,{{"balance",quantity(Int(minimum*20))}}},{second,{{"balance",quantity(Int(minimum*20))}}}}}});
 // Signed genesis starts without contract code. Deploy it through the first
 // signed transaction, then derive the validator set from its executed state.
 auto signed_genesis=parse_genesis({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",first},{"stakingContract",target}}},{"alloc",{{first,{{"balance",quantity(Int(minimum*20))}}},{second,{{"balance",quantity(Int(minimum*20))}}}}}});
 Block recovery_proposal;
 {Blockchain producer(root/"signed-producer",signed_genesis),peer(root/"signed-peer",signed_genesis);ChainStore voting(root/"certified-journal");
  auto certificate=[&](const Block& block,const VotingSet& members){std::vector<FinalityVote> result;for(const auto& [signer,power]:members){
   FinalityVote vote;vote.height=u64(block.header.number);vote.round=0;vote.step=VoteStep::precommit;vote.validator_set_hash=voting_set_hash(members);vote.block_hash=block.header.hash();vote.validator=signer;
   sign_vote(vote,address(signer)==address(first)?first_key:second_key,1337,signed_genesis.block.header.hash());result.push_back(vote);
  }return result;};
  auto certify=[&](const Block& block){auto members=peer.voting_set();auto commits=certificate(block,members);auto prevotes=commits;
   for(auto& vote:prevotes){vote.step=VoteStep::prevote;sign_vote(vote,address(vote.validator)==address(first)?first_key:second_key,1337,signed_genesis.block.header.hash());}
   auto prevote=peer.vote_for_block(voting,first_key,block,0,VoteStep::prevote,{},block.header.timestamp);
   check(verify_vote(prevote,1337,signed_genesis.block.header.hash()),"engine signs prevote only after replay");
   auto commit=peer.vote_for_block(voting,first_key,block,0,VoteStep::precommit,prevotes,block.header.timestamp);
   for(auto& vote:commits)if(address(vote.validator)==address(first))vote=commit;
   return peer.certify_voting_progress(voting,first,block,0,commits,block.header.timestamp);
  };
  check(producer.validators().empty()&&producer.next_validator()==address(first),"signed bootstrap active before deployment");
  Transaction deployment;deployment.gas=3000000;deployment.gas_price=2000000000;deployment.value=minimum;deployment.data=unhex(artifact.at("bytecode"));
  auto signed_tx=SignedTransaction::sign(deployment,first_key);check(producer.submit(signed_tx),"signed bootstrap deployment queued");
  auto first_block=producer.validate_signed(first_key,12);
  auto forged=first_block;forged.header.state_root[0]^=1;sign_block(forged,first_key,1337,signed_genesis.block.header.hash());bool refused=false;
  try{peer.certify_voting_progress(voting,first,forged,0,certificate(forged,peer.voting_set()),12);}catch(const std::exception& e){refused=std::string(e.what()).find("block execution commitments mismatch")!=std::string::npos;}check(refused,"valid quorum cannot certify incorrect execution commitments");
  refused=false;try{peer.vote_for_block(voting,first_key,forged,0,VoteStep::prevote,{},12);}catch(const std::exception& e){refused=std::string(e.what()).find("block execution commitments mismatch")!=std::string::npos;}
  check(refused,"valid producer signature cannot authorize vote for false state root");
  auto unsigned_block=first_block;unsigned_block.producer_signature.clear();refused=false;
  try{peer.vote_for_block(voting,first_key,unsigned_block,0,VoteStep::prevote,{},12);}catch(const std::exception&){refused=true;}check(refused,"engine refuses vote for unauthenticated producer");
  refused=false;try{peer.vote_for_block(voting,second_key,first_block,0,VoteStep::prevote,{},12);}catch(const std::exception&){refused=true;}check(refused,"future validator cannot vote before registration");
  refused=false;try{peer.vote_for_block(voting,first_key,first_block,0,VoteStep::precommit,{},12);}catch(const std::exception&){refused=true;}check(refused,"valid execution does not bypass precommit quorum requirement");
  FinalityVote saved;
  {ChainStore restart(root/"vote-restart");saved=peer.vote_for_block(restart,first_key,first_block,0,VoteStep::prevote,{},12);}
  {ChainStore restart(root/"vote-restart");auto stored=restart.voting_proposal(first,1,first_block.header.hash(),1337,signed_genesis.block.header.hash());check(stored&&stored->to_json(true)==first_block.to_json(true),"verified voting proposal survives journal reopen");auto resumed=peer.vote_for_block(restart,first_key,first_block,0,VoteStep::prevote,{},12);check(saved.signature==resumed.signature,"engine replays proposal and resumes exact vote after journal reopen");}
  {ChainStore mismatch(root/"proposal-mismatch");auto bad=first_block;bad.producer_signature[0]^=1;auto unsigned_vote=saved;unsigned_vote.signature.clear();bool refused=false;
   try{sign_consensus_vote(mismatch,unsigned_vote,first_key,peer.voting_set(),{},1337,signed_genesis.block.header.hash(),&bad);}catch(const std::exception&){refused=true;}
   check(refused&&unsigned_vote.signature.empty()&&!mismatch.latest_consensus_vote(first,1337,signed_genesis.block.header.hash()),"invalid proposal creates neither vote reservation nor signature");
   check(!mismatch.voting_proposal(first,1,first_block.header.hash(),1337,signed_genesis.block.header.hash()),"invalid proposal is not cached");}
  FinalityVote nil_prevote;
  {ChainStore nil_journal(root/"nil-journal");
   bool rejected=false;try{peer.vote_nil(nil_journal,second_key,0,VoteStep::prevote);}catch(const std::exception&){rejected=true;}check(rejected,"nonmember cannot sign nil vote through engine");
   nil_prevote=peer.vote_nil(nil_journal,first_key,0,VoteStep::prevote);
   auto nil_commit=peer.vote_nil(nil_journal,first_key,0,VoteStep::precommit);
   check(!nil_prevote.block_hash&&!nil_commit.block_hash&&nil_commit.height==1&&verify_vote(nil_commit,1337,signed_genesis.block.header.hash()),"engine derives nil vote domain from canonical parent");
   check(!verify_commit_certificate({nil_commit},peer.voting_set(),1,0,first_block.header.hash(),1337,signed_genesis.block.header.hash()),"nil precommit cannot certify proposed block");
  }
  {ChainStore nil_journal(root/"nil-journal");auto resumed=peer.vote_nil(nil_journal,first_key,0,VoteStep::prevote);check(resumed.signature==nil_prevote.signature,"nil signature resumes exactly after reopen");
   auto prevote=peer.vote_for_block(nil_journal,first_key,first_block,1,VoteStep::prevote,{},12);
   peer.vote_for_block(nil_journal,first_key,first_block,1,VoteStep::precommit,{prevote},12);
   peer.vote_nil(nil_journal,first_key,2,VoteStep::precommit);
  }
  {ChainStore nil_journal(root/"nil-journal");bool rejected=false;try{peer.vote_nil(nil_journal,first_key,3,VoteStep::prevote);}catch(const std::exception&){rejected=true;}check(rejected,"nil precommit without quorum preserved block lock across restart");
   auto retained=peer.vote_for_block(nil_journal,first_key,first_block,3,VoteStep::prevote,{},12);check(retained.block_hash==first_block.header.hash(),"locked block remains votable after nil precommit");
  }
  auto premature=VotingSet{{first,minimum}};refused=false;
  try{peer.certify_voting_progress(voting,first,first_block,0,certificate(first_block,premature),12);}catch(const std::exception&){refused=true;}check(refused,"post-deployment weights cannot vote for deployment retroactively");
  refused=false;try{peer.import_certified_block(forged,0,certificate(forged,peer.voting_set()),12);}catch(const std::exception&){refused=true;}check(refused&&!peer.store().certificate_at(1)&&peer.store().count()==1,"invalid execution writes neither block nor certificate");
  refused=false;try{peer.import_certified_block(first_block,0,{},12);}catch(const std::exception&){refused=true;}check(refused&&peer.tip().header.number==0&&!peer.store().certificate_at(1),"missing quorum leaves canonical state unchanged");
  {ChainStore wrong(root/"ahead-commit-journal");auto members=peer.voting_set();wrong.advance_voting_height(first,1,0,first_block.header.hash(),certificate(first_block,members),members,{{first,minimum}},1337,signed_genesis.block.header.hash());
   bool rejected=false;try{peer.commit_certified_round(wrong,first,first_block,0,certificate(first_block,members),12);}catch(const std::exception&){rejected=true;}check(rejected&&peer.tip().header.number==0&&!peer.store().certificate_at(1),"coordinated commit rejects ahead journal before canonical mutation");}
  {Blockchain coordinated(root/"coordinated",signed_genesis);ChainStore fresh(root/"coordinated-journal");
   coordinated.commit_certified_round(fresh,first,first_block,0,certificate(first_block,coordinated.voting_set()),12);
   auto position=fresh.voting_position(first,1337,signed_genesis.block.header.hash());check(coordinated.tip().header.number==1&&coordinated.store().certificate_at(1)&&position&&position->height==2&&position->validator_set_hash==voting_set_hash(coordinated.voting_set()),"coordinated commit stores block then enables derived next voting set");}
  auto deployed_set=certify(first_block);check(deployed_set.size()==1&&deployed_set[0].second==minimum,"certification derives deposited stake from block execution");
  check(peer.tip().header.number==0,"certifying voting progression does not publish canonical block");peer.import_certified_block(first_block,0,certificate(first_block,peer.voting_set()),12);
  {ChainStore nil_journal(root/"nil-journal");bool rejected=false;try{peer.vote_nil(nil_journal,first_key,0,VoteStep::prevote);}catch(const std::exception&){rejected=true;}check(rejected,"nil vote cannot skip uncertified journal height after canonical import");}
  check(producer.store().receipt(signed_tx.hash())->success&&producer.validators().size()==1&&producer.stake(first)==minimum,"contract deployment activates first funded validator");
  check(producer.state().root()==peer.state().root()&&producer.validators()==peer.validators(),"peer replays signed bootstrap transition");
  Transaction registration;registration.to=target;registration.gas=300000;registration.gas_price=2000000000;registration.value=minimum;
  registration.data=unhex(artifact.at("methodIdentifiers").at("registerValidator()"));
  auto registered=SignedTransaction::sign(registration,second_key);check(producer.submit(registered),"second validator registration queued");
  auto next=producer.validate_signed(first_key,24);
  refused=false;try{peer.import_block(next,24);}catch(const std::exception&){refused=true;}check(refused&&peer.tip().header.number==1&&!peer.store().block_by_number(2),"ordinary import cannot bypass certified chain mode");
  auto expanded=certify(next);check(expanded.size()==2,"new validator joins only the derived next voting set");peer.import_certified_block(next,0,certificate(next,peer.voting_set()),24);
  check(producer.store().receipt(registered.hash())->success&&producer.validators().size()==2,"signed registration changes consensus set after inclusion");
  for(unsigned i=0;i<8;++i){auto selected=producer.next_validator();check(selected.has_value(),"contract registry selects signed producer");auto timestamp=Int(producer.tip().header.timestamp+12);
   auto next=producer.validate_signed(address(*selected)==address(first)?first_key:second_key,timestamp);certify(next);peer.import_certified_block(next,0,certificate(next,peer.voting_set()),timestamp);
   check(producer.state().root()==peer.state().root()&&producer.validators()==peer.validators(),"signed contract validators agree across peers");}
  refused=false;try{peer.restore(0,signed_genesis.block.header.hash());}catch(const std::exception&){refused=true;}check(refused&&peer.tip().header.number==10,"rewind cannot erase certified history");
  check(peer.store().certificate_at(10)->votes.size()==2,"canonical certificate stores both validator signatures");
  check(producer.audit().at("verified")==true&&peer.audit().at("verified")==true,"signed staking history fully replays from genesis");
  auto selected=peer.next_validator();recovery_proposal=peer.block_template(*selected,132);
  sign_block(recovery_proposal,address(*selected)==address(first)?first_key:second_key,1337,signed_genesis.block.header.hash());
  {Blockchain behind(root/"behind",signed_genesis);bool refused=false;try{behind.recover_voting_progress(voting,first);}catch(const std::exception&){refused=true;}check(refused,"recovery refuses journal ahead of canonical chain");}
  {ChainStore empty(root/"uncertified-recovery");bool refused=false;try{producer.recover_voting_progress(empty,first);}catch(const std::exception&){refused=true;}check(refused&&!empty.voting_position(first,1337,signed_genesis.block.header.hash()),"uncertified history cannot authorize voting recovery");}
  {ChainStore conflicting(root/"conflicting-recovery");auto proof=certificate(first_block,VotingSet{{first,1}});auto other_hash=keccak(Bytes{42});proof[0].block_hash=other_hash;sign_vote(proof[0],first_key,1337,signed_genesis.block.header.hash());
   conflicting.advance_voting_height(first,1,0,other_hash,proof,{{first,1}},{{first,minimum}},1337,signed_genesis.block.header.hash());
   bool refused=false;try{peer.recover_voting_progress(conflicting,first);}catch(const std::exception&){refused=true;}check(refused&&conflicting.voting_position(first,1337,signed_genesis.block.header.hash())->height==2,"recovery rejects conflicting journal certificate without advancing");}

 }
 {Blockchain reopened(root/"signed-peer",signed_genesis);check(reopened.validators().size()==2&&reopened.stake(second)==minimum,"signed staking registry survives reopen");
  check(reopened.store().certificate_at(1).has_value()&&reopened.store().certificate_at(10).has_value()&&reopened.audit().at("verified")==true,"certified history and replay survive reopen");
  {ChainStore journal(root/"vote-restart");bool refused=false;try{reopened.vote_for_block(journal,first_key,recovery_proposal,0,VoteStep::prevote,{},132);}catch(const std::exception&){refused=true;}check(refused,"lagging journal refuses next vote before recovery");
   reopened.recover_voting_progress(journal,first);reopened.recover_voting_progress(journal,first);
   auto vote=reopened.vote_for_block(journal,first_key,recovery_proposal,0,VoteStep::prevote,{},132);check(vote.height==11&&verify_vote(vote,1337,signed_genesis.block.header.hash()),"audited recovery resumes voting after canonical commit without journal progression");}
  {ChainStore journal(root/"vote-restart");reopened.recover_voting_progress(journal,first);auto vote=reopened.vote_for_block(journal,first_key,recovery_proposal,0,VoteStep::prevote,{},132);check(verify_vote(vote,1337,signed_genesis.block.header.hash()),"recovered journal reopens and resumes exact vote");}
  {ChainStore observer(root/"new-validator-journal");reopened.recover_voting_progress(observer,second);auto vote=reopened.vote_for_block(observer,second_key,recovery_proposal,0,VoteStep::prevote,{},132);check(vote.height==11,"new validator catches up certified history before first vote");}
 }
 // Each single-key deletion must fail closed, including direct storage rewind.
 for(const auto& key:std::vector<std::string>{"meta/certified-tip","meta/tip","certificate/"+hex(bytes(Int(10),8))}){
  std::string saved;
  {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(rocksdb::DB::Open(options,(root/"signed-peer").string(),&db).ok(),"open metadata fault fixture");
   check(db->Get(rocksdb::ReadOptions(),key,&saved).ok(),"save metadata fixture");rocksdb::WriteOptions write;write.sync=true;check(db->Delete(write,key).ok(),"delete single certified metadata key");}
  {ChainStore store(root/"signed-peer");bool refused=false;try{store.latest();}catch(const std::exception&){refused=true;}check(refused,"missing certified metadata refuses latest rather than downgrading");
   refused=false;try{store.rewind(0);}catch(const std::exception&){refused=true;}check(refused&&store.block_by_number(10).has_value(),"missing metadata cannot permit deletion of certified history");
   refused=false;try{store.certificate_at(10);}catch(const std::exception&){refused=true;}check(refused,"certificate reader rejects incomplete metadata");}
  {bool refused=false;try{Blockchain reopened(root/"signed-peer",signed_genesis);}catch(const std::exception&){refused=true;}check(refused,"chain reopen fails closed on incomplete certified metadata");}
  {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(rocksdb::DB::Open(options,(root/"signed-peer").string(),&db).ok(),"restore metadata fixture");rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,key,saved).ok(),"restore exact metadata value");}
 }
 {Blockchain reopened(root/"signed-peer",signed_genesis);check(reopened.audit().at("verified")==true,"refused rewind left all canonical data intact");}
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(rocksdb::DB::Open(options,(root/"signed-peer").string(),&db).ok(),"open canonical certificate corruption fixture");
  auto key="certificate/"+hex(bytes(Int(10),8));std::string encoded;check(db->Get(rocksdb::ReadOptions(),key,&encoded).ok(),"read certificate fixture");auto record=Json::parse(encoded);
  record["precommits"][0]["signature"]=hex(Bytes(65,0));rocksdb::WriteOptions write;write.sync=true;check(db->Put(write,key,record.dump()).ok(),"corrupt persisted certificate signature");}
 {bool refused=false;try{Blockchain reopened(root/"signed-peer",signed_genesis);}catch(const std::exception&){refused=true;}check(refused,"reopen rejects corrupted certified tip signature");}
 State deployed;
 {Blockchain chain(root/"deployment",genesis);
  Transaction deployment;deployment.gas=3000000;deployment.gas_price=2000000000;deployment.value=minimum;deployment.data=unhex(artifact.at("bytecode"));
  auto signed_tx=SignedTransaction::sign(deployment,first_key);check(chain.submit(signed_tx),"signed staking deployment submitted");
  auto block=chain.mine(first,12);auto receipt=chain.store().receipt(signed_tx.hash());
  check(receipt&&receipt->success,"staking constructor executes in native EVM");
  check(chain.state().get(target).code==unhex(artifact.at("deployedBytecode")),"deployed staking bytecode matches artifact");
  check(chain.state().get(target).balance==minimum&&staking_contract_stake(chain.state(),block.header,target,first)==minimum,"constructor locks first validator deposit");
  deployed=chain.state();
 }
 auto pos_genesis=parse_genesis({{"config",{{"chainId",1337},{"consensus","pos"},{"stakingContract",target}}},{"alloc",deployed.to_json()}});
 Bytes final_root;Json final_validators;Int final_height;
 {Blockchain producer(root/"producer",pos_genesis),peer(root/"peer",pos_genesis);Rpc rpc(producer);
  setenv("NANO_STAKING_TEST_PASSWORD","fixture-only",1);
  Wallet(first_key).save_keyfile(root/"first.json","fixture-only");Wallet(second_key).save_keyfile(root/"second.json","fixture-only");
  rpc.load_wallets(Json{{"wallets",Json::array({Json{{"path",(root/"first.json").string()},{"password_env","NANO_STAKING_TEST_PASSWORD"}},Json{{"path",(root/"second.json").string()},{"password_env","NANO_STAKING_TEST_PASSWORD"}}})}},"127.0.0.1");
  auto request=[&](const std::string& method,Json params=Json::array()){return rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});};
  auto call=[&](const std::string& method,Json params=Json::array()){auto reply=request(method,params);if(reply.contains("error"))throw std::runtime_error(reply.dump());return reply.at("result");};
  auto produce=[&](){auto selected=producer.next_validator();check(selected.has_value(),"staking leaves a selected validator");auto timestamp=Int(producer.tip().header.timestamp+12);auto block=producer.validate(*selected,timestamp);peer.import_block(block,timestamp);check(producer.state().root()==peer.state().root()&&producer.validators()==peer.validators(),"peer replays staking state and validator registry");return block;};
  check(producer.next_validator()==address(first),"contract seeds PoS validator at genesis");
  auto before=producer.state().root();auto hash=call("nno_registerValidator",{second,quantity(minimum)}).get<std::string>();
  check(hash.size()==66&&producer.state().root()==before&&producer.stake(second)==0,"registration queues signed transaction without mutating state");
  auto pending=producer.pending_transaction(unhex(hash));check(pending&&address(pending->sender())==address(second)&&pending->tx.to==target&&pending->tx.value==minimum,"registration signer target and value are correct");
  produce();check(producer.store().receipt(unhex(hash))->success&&producer.stake(second)==minimum&&producer.validators().size()==2,"registration becomes effective only after inclusion");
  check(request("nno_registerValidator",{second,quantity(minimum)}).contains("error"),"duplicate registration rejected by contract simulation");
  check(request("nno_addStake",{second,"0x0"}).contains("error"),"zero deposit rejected");
  hash=call("nno_addStake",{second,quantity(Int(minimum/2))});before=producer.state().root();
  check(producer.stake(second)==minimum,"queued addition has no immediate stake effect");produce();
  check(producer.store().receipt(unhex(hash))->success&&producer.stake(second)==minimum*3/2,"addition increases contract stake");
  check(request("nno_withdrawStake",{second}).contains("error")&&producer.pending_count()==0,"early withdrawal rejected without queuing a transaction");
  check(request("nno_registerValidator",{"0x00000000000000000000000000000000000000ff",quantity(minimum)}).contains("error"),"RPC cannot register an address without its signing key");
  Rpc public_rpc(producer);check(public_rpc.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method","nno_addStake"},{"params",Json::array({second,"0x1"})}}).contains("error"),"public RPC cannot sign with unavailable wallet");
  while(producer.tip().header.number<102)produce();
  auto balance=producer.state().get(target).balance;hash=call("nno_withdrawStake",{second});
  check(producer.state().get(target).balance==balance&&producer.stake(second)==minimum*3/2,"withdrawal remains pending until included");
  produce();check(producer.store().receipt(unhex(hash))->success&&producer.stake(second)==0&&producer.validators().size()==1,"withdrawal removes validator through executed contract");
  check(producer.state().get(target).balance==minimum,"withdrawal releases only sender deposit");
  check(request("nno_withdrawStake",{first}).contains("error"),"last validator cannot halt the chain by withdrawing");
  // A contract validator rejects the refund, then accepts it while trying reentry.
  std::ifstream receiver_file("tests/fixtures/staking-receiver.json");Json receiver_artifact;receiver_file>>receiver_artifact;
  auto receiver=create_address(first,producer.pending_nonce(first));
  auto execute=[&](std::optional<std::string> destination,Bytes data,Int value=0){
   Transaction tx;tx.chain_id=1337;tx.nonce=producer.pending_nonce(first);tx.to=destination;tx.data=std::move(data);tx.value=value;tx.gas=3000000;tx.gas_price=2000000000;
   auto signed_tx=SignedTransaction::sign(tx,first_key);check(producer.submit(signed_tx),"adversarial signed transaction accepted");produce();
   auto receipt=producer.store().receipt(signed_tx.hash());check(receipt.has_value(),"adversarial transaction has receipt");return *receipt;
  };
  auto constructor=unhex(receiver_artifact.at("bytecode"));auto argument=bytes(integer(unhex(target)),32);constructor.insert(constructor.end(),argument.begin(),argument.end());
  check(execute(std::nullopt,constructor).success,"contract validator deployed");
  auto selector=[&](const char* name){return unhex(receiver_artifact.at("methodIdentifiers").at(name));};
  check(execute(receiver,selector("register()"),minimum).success&&producer.stake(receiver)==minimum,"contract validator registered through signed transaction");
  Int unlock_height=producer.tip().header.number+100;
  while(producer.tip().header.number<unlock_height)produce();
  auto locked_balance=producer.state().get(target).balance;
  auto failed=execute(receiver,selector("withdraw()"));
  check(!failed.success&&failed.logs.empty(),"rejected refund fails transaction and discards logs");
  check(producer.stake(receiver)==minimum&&producer.validators().size()==2&&producer.state().get(target).balance==locked_balance,"failed refund rolls back registry and balances");
  check(execute(receiver,selector("acceptRefund()")).success,"recipient enables refund");
  auto withdrawn=execute(receiver,selector("withdraw()"));
  check(withdrawn.success&&producer.stake(receiver)==0&&producer.validators().size()==1,"withdrawal succeeds after rejected refund and blocks reentry");
  check(producer.state().get(receiver).balance>=minimum&&producer.state().get(target).balance==locked_balance-minimum,"reentry does not duplicate refund");
  // Solidity packs registry/rejectRefund/reentryRejected in slot zero.
  check(((producer.state().get_storage(receiver,0)>>168)&255)==1,"recipient actually attempted reentry and observed rejection");
  final_root=producer.state().root();final_validators=producer.validators();final_height=producer.tip().header.number;
 }
 {Blockchain reopened(root/"producer",pos_genesis);check(reopened.state().root()==final_root&&reopened.validators()==final_validators&&reopened.tip().header.number==final_height,"staking state and registry survive RocksDB reopen");}
 std::filesystem::remove_all(root);std::cout<<checks<<" staking checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
