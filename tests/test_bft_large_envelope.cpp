#include "support/rocksdb.hpp"
#include "support/process.hpp"
#include "nano/bft_reactor.hpp"
#include "nano/block_auth.hpp"
#include "nano/block_import.hpp"
#include "nano/rpc.hpp"
#include <rocksdb/db.h>
#include "../network/json_frame.hpp"
using namespace test;
#include "support/bft_assembly_crash.hpp"
int main(int argc,char** argv){try{
 if(argc==4&&std::string(argv[1])=="--assembly-crash-child")return assembly_crash_child(argv[2],std::stoul(argv[3]));
 Temp root;const bool ingress=std::getenv("NANO_TEST_BFT_INGRESS")!=nullptr;const bool assembly_crash=std::getenv("NANO_TEST_BFT_ASSEMBLY_CRASH")!=nullptr;const bool fragmented=assembly_crash||ingress||std::getenv("NANO_TEST_BFT_FRAGMENTS")!=nullptr;const unsigned transactions=fragmented?10:5;
 Json staking;std::ifstream(std::string(NANO_TEST_SOURCE)+"/contracts/NanoStaking.json")>>staking;
 std::vector<Bytes> keys;std::vector<std::string> addresses;Json allocations=Json::object();
 Int minimum=ProofOfStake::minimum_stake;
 for(unsigned i=1;i<=4;++i){keys.push_back(bytes(Int(i),32));addresses.push_back(public_address(public_key(keys.back())));allocations[addresses.back()]={{"balance",quantity(Int(minimum*10))}};}
 auto registry=create_address(addresses[0],0);
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",addresses[0]},{"stakingContract",registry}};
 std::string fork="NanoExperimental";if(const char* configured=std::getenv("NANO_TEST_EVM_FORK")){fork=configured;check(fork=="Paris"||fork=="Shanghai","unsupported network fixture fork");config["evmFork"]=fork;}
 auto seed=Genesis::from_json({{"config",config},{"alloc",allocations}});State registered;
 {Blockchain setup(root/"setup",seed);
  for(unsigned i=0;i<4u;++i){Transaction tx;tx.chain_id=1337;tx.value=minimum;tx.gas=3000000;tx.gas_price=2000000000;
   if(i==0)tx.data=unhex(staking.at("bytecode"));else{tx.to=registry;tx.data=unhex(staking.at("methodIdentifiers").at("registerValidator()"));}
   auto signed_tx=SignedTransaction::sign(tx,keys[i]);check(setup.submit(signed_tx),"seed validator registration submitted");
   auto selected=setup.next_validator();auto index=std::find_if(addresses.begin(),addresses.end(),[&](const auto& candidate){return address(candidate)==address(*selected);})-addresses.begin();check(index<4,"known setup proposer");setup.validate_signed(keys[index],Int((i+1)*12));
   check(setup.store().receipt(signed_tx.hash())->success,"seed validator registered by executed contract");
  }registered=setup.state();
 }

 config["bftVersion"]=1;auto genesis=Genesis::from_json({{"gasLimit",fragmented?"0x2625a00":"0x1c9c380"},{"config",config},{"alloc",registered.to_json()}});
 Blockchain chain(root/"node",genesis);
 auto selected=chain.next_validator();auto proposer=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*selected);})-addresses.begin();check(proposer<4,"known proposer");
 if(std::getenv("NANO_TEST_BFT_PROPOSER_CYCLE")){
  auto cycle_chain=std::make_unique<Blockchain>(root/"cycle-chain",genesis);auto& chain=*cycle_chain;Block original;
  using Clock=ConsensusRound::Clock;auto now=Clock::time_point{};auto reactor=std::make_unique<BftReactor>(chain,root/"cycle-journal",keys[proposer]);auto members=chain.voting_set();
  for(unsigned round=0;round<4;++round){
   reactor->tick(now,12+round);auto before=reactor->snapshot_page();if(round==0)original=Block::from_json(before.at("proposal"));check(before.at("round")==quantity(Int(round)),"reactor starts expected cycle round");
   Json votes=Json::array();for(unsigned i=0;i<4;++i)if(i!=proposer)for(auto step:{VoteStep::prevote,VoteStep::precommit}){FinalityVote vote;vote.height=1;vote.round=round;vote.step=step;vote.validator=addresses[i];vote.validator_set_hash=voting_set_hash(members);sign_vote(vote,keys[i],1337,genesis.block.header.hash());votes.push_back(encode_vote_message(vote,1337,genesis.block.header.hash()));}
   reactor->receive({{"version",1},{"height","0x1"},{"round",quantity(Int(round))},{"proposal",nullptr},{"votes",votes},{"previous",nullptr}},now,12+round);reactor->tick(now,12+round);
   check(reactor->snapshot_page().at("round")==quantity(Int(round+1))&&chain.tip().header.number==0,"signed nil quorum advances without committing");now+=1s;
  }
  check(address(*chain.next_validator(4))==address(addresses[proposer]),"proposer returns after full rotation");
  std::cout<<Json{{"stage","proposer-cycle-return"},{"fork",fork},{"height",1},{"round",4},{"proposer",addresses[proposer]}}.dump()<<std::endl;
  reactor->tick(now,16);auto returned=reactor->snapshot_page();check(returned.at("round")=="0x4"&&!returned.at("proposal").is_null()&&bft_proposal_round(Block::from_json(returned.at("proposal")))==4,"returning proposer provides current-round proposal");
  auto block=Block::from_json(returned.at("proposal"));check(block.header.hash()!=original.header.hash(),"new round has distinct producer proposal domain");
  auto reserve=[&](ChainStore& store,const Block& proposal,std::optional<uint64_t> round){store.reserve_signature(1337,genesis.block.header.hash(),addresses[proposer],1,block_signing_hash(proposal,1337,genesis.block.header.hash()),proposal,round);};
  unsigned refused=0;auto reject=[&](auto action){bool failed=false;try{action();}catch(const std::runtime_error&){failed=true;}check(failed,"conflicting or ambiguous signing reservation refused");++refused;};
  {ChainStore store(root/"slot-tests");reserve(store,block,4);reserve(store,block,4);auto changed=block;changed.header.timestamp+=1;reject([&]{reserve(store,changed,4);});reject([&]{reserve(store,original,std::nullopt);});reject([&]{store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1);});}
  {ChainStore store(root/"slot-tests");check(store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,4)->to_json(true)==block.to_json(true),"round reservation resumes exact bytes after reopen");}
  {ChainStore store(root/"legacy-tests");reserve(store,original,std::nullopt);check(store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,0)->to_json(true)==original.to_json(true),"legacy proposal recovers only in its original round");check(!store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,4),"legacy proposal does not shadow a later round");auto changed=original;changed.header.timestamp+=1;reject([&]{reserve(store,changed,0);});reserve(store,block,4);reject([&]{reserve(store,block,std::nullopt);});}
  {ChainStore store(root/"legacy-tests");check(store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,0)->header.hash()==original.header.hash()&&store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,4)->header.hash()==block.header.hash(),"legacy and round slots survive reopen independently");}
  {ChainStore store(root/"legacy-incomplete");store.reserve_signature(1337,genesis.block.header.hash(),addresses[proposer],1,block_signing_hash(original,1337,genesis.block.header.hash()));reject([&]{reserve(store,block,4);});reject([&]{store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,4);});}
  for(unsigned mode=0;mode<5;++mode){auto path=root/("corrupt-slot-"+std::to_string(mode));{ChainStore store(path);reserve(store,block,4);}
   const auto base="signing/v1/"+hex(bytes(Int(1337),32))+"/"+hex(genesis.block.header.hash())+"/"+address(addresses[proposer])+"/"+hex(bytes(Int(1),8));const auto slot=base+"/round/v1/"+hex(bytes(Int(4),8));
   {std::unique_ptr<rocksdb::DB> db;rocksdb::Options options;check(test::open_rocksdb(options,path.string(),db).ok(),"open corruption fixture");rocksdb::WriteOptions durable;durable.sync=true;rocksdb::Status result;
    if(mode==0)result=db->Delete(durable,base);if(mode==1)result=db->Delete(durable,slot+"/proposal");if(mode==2)result=db->Put(durable,slot,hex(Bytes(32,0)));if(mode==3){auto changed=block;changed.header.timestamp+=1;result=db->Put(durable,slot+"/proposal",changed.to_json(true).dump());}if(mode==4)result=db->Delete(durable,slot);check(result.ok(),"apply corruption fixture");}
   {ChainStore store(path);reject([&]{store.signing_proposal(1337,genesis.block.header.hash(),addresses[proposer],1,4);});reject([&]{reserve(store,block,4);});}
  }
  auto previous_votes=returned.at("votes");reactor.reset();reactor=std::make_unique<BftReactor>(chain,root/"cycle-journal",keys[proposer]);reactor->tick(now,16);auto recovered=reactor->snapshot_page();check(recovered.at("proposal")==returned.at("proposal")&&recovered.at("votes")==previous_votes,"reactor reopens exact proposal and reserved vote");
  Json votes=Json::array();for(unsigned i=0;i<4;++i)if(i!=proposer)for(auto step:{VoteStep::prevote,VoteStep::precommit}){FinalityVote vote;vote.height=1;vote.round=4;vote.step=step;vote.validator=addresses[i];vote.validator_set_hash=voting_set_hash(members);vote.block_hash=block.header.hash();sign_vote(vote,keys[i],1337,genesis.block.header.hash());votes.push_back(encode_vote_message(vote,1337,genesis.block.header.hash()));}
  reactor->receive({{"version",1},{"height","0x1"},{"round","0x4"},{"proposal",nullptr},{"votes",votes},{"previous",nullptr}},now,16);reactor->tick(now,16);check(chain.tip().header.hash()==block.header.hash()&&chain.store().certificate_at(1)->round==4&&chain.audit().at("verified")==true,"returning proposer commits only with signed quorum");
  reactor.reset();cycle_chain.reset();{Blockchain reopened(root/"cycle-chain",genesis);check(reopened.tip().header.hash()==block.header.hash()&&reopened.audit().at("verified")==true,"cycle commit survives full database reopen");}
  std::cout<<Json{{"scenario","bft-proposer-cycle"},{"fork",fork},{"returnedRound",4},{"canonicalHeight","0x1"},{"signatureRefusals",refused},{"reactorReopened",true},{"historyReopened",true},{"legacyMigrationVerified",true},{"corruptedSlotsRejected",5}}.dump()<<std::endl;return 0;
 }
 for(unsigned n=0;n<transactions;++n){Transaction tx;tx.chain_id=1337;tx.nonce=registered.get(addresses[0]).nonce+n;tx.to="0x00000000000000000000000000000000000000b0";if(assembly_crash)tx.value=1;tx.gas=4000000;tx.gas_price=2000000000;tx.data=Bytes(900000,0);check(chain.submit(SignedTransaction::sign(tx,keys[0])),"large transaction admitted");}
 auto block=chain.block_template(*selected,12,0);check(block.transactions.size()==transactions,"all large transactions fit");sign_block(block,keys[proposer],1337,genesis.block.header.hash());
 using Clock=ConsensusRound::Clock;auto start=Clock::time_point{};BftReactor reactor(chain,root/"journal",keys[0]);
 Json envelope={{"version",1},{"height","0x1"},{"round","0x0"},{"proposal",block.to_transport_json()},{"votes",Json::array()},{"previous",nullptr}};
 reactor.receive(envelope,start,12);reactor.tick(start,12);
 auto members=chain.voting_set();auto vote=[&](unsigned index,VoteStep step,std::optional<Bytes> target){FinalityVote v;v.height=1;v.round=0;v.step=step;v.validator=addresses[index];v.validator_set_hash=voting_set_hash(members);v.block_hash=target;sign_vote(v,keys[index],1337,genesis.block.header.hash());return encode_vote_message(v,1337,genesis.block.header.hash());};
 envelope["proposal"]=nullptr;envelope["votes"]=Json::array({vote(1,VoteStep::prevote,block.header.hash()),vote(2,VoteStep::prevote,block.header.hash())});reactor.receive(envelope,start,12);reactor.tick(start,12);
 auto before=reactor.snapshot();bool locked=false;for(const auto& wire:before.at("votes")){auto v=decode_vote_message(wire.get<std::string>(),members,1337,genesis.block.header.hash(),1,0);if(address(v.validator)==address(addresses[0])&&v.step==VoteStep::precommit&&v.block_hash==block.header.hash())locked=true;}check(locked,"local validator locked verified large proposal");
 if(std::getenv("NANO_TEST_BFT_SNAPSHOT_BENCH")){
  auto expected=reactor.snapshot_page(false,true);check(expected.at("votes").size()==4,"snapshot benchmark contains four validated votes");
  reactor.receive(envelope,start,12);check(reactor.snapshot_page(false,true)==expected,"duplicate votes preserve exact snapshot order and bytes");
  auto invalid=envelope;auto malformed=invalid["votes"][0].get<std::string>();malformed.back()='z';invalid["votes"]=Json::array({malformed});bool refused=false;
  try{reactor.receive(invalid,start,12);}catch(const std::exception&){refused=true;}check(refused&&reactor.snapshot_page(false,true)==expected,"invalid vote cannot change cached snapshot");
  auto conflict=envelope;conflict["votes"]=Json::array({vote(1,VoteStep::prevote,std::nullopt)});reactor.receive(conflict,start,12);
  check(reactor.snapshot_page(false,true)==expected,"equivocation retains first validated gossip vote");
  const auto started=Clock::now();for(unsigned i=0;i<1000;++i)check(reactor.snapshot_page(false,true)==expected,"repeated snapshot is byte equivalent");
  std::cout<<Json{{"scenario","bft-snapshot-cache"},{"fork",fork},{"reads",1000},{"votes",4},{"milliseconds",std::chrono::duration<double,std::milli>(Clock::now()-started).count()},{"duplicatePreserved",true},{"invalidRejected",true},{"equivocationPreserved",true}}.dump()<<std::endl;
 }
 // Peers may time out before receiving the prevote quorum; their nil precommits
 // permit a round transition but do not form a conflicting commit certificate.
 envelope["votes"]=Json::array({vote(1,VoteStep::precommit,std::nullopt),vote(2,VoteStep::precommit,std::nullopt)});reactor.receive(envelope,start,12);reactor.tick(start+5s,17);
 auto after=reactor.snapshot();check(after.at("round")=="0x1"&&after.at("previous").at("round")=="0x0","split precommits advance one round");check(chain.tip().header.number==0,"no quorum manufactured");
 check(Block::from_json(after.at("proposal")).header.hash()==block.header.hash()&&Block::from_json(after.at("previous").at("proposal")).header.hash()==block.header.hash(),"locked block retained across round");
 const auto before_bytes=before.dump().size();const auto wire=after.dump();bool initial_fits=false;try{JsonFrame initial;initial_fits=initial.feed(before.dump());}catch(const std::exception&){}check(initial_fits!=fragmented,"initial proposal frame boundary matches fixture");
 bool fits=true;std::string error;try{JsonFrame frame;fits=frame.feed(wire);}catch(const std::exception& e){fits=false;error=e.what();}
 std::cout<<Json{{"scenario","large-bft-round-envelope"},{"fork",fork},{"beforeBytes",before_bytes},{"afterBytes",wire.size()},{"maximumBytes",JsonFrame::maximum_size},{"proposalHash",hex(block.header.hash())},{"roundAdvanced",true},{"lockPreserved",true},{"frameAccepted",fits},{"error",error}}.dump()<<std::endl;
 check(!fits,"fixture still demonstrates oversized aggregate snapshot");
 auto current=reactor.snapshot_page();auto previous=reactor.snapshot_page(true);
 check(current.at("round")=="0x1"&&previous.at("round")=="0x0","both rounds remain available");
 for(const auto& page:{current,previous}){check(page.at("previous").is_null(),"page cannot nest another proposal");bool page_fits=false;try{JsonFrame frame;page_fits=frame.feed(page.dump());}catch(const std::exception&){}check(page_fits!=fragmented,"single-round frame boundary matches fixture");check(Block::from_json(page.at("proposal")).header.hash()==block.header.hash(),"page preserves exact locked proposal");}
 auto historical=after.at("previous");historical["previous"]=nullptr;check(previous==historical,"previous page preserves every retained vote");auto active=after;active["previous"]=nullptr;check(current==active,"current page preserves every active vote");
 size_t fragments=0,maximum_fragment=0,manifest_bytes=0;unsigned rejected=0;
 if(fragmented){
  Blockchain api_chain(root/"rpc",genesis);Rpc api(api_chain);
  auto invoke=[&](const char* method,Json params=Json::array()){return api.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});};
  check(invoke("nano_bftManifest").at("result").is_null(),"disabled reactor has no manifest");
  setenv("NANO_BFT_FRAGMENT_FIXTURE_KEY",hex(keys[3]).c_str(),1);api.load_wallets({{"bft",{{"key_env","NANO_BFT_FRAGMENT_FIXTURE_KEY"},{"journal",(root/"rpc-journal").string()}}}},"127.0.0.1");
  Json incoming={{"version",1},{"height","0x1"},{"round","0x0"},{"proposal",block.to_transport_json()},{"votes",Json::array()},{"previous",nullptr}};
  check(!invoke("nano_bftExchangePage",Json::array({incoming})).contains("error"),"RPC accepts ordinary authenticated proposal");
  auto rpc_before=invoke("nano_bftExchangePage");auto api_manifest=invoke("nano_bftManifest");
  check(api_manifest.at("result").at("proposal")==reactor.snapshot_page(false,true).at("proposal"),"RPC manifest preserves proposal identity");
  check(invoke("nano_bftTransactionFragment",Json::array({hex(block.header.hash()),hex(block.transactions[0].hash()),"0x0"})).at("result")==reactor.proposal_fragment(block.header.hash(),block.transactions[0].hash(),0),"RPC returns exact signed fragment");
  check(invoke("nano_bftManifest",Json::array({"previous"})).contains("error")&&invoke("nano_bftTransactionFragment",Json::array({hex(block.header.hash()),hex(block.transactions[0].hash()),"0x1"})).contains("error"),"RPC refuses bad selector and offset");
  check(invoke("nano_bftExchangePage")==rpc_before&&api_chain.tip().header.number==0,"read-only proposal RPC changes neither vote nor canonical state");
  if(assembly_crash){
   auto manifest=reactor.snapshot_page(false,true);manifest["round"]="0x0";manifest["votes"]=Json::array();Json parts=Json::array();
   for(size_t index=0;index<block.transactions.size();++index){uint64_t offset=0;for(;;){auto part=reactor.proposal_fragment(block.header.hash(),block.transactions[index].hash(),offset);parts.push_back({{"index",index},{"fragment",part}});if(part.at("next").is_null())break;offset=u64(quantity(part.at("next")));}}
   verify_assembly_crashes(argv[0],root.path,{{"gasLimit","0x2625a00"},{"config",config},{"alloc",registered.to_json()}},manifest,parts,block,Json::array({vote(0,VoteStep::prevote,block.header.hash()),vote(1,VoteStep::prevote,block.header.hash())}),Json::array({vote(0,VoteStep::precommit,block.header.hash()),vote(1,VoteStep::precommit,block.header.hash())}),fork);
  }
  if(ingress){
   unsigned ingress_rejected=0,forwarded=0;
   {
    Blockchain destination(root/"ingress",genesis);Rpc receiver(destination,"proposal-test-token");
    receiver.load_wallets({{"bft",{{"key_env","NANO_BFT_FRAGMENT_FIXTURE_KEY"},{"journal",(root/"ingress-journal").string()}}}},"127.0.0.1");
    auto request=[&](const char* method,Json params=Json::array()){return receiver.handle(Json{{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",std::move(params)}});};
    auto good=reactor.snapshot_page(false,true);good["round"]="0x0";good["votes"]=Json::array();
    auto reject=[&](const char* method,Json params){auto result=request(method,std::move(params));check(result.contains("error"),"invalid BFT ingress accepted");++ingress_rejected;return result;};
    reject("nano_beginBftImport",Json::array({"wrong",good}));reject("nano_finishBftImport",Json::array({"wrong","0x"}));
    auto invalid=good;invalid["round"]="0x1";reject("nano_beginBftImport",Json::array({"proposal-test-token",invalid}));invalid=good;invalid["proposal"]["producerProof"]["signature"]="0x";reject("nano_beginBftImport",Json::array({"proposal-test-token",invalid}));
    auto begin=[&](const Json& value){auto result=request("nano_beginBftImport",Json::array({"proposal-test-token",value}));if(result.contains("error"))throw std::runtime_error("BFT begin failed: "+result.dump());return result.at("result").at("id").get<std::string>();};
    auto valid_id=begin(good);
    auto bad_block=block;bad_block.header.state_root[0]^=1;sign_block(bad_block,keys[proposer],1337,genesis.block.header.hash());auto bad_manifest=good;bad_manifest["proposal"]=bad_block.to_json(false);bad_manifest["proposal"]["transactionEncoding"]="signed-rlp-hashes-v1";auto bad_id=begin(bad_manifest);
    auto plain=Json{{"version",1},{"block",good.at("proposal")}};auto block_id=request("nano_beginBlockImport",Json::array({"proposal-test-token",plain})).at("result").at("id").get<std::string>();auto spare=begin(good);
    check(reject("nano_beginBftImport",Json::array({"proposal-test-token",good})).at("error").at("code")==-32005,"blocks and proposals share four session slots");
    reject("nano_finishBlockImport",Json::array({"proposal-test-token",valid_id}));reject("nano_finishBftImport",Json::array({"proposal-test-token",block_id}));reject("nano_finishBftImport",Json::array({"proposal-test-token",valid_id}));
    for(const auto& id:{block_id})check(request("nano_abortBlockImport",Json::array({"proposal-test-token",id})).at("result")==true,"shared abort releases session");
    auto before=destination.state().root();
    for(size_t index=0;index<block.transactions.size();++index){uint64_t offset=0;for(;;){
     auto part=reactor.proposal_fragment(block.header.hash(),block.transactions[index].hash(),offset);
     check(!request("nano_appendBlockImport",Json::array({"proposal-test-token",valid_id,index,part})).contains("error"),"valid proposal fragment accepted");auto bad_part=part;bad_part["blockHash"]=hex(bad_block.header.hash());check(!request("nano_appendBlockImport",Json::array({"proposal-test-token",bad_id,index,bad_part})).contains("error"),"invalid-state proposal stages only bytes");check(!request("nano_appendBlockImport",Json::array({"proposal-test-token",spare,index,part})).contains("error"),"late-finishing proposal stages exact bytes");++forwarded;
     auto waiting=request("nano_bftManifest").at("result");check(waiting.at("proposal").is_null()&&waiting.at("votes").empty()&&destination.state().root()==before&&destination.tip().header.number==0,"partial or complete staging cannot publish proposal or vote");
     if(part.at("next").is_null())break;offset=u64(quantity(part.at("next")));
    }}
    reject("nano_finishBftImport",Json::array({"proposal-test-token",bad_id}));auto untouched=request("nano_bftManifest").at("result");check(untouched.at("proposal").is_null()&&untouched.at("votes").empty()&&destination.state().root()==before,"invalid executed state cannot cause a vote");
    auto accepted=request("nano_finishBftImport",Json::array({"proposal-test-token",valid_id}));check(accepted.at("result").at("received")==true&&accepted.at("result").at("blockHash")==hex(block.header.hash()),"complete proposal validated through ordinary BFT receiver");reject("nano_finishBftImport",Json::array({"proposal-test-token",valid_id}));
    auto voted=request("nano_bftManifest").at("result");bool positive=false;for(const auto& wire:voted.at("votes")){auto v=decode_vote_message(wire.get<std::string>(),members,1337,genesis.block.header.hash(),1,0);if(address(v.validator)==address(addresses[3])&&v.step==VoteStep::prevote&&v.block_hash==block.header.hash())positive=true;}check(positive&&destination.tip().header.number==0&&destination.state().root()==before,"validated proposal permits prevote but no premature commit");
    Json quorum={{"version",1},{"height","0x1"},{"round","0x0"},{"proposal",nullptr},{"votes",Json::array({vote(0,VoteStep::prevote,block.header.hash()),vote(1,VoteStep::prevote,block.header.hash())})},{"previous",nullptr}};
    check(!request("nano_bftExchangePage",Json::array({quorum})).contains("error"),"ordinary prevote quorum accepted");quorum["votes"]=Json::array({vote(0,VoteStep::precommit,block.header.hash()),vote(1,VoteStep::precommit,block.header.hash())});check(!request("nano_bftExchangePage",Json::array({quorum})).contains("error"),"ordinary precommit quorum accepted");
    check(destination.tip().header.hash()==block.header.hash()&&destination.audit().at("verified")==true,"fragmented proposal reaches quorum-certified audited state");reject("nano_beginBftImport",Json::array({"proposal-test-token",good}));reject("nano_finishBftImport",Json::array({"proposal-test-token",spare}));check(destination.tip().header.hash()==block.header.hash(),"late finish cannot apply committed proposal again");
   }
   {Blockchain reopened(root/"ingress",genesis);check(reopened.tip().header.hash()==block.header.hash()&&reopened.audit().at("verified")==true,"fragmented BFT commit survives reopening");}
   std::cout<<Json{{"scenario","bft-proposal-ingress"},{"fork",fork},{"fragmentsPerAssembly",forwarded},{"assemblies",3},{"staleFinishRejected",true},{"invalidRequestsRejected",ingress_rejected},{"sharedSessionBudget",true},{"invalidStateNoVote",true},{"quorumCommitAudited",true},{"reopenedAudit",true},{"libp2pIntegrated",false}}.dump()<<std::endl;
  }
  auto unchanged=reactor.snapshot();BlockImports assembler;
  for(bool old:{false,true}){
   auto manifest=reactor.snapshot_page(old,true);auto expected=old?previous:current;
   manifest_bytes=std::max(manifest_bytes,manifest.dump().size());JsonFrame manifest_frame;check(manifest_frame.feed(manifest.dump()),"manifest fits a real frame");
   Blockchain incomplete(root/(old?"incomplete-previous":"incomplete-current"),genesis);BftReactor guard(incomplete,root/(old?"guard-previous":"guard-current"),keys[3]);
   auto attempt=manifest;attempt["round"]="0x0";attempt["votes"]=Json::array();
   bool refused=false;try{guard.receive(attempt,start,17);}catch(const std::exception&){refused=true;}
   check(refused&&guard.snapshot().at("proposal").is_null()&&guard.snapshot().at("votes").empty()&&incomplete.tip().header.number==0,"manifest alone cannot produce a proposal, vote or commit");++rejected;
   auto session=assembler.begin(Json{{"version",1},{"block",manifest.at("proposal")}},start);auto id=session.at("id").get<std::string>();
   for(size_t index=0;index<block.transactions.size();++index){uint64_t offset=0;for(;;){
    auto part=reactor.proposal_fragment(block.header.hash(),block.transactions[index].hash(),offset);maximum_fragment=std::max(maximum_fragment,part.dump().size());JsonFrame frame;check(frame.feed(part.dump()),"proposal fragment fits frame");assembler.append(id,index,part,start);++fragments;
    if(part.at("next").is_null())break;offset=u64(quantity(part.at("next")));
   }}
   auto reconstructed=assembler.finish(id,start);check(reconstructed==expected.at("proposal"),"every reconstructed proposal field matches original");manifest["proposal"]=reconstructed;check(manifest==expected,"votes and round survive fragmented reconstruction");
  }
  for(unsigned invalid=0;invalid<4;++invalid){bool refused=false;try{reactor.proposal_fragment(invalid==0?Bytes(32,0):block.header.hash(),invalid==1?Bytes(32,0):block.transactions[0].hash(),invalid==2?1:invalid==3?1024*1024:0);}catch(const std::exception&){refused=true;}check(refused,"bad proposal fragment request refused");++rejected;}
  check(reactor.snapshot()==unchanged&&chain.tip().header.number==0,"fragment serving changes neither votes nor canonical state");
 }
 Blockchain lagging(root/"lagging",genesis);BftReactor receiver(lagging,root/"lagging-journal",keys[3]);receiver.receive(previous,start,17);receiver.tick(start,17);check(lagging.tip().header.number==0,"previous page cannot fabricate finality");check(receiver.snapshot().at("proposal").at("hash")==current.at("proposal").at("hash"),"lagging reactor receives exact proposal from previous page");
 reactor.tick(start+5s,17);
 auto next_vote=[&](unsigned index,VoteStep step){FinalityVote v;v.height=1;v.round=1;v.step=step;v.validator=addresses[index];v.validator_set_hash=voting_set_hash(members);v.block_hash=block.header.hash();sign_vote(v,keys[index],1337,genesis.block.header.hash());return encode_vote_message(v,1337,genesis.block.header.hash());};
 auto next=current;next["proposal"]=nullptr;next["votes"]=Json::array({next_vote(1,VoteStep::prevote),next_vote(2,VoteStep::prevote)});reactor.receive(next,start+5s,17);reactor.tick(start+5s,17);
 next["votes"]=Json::array({next_vote(1,VoteStep::precommit),next_vote(2,VoteStep::precommit)});reactor.receive(next,start+5s,17);reactor.tick(start+5s,17);
 check(chain.tip().header.hash()==block.header.hash()&&chain.store().certificate_at(1)->round==1&&chain.audit().at("verified")==true,"large retained proposal finalizes and audits in next round");
 check(reactor.snapshot_page().is_null()&&reactor.snapshot_page(true).is_null(),"old-height pages disappear after commit");
 if(fragmented){bool refused=false;try{reactor.proposal_fragment(block.header.hash(),block.transactions[0].hash(),0);}catch(const std::exception&){refused=true;}check(refused&&reactor.snapshot_page(false,true).is_null()&&reactor.snapshot_page(true,true).is_null(),"committed height cannot serve stale proposal fragments");++rejected;
 std::cout<<Json{{"scenario","bft-proposal-fragments"},{"fork",fork},{"fragments",fragments},{"maximumFragmentBytes",maximum_fragment},{"maximumManifestBytes",manifest_bytes},{"rejections",rejected},{"compactProposalBytes",block.to_transport_json().dump().size()},{"nextRoundAudited",true},{"networkQualified",false}}.dump()<<std::endl;}
 std::cout<<Json{{"nextRoundAudited",true},{"scenario","large-bft-round-pages"},{"fork",fork},{"currentBytes",current.dump().size()},{"previousBytes",previous.dump().size()},{"votesPreserved",true},{"laggingAccepted",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
