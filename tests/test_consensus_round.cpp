#include "nano/consensus_round.hpp"
#include "nano/block_auth.hpp"
#include "nano/rpc.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
using namespace nano;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 auto key=bytes(Int(1),32);auto signer=public_address(public_key(key));
 auto genesis=Genesis::from_json({{"config",{{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",signer}}}});
 auto root=std::filesystem::temp_directory_path()/("nano-round-"+std::to_string(getpid()));
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 using Clock=ConsensusRound::Clock;auto start=Clock::time_point{};auto second=std::chrono::seconds(1);
 {Blockchain chain(root/"chain",genesis);ChainStore journal(root/"journal");ConsensusRound round(chain,journal,key,0,start,second,second,second);
  bool early=false;try{round.begin_next_round(start);}catch(const std::runtime_error&){early=true;}check(early&&round.round()==0,"no round change without participation");
  auto idle=round.advance(start,12);check(idle.broadcast.empty()&&!idle.committed&&!idle.next_round,"no vote before proposal or timeout");
  auto block=chain.block_template(signer,12);sign_block(block,key,1337,genesis.block.header.hash());
  auto bad=block;bad.header.state_root[0]^=1;sign_block(bad,key,1337,genesis.block.header.hash());bool refused=false;
  try{round.propose(bad,12);}catch(const std::exception&){refused=true;}check(refused&&!journal.latest_consensus_vote(signer,1337,genesis.block.header.hash()),"bad execution never reaches signing journal");
  round.propose(block,12);auto progress=round.advance(start,12);
  check(progress.committed&&progress.broadcast.size()==2&&progress.broadcast[0].step==VoteStep::prevote&&progress.broadcast[1].step==VoteStep::precommit,"round coordinates proposal prevote precommit and durable commit");
  check(chain.tip().header.hash()==block.header.hash()&&chain.store().certificate_at(1).has_value()&&chain.audit().at("verified")==true,"coordinator publishes audited certified block");
  auto repeat=round.advance(start+second,13);check(repeat.committed&&repeat.broadcast.empty(),"completed round does not sign again");
  ConsensusRound next(chain,journal,key,0,start+second,second,second,second);auto next_block=chain.block_template(signer,24);sign_block(next_block,key,1337,genesis.block.header.hash());next.propose(next_block,24);check(next.advance(start+second,24).committed,"certified next height starts from advanced journal");
 }
 {Blockchain reopened(root/"chain",genesis);check(reopened.tip().header.number==2&&reopened.audit().at("verified")==true,"coordinated commits survive reopen");}
 {Blockchain chain(root/"nil",genesis);ChainStore journal(root/"nil-journal");ConsensusRound round(chain,journal,key,0,start,second,second,second);
  auto timeout=round.advance(start+second,12);check(timeout.broadcast.size()==2&&!timeout.committed&&timeout.next_round&&chain.tip().header.number==0,"nil quorum requests new round without committing");
  check(!timeout.broadcast[0].block_hash&&!timeout.broadcast[1].block_hash,"timeout emits nil votes");
  bool refused=false;try{ConsensusRound restarted(chain,journal,key,1,start+second,second,second,second);}catch(const std::runtime_error&){refused=true;}check(refused,"cross-round restart still refuses guessed progression");
  ConsensusRound resumed(chain,journal,key,0,start+second,second,second,second);auto recovered=resumed.advance(start+second,12);check(recovered.broadcast.size()==1&&recovered.broadcast[0].signature==timeout.broadcast[1].signature&&recovered.next_round&&!recovered.committed,"same-round nil precommit resumes exact signature");
  resumed.begin_next_round(start+second);check(resumed.round()==1,"nil quorum advances round explicitly");auto proposal=chain.block_template(signer,12);sign_block(proposal,key,1337,genesis.block.header.hash());resumed.propose(proposal,12);auto committed=resumed.advance(start+second,12);check(committed.committed&&committed.broadcast.size()==2&&committed.broadcast[0].round==1&&chain.store().certificate_at(1)->round==1,"new round commits with new vote domain");
 }
 {Blockchain chain(root/"backoff",genesis);ChainStore journal(root/"backoff-journal");ConsensusRound round(chain,journal,key,0,start,second,second,second);
  check(round.advance(start+second,12).next_round,"initial timeout permits retry");
  bool rejected=false;try{round.begin_next_round(start+second,Clock::duration::zero());}catch(const std::invalid_argument&){rejected=true;}check(rejected&&round.round()==0,"invalid retry duration leaves round unchanged");
  round.begin_next_round(start+second,10*second);check(round.advance(start+10*second,12).broadcast.empty(),"retry does not vote nil at old shorter timeout");auto retry=round.advance(start+11*second,12);check(retry.next_round&&retry.broadcast.size()==2&&!retry.committed,"retry timeout emits nil votes without a block commit");
 }
 // Derive four real equal-weight validators from executed staking transactions.
 Json staking;std::ifstream("contracts/NanoStaking.json")>>staking;
 std::vector<Bytes> keys;std::vector<std::string> addresses;Json allocations=Json::object();
 Int minimum=ProofOfStake::minimum_stake;
 for(unsigned i=1;i<=4;++i){keys.push_back(bytes(Int(i),32));addresses.push_back(public_address(public_key(keys.back())));allocations[addresses.back()]={{"balance",quantity(Int(minimum*10))}};}
 auto registry=create_address(addresses[0],0);
 Json config={{"chainId",1337},{"consensus","pos"},{"posSignatureVersion",1},{"bootstrapValidator",addresses[0]},{"stakingContract",registry}};
 auto seed=Genesis::from_json({{"config",config},{"alloc",allocations}});State registered;
 {Blockchain setup(root/"setup",seed);
  for(unsigned i=0;i<4;++i){Transaction tx;tx.chain_id=1337;tx.value=minimum;tx.gas=3000000;tx.gas_price=2000000000;
   if(i==0)tx.data=unhex(staking.at("bytecode"));else{tx.to=registry;tx.data=unhex(staking.at("methodIdentifiers").at("registerValidator()"));}
   auto signed_tx=SignedTransaction::sign(tx,keys[i]);check(setup.submit(signed_tx),"seed validator registration submitted");
   auto selected=setup.next_validator();auto index=std::find_if(addresses.begin(),addresses.end(),[&](const auto& candidate){return address(candidate)==address(*selected);})-addresses.begin();check(index<4,"known setup proposer");setup.validate_signed(keys[index],Int((i+1)*12));
   check(setup.store().receipt(signed_tx.hash())->success,"seed validator registered by executed contract");
  }registered=setup.state();
 }
 auto old_mode=Genesis::from_json({{"config",config},{"alloc",registered.to_json()}});
 config["bftVersion"]=1;
 auto multi=Genesis::from_json({{"config",config},{"alloc",registered.to_json()}});Bytes committed_hash;
 check(multi.bft_version==1&&multi.block.header.hash()!=old_mode.block.header.hash(),"BFT requirement changes genesis identity");
 for(const auto& invalid:std::vector<Json>{0,2,1.0,"1",true}){auto bad=config;bad["bftVersion"]=invalid;bool refused=false;try{Genesis::from_json({{"config",bad}});}catch(const std::invalid_argument&){refused=true;}check(refused,"unsupported BFT version refused");}

 {
  std::vector<std::unique_ptr<Blockchain>> chains;std::vector<std::unique_ptr<ChainStore>> journals;std::vector<std::unique_ptr<ConsensusRound>> rounds;
  for(unsigned i=0;i<4;++i){chains.push_back(std::make_unique<Blockchain>(root/("multi-"+std::to_string(i)),multi));journals.push_back(std::make_unique<ChainStore>(root/("votes-"+std::to_string(i))));rounds.push_back(std::make_unique<ConsensusRound>(*chains.back(),*journals.back(),keys[i],0,start,second,second,second));}
  auto members=chains[0]->voting_set();check(members.size()==4,"four contract-derived validators");for(const auto& m:members)check(m.second==minimum,"equal voting weight from deposited stake");
  auto selected=chains[0]->next_validator();auto proposer=std::find_if(addresses.begin(),addresses.end(),[&](const auto& candidate){return address(candidate)==address(*selected);})-addresses.begin();check(proposer<4,"known multi-node proposer");
  auto block=chains[0]->block_template(*selected,12);sign_block(block,keys[proposer],1337,multi.block.header.hash());committed_hash=block.header.hash();
  bool uncertified=false;try{chains[0]->import_block(block,12);}catch(const std::invalid_argument&){uncertified=true;}check(uncertified&&chains[0]->tip().header.number==0,"BFT rejects first block without certificate");
  uncertified=false;try{chains[0]->validate_signed(keys[proposer],12);}catch(const std::invalid_argument&){uncertified=true;}check(uncertified&&chains[0]->tip().header.number==0,"direct signed production cannot bypass BFT");
  std::vector<FinalityVote> prevotes,precommits;
  for(unsigned i=0;i<3;++i){rounds[i]->propose(block,12);auto progress=rounds[i]->advance(start,12);check(!progress.committed&&progress.broadcast.size()==1,"one quarter cannot precommit");prevotes.push_back(progress.broadcast[0]);}
  auto restart=[&](unsigned i){rounds[i].reset();journals[i].reset();chains[i].reset();chains[i]=std::make_unique<Blockchain>(root/("multi-"+std::to_string(i)),multi);journals[i]=std::make_unique<ChainStore>(root/("votes-"+std::to_string(i)));rounds[i]=std::make_unique<ConsensusRound>(*chains[i],*journals[i],keys[i],0,start,second,second,second);};
  restart(0);auto resumed_prevote=rounds[0]->advance(start,12);check(resumed_prevote.broadcast.size()==1&&resumed_prevote.broadcast[0].signature==prevotes[0].signature&&!resumed_prevote.committed,"prevote resumes after chain and journal reopen");
  auto wire=[&](const FinalityVote& vote){return encode_vote_message(vote,1337,multi.block.header.hash());};
  rounds[0]->receive(wire(prevotes[1]),start);
  check(rounds[0]->receive(wire(prevotes[1]),start)==VoteReception::duplicate,"duplicate vote not counted twice");
  auto conflicting=prevotes[1];conflicting.block_hash=keccak(Bytes{99});sign_vote(conflicting,keys[1],1337,multi.block.header.hash());
  check(rounds[0]->receive(wire(conflicting),start)==VoteReception::equivocation,"conflicting signed prevote retained as evidence");
  auto insufficient=rounds[0]->advance(start,12);check(insufficient.broadcast.empty()&&!insufficient.committed&&chains[0]->tip().header.number==0,"half weight plus duplicates and equivocation cannot commit");
  auto forged=prevotes[2];forged.signature[0]^=1;bool rejected=false;try{rounds[0]->receive(wire(forged),start);}catch(const std::exception&){rejected=true;}check(rejected,"forged signature refused by coordinator");
  for(unsigned i=0;i<3;++i){for(const auto& vote:prevotes)rounds[i]->receive(wire(vote),start);auto progress=rounds[i]->advance(start,12);check(progress.broadcast.size()==1&&progress.broadcast[0].step==VoteStep::precommit&&!progress.committed,"three quarters prevote permits precommit only");precommits.push_back(progress.broadcast[0]);}
  restart(1);auto resumed_precommit=rounds[1]->advance(start,12);check(resumed_precommit.broadcast.size()==1&&resumed_precommit.broadcast[0].signature==precommits[1].signature&&!resumed_precommit.committed,"precommit and locked proposal resume after reopen without quorum fabrication");
  for(unsigned i=0;i<3;++i){rounds[i]->receive(wire(precommits[(i+1)%3]),start);auto progress=rounds[i]->advance(start,12);check(!progress.committed&&chains[i]->tip().header.number==0,"two precommits cannot publish block");}
  for(unsigned i=0;i<3;++i){rounds[i]->receive(wire(precommits[(i+2)%3]),start);check(rounds[i]->advance(start,12).committed,"three quarters commits despite silent fourth validator");}
  // Lagging node receives certificate before block: it must wait for verified data.
  for(const auto& vote:precommits)rounds[3]->receive(wire(vote),start);
  check(!rounds[3]->advance(start,12).committed&&chains[3]->tip().header.number==0,"quorum without proposal data cannot commit");
  rounds[3]->propose(block,12);check(rounds[3]->advance(start,12).committed,"lagging node commits once verified proposal arrives");
  for(const auto& chain:chains)check(chain->tip().header.hash()==committed_hash&&chain->state().root()==chains[0]->state().root()&&chain->audit().at("verified")==true,"all four nodes converge to audited certified state");
  // The actual RPC payload forwarded by both libp2p sync and announcements.
  Rpc source(*chains[0]);Blockchain follower(root/"rpc-follower",multi);Rpc receiver(follower);
  auto call=[](Rpc& rpc,const std::string& method,Json params){return rpc.handle({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}}).at("result");};
  auto batch=call(source,"nano_networkBlocks",Json::array({0}));auto envelope=batch.at("blocks").at(0);
  check(envelope.at("bftCertificate").at("votes").size()==3,"sync payload carries quorum certificate");
  auto events=call(source,"nano_networkEvents",Json::array({0}));bool announced=false;
  for(const auto& event:events.at("events"))if(event.at("type")=="block"){check(event.at("value")==envelope,"announce and sync use identical certified block encoding");announced=true;}
  check(announced,"certified commit creates network announcement");
  auto reject=[&](Json bad){auto result=call(receiver,"validator_submitBlock",Json::array({bad}));check(result.at("success")==false&&follower.tip().header.number==0,"invalid remote certificate cannot advance follower");};
  auto bad=envelope;bad.erase("bftCertificate");reject(bad);
  bad=envelope;bad["bftCertificate"]["votes"].erase(2);reject(bad);
  bad=envelope;bad["bftCertificate"]["votes"][2]=bad["bftCertificate"]["votes"][1];reject(bad);
  bad=envelope;bad["bftCertificate"]["votes"][0]=std::string(2049,'x');reject(bad);
  bad=envelope;bad["bftCertificate"]["validators"]=Json::array();reject(bad);
  bad=envelope;bad["bftCertificate"]["round"]="0x00";reject(bad);
  auto imported=call(receiver,"validator_submitBlock",Json::array({envelope}));
  check(imported.at("success")==true&&follower.tip().header.hash()==committed_hash&&follower.state().root()==chains[0]->state().root()&&follower.store().certificate_at(1)->votes.size()==3&&follower.audit().at("verified")==true,"network payload imports certified state and durable proof");
  check(call(receiver,"validator_submitBlock",Json::array({envelope})).at("success")==true,"repeated certified announcement is idempotent");
  {Blockchain initial(root/"uncertified-disk",multi);}
  {ChainStore raw(root/"uncertified-disk");raw.commit(block,chains[0]->state(),{},chains[0]->store().consensus_at(1));}
  bool missing_certificate=false;try{Blockchain corrupted(root/"uncertified-disk",multi);}catch(const std::runtime_error&){missing_certificate=true;}check(missing_certificate,"BFT reopen refuses valid block persisted without its certificate");

 }
 {
  std::vector<std::unique_ptr<Blockchain>> nodes;std::vector<std::unique_ptr<BftReactor>> reactors;
  for(unsigned i=0;i<4;++i){nodes.push_back(std::make_unique<Blockchain>(root/("reactor-"+std::to_string(i)),multi));reactors.push_back(std::make_unique<BftReactor>(*nodes.back(),root/("reactor-journal-"+std::to_string(i)),keys[i]));}
  for(unsigned step=0;step<10;++step){
   for(unsigned i=0;i<4;++i)if(nodes[i]->tip().header.number==0)reactors[i]->tick(start+std::chrono::milliseconds(step*100),12);
   if(step==1){
    auto message=reactors[0]->snapshot();auto target=reactors[1]->snapshot();
    check(!message.is_null()&&!target.is_null(),"reactors expose current vote round");
    for(unsigned repeat=0;repeat<25;++repeat)reactors[1]->receive(message,start+std::chrono::milliseconds(step*100),12);
    auto stable=reactors[1]->snapshot();check(stable.at("votes").size()<=8,"repeated relay traffic is bounded by member and step");
    auto oversized=message;oversized["votes"]=Json::array();for(unsigned n=0;n<9;++n)oversized["votes"].push_back("x");
    bool refused=false;try{reactors[1]->receive(oversized,start+std::chrono::milliseconds(step*100),12);}catch(const std::invalid_argument&){refused=true;}
    check(refused&&reactors[1]->snapshot()==stable,"oversized relay vote batch rejected without gossip growth");
    if(!message.at("votes").empty()){
     auto malformed=message;auto vote=Json::parse(malformed["votes"][0].get<std::string>());vote["signature"]="0x"+std::string(130,'0');malformed["votes"][0]=vote.dump();
     refused=false;try{reactors[1]->receive(malformed,start+std::chrono::milliseconds(step*100),12);}catch(const std::invalid_argument&){refused=true;}
     check(refused&&reactors[1]->snapshot()==stable,"forged relayed signature never enters outbound gossip");
    }
   }
   for(unsigned i=0;i<4;++i){auto message=reactors[i]->snapshot();if(message.is_null())continue;for(unsigned j=0;j<4;++j)if(j!=i&&nodes[j]->tip().header.number==0)reactors[j]->receive(message,start+std::chrono::milliseconds(step*100),12);}
  }
  for(unsigned i=0;i<4;++i)check(nodes[i]->tip().header.number==1&&nodes[i]->state().root()==nodes[0]->state().root()&&nodes[i]->store().certificate_at(1).has_value(),"reactors exchange proposals and votes to certify matching state");
 }
 // A configured signer can follow certified history before registration, then
 // join the next-height quorum without restarting its reactor.
 {
  auto newcomer_key=bytes(Int(5),32);auto newcomer=public_address(public_key(newcomer_key));
  auto alloc=registered.to_json();alloc[newcomer]={{"balance",quantity(Int(minimum*10))}};
  auto joining_genesis=Genesis::from_json({{"config",config},{"alloc",alloc}});
  std::vector<std::unique_ptr<Blockchain>> nodes;std::vector<std::unique_ptr<BftReactor>> reactors;
  auto all_keys=keys;all_keys.push_back(newcomer_key);
  for(unsigned i=0;i<5;++i){nodes.push_back(std::make_unique<Blockchain>(root/("joining-node-"+std::to_string(i)),joining_genesis));reactors.push_back(std::make_unique<BftReactor>(*nodes.back(),root/("joining-journal-"+std::to_string(i)),all_keys[i]));}
  reactors[4]->tick(start,12);check(reactors[4]->snapshot().is_null(),"unregistered signer remains a passive observer");
  Transaction tx;tx.chain_id=1337;tx.to=registry;tx.value=minimum;tx.gas=500000;tx.gas_price=2000000000;tx.data=unhex(staking.at("methodIdentifiers").at("registerValidator()"));
  auto signed_tx=SignedTransaction::sign(tx,newcomer_key);check(nodes[0]->submit(signed_tx),"newcomer registration submitted");
  auto chosen=nodes[0]->next_validator();auto proposer=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*chosen);})-addresses.begin();check(proposer<4,"existing member proposes registration");
  auto block=nodes[0]->block_template(*chosen,12,0);sign_block(block,keys[proposer],1337,joining_genesis.block.header.hash());
  auto original=nodes[0]->voting_set();std::vector<FinalityVote> proof;
  for(unsigned i=0;i<3;++i){FinalityVote v;v.height=1;v.step=VoteStep::precommit;v.block_hash=block.header.hash();v.validator_set_hash=voting_set_hash(original);v.validator=addresses[i];sign_vote(v,keys[i],1337,joining_genesis.block.header.hash());proof.push_back(v);}
  Json envelope={{"version",1},{"height","0x1"},{"round","0x0"},{"previous",nullptr},{"proposal",block.to_json(true)},{"votes",Json::array()}};
  for(const auto& v:proof)envelope["votes"].push_back(encode_vote_message(v,1337,joining_genesis.block.header.hash()));
  reactors[4]->receive(envelope,start,12);check(reactors[4]->snapshot().is_null(),"incoming registration proposal cannot activate newcomer before certification");
  for(auto& node:nodes){node->import_certified_block(block,0,proof,12);check(node->voting_set().size()==5&&node->store().receipt(signed_tx.hash())->success,"certified registration activates fifth validator");}
  auto next=nodes[0]->next_validator();unsigned absent=address(*next)==address(addresses[0])?1:0;
  for(unsigned step=0;step<20;++step){auto now=start+std::chrono::milliseconds(step*100);
   for(unsigned i=0;i<5;++i)if(i!=absent&&nodes[i]->tip().header.number==1)reactors[i]->tick(now,24);
   std::vector<Json> messages;for(unsigned i=0;i<5;++i)if(i!=absent){auto message=reactors[i]->snapshot();if(!message.is_null())messages.push_back(message);}
   for(unsigned i=0;i<5;++i)if(i!=absent&&nodes[i]->tip().header.number==1)for(const auto& message:messages)reactors[i]->receive(message,now,24);
  }
  for(unsigned i=0;i<5;++i)if(i!=absent){check(nodes[i]->tip().header.number==2&&nodes[i]->tip().header.hash()==nodes[4]->tip().header.hash(),"newcomer participates in next-height finality");auto cert=nodes[i]->store().certificate_at(2);bool found=false;for(const auto& v:cert->votes)if(address(v.validator)==address(newcomer))found=true;check(found&&cert->votes.size()==4,"newcomer supplies required fourth signature");}
  // Keep this same reactor/journal through withdrawal and re-registration.
  // Advance real certified blocks to satisfy the contract's 100-block lock.
  auto& chain=*nodes[4];auto domain=joining_genesis.block.header.hash();
  auto next_proposal=[&](){auto selected=chain.next_validator();auto signer=std::find_if(all_keys.begin(),all_keys.end(),[&](const auto& key){return address(public_address(public_key(key)))==address(*selected);});check(signer!=all_keys.end(),"known lifecycle proposer");auto block=chain.block_template(*selected,Int(chain.tip().header.timestamp+12),0);sign_block(block,*signer,1337,domain);return block;};
  auto certify=[&](const Block& candidate){auto members=chain.voting_set();std::vector<FinalityVote> votes;
   for(const auto& key:all_keys){auto signer=public_address(public_key(key));if(std::none_of(members.begin(),members.end(),[&](const auto& member){return address(member.first)==address(signer);}))continue;
    FinalityVote v;v.height=u64(candidate.header.number);v.step=VoteStep::precommit;v.validator=signer;v.validator_set_hash=voting_set_hash(members);v.block_hash=candidate.header.hash();sign_vote(v,key,1337,domain);votes.push_back(v);
   }chain.import_certified_block(candidate,0,votes,candidate.header.timestamp);
  };
  while(chain.tip().header.number<100)certify(next_proposal());
  Transaction withdrawal;withdrawal.chain_id=1337;withdrawal.nonce=chain.state().get(newcomer).nonce;withdrawal.to=registry;withdrawal.gas=500000;withdrawal.gas_price=2000000000;withdrawal.data=unhex(staking.at("methodIdentifiers").at("withdrawStake()"));
  auto signed_withdrawal=SignedTransaction::sign(withdrawal,newcomer_key);check(chain.submit(signed_withdrawal),"validator withdrawal submitted after lock period");
  auto old_members=chain.voting_set();auto departing=next_proposal();certify(departing);
  check(chain.tip().header.number==101&&chain.store().receipt(signed_withdrawal.hash())->success&&chain.stake(newcomer)==0&&chain.voting_set().size()==4,"certified withdrawal removes voting rights");
  reactors[4]->tick(start+std::chrono::seconds(100),1224);check(reactors[4]->snapshot().is_null(),"removed validator stops gossip and signing without restart");
  auto candidate=next_proposal();FinalityVote unauthorized;unauthorized.height=102;unauthorized.step=VoteStep::prevote;unauthorized.validator=newcomer;unauthorized.block_hash=candidate.header.hash();
  for(const auto& set:{old_members,chain.voting_set()}){unauthorized.validator_set_hash=voting_set_hash(set);sign_vote(unauthorized,newcomer_key,1337,domain);bool refused=false;
   try{decode_vote_message(encode_vote_message(unauthorized,1337,domain),chain.voting_set(),1337,domain,102,0);}catch(const std::exception&){refused=true;}check(refused,"removed key rejected with either old or current set hash");
  }
  tx.nonce=chain.state().get(newcomer).nonce;auto registration_again=SignedTransaction::sign(tx,newcomer_key);check(chain.submit(registration_again),"withdrawn account can submit a new signed registration");
  auto rejoining=next_proposal();certify(rejoining);check(chain.store().receipt(registration_again.hash())->success&&chain.voting_set().size()==5,"re-registration derives renewed membership");
  auto fresh=next_proposal();Json message={{"version",1},{"height","0x67"},{"round","0x0"},{"previous",nullptr},{"proposal",fresh.to_json(true)},{"votes",Json::array()}};
  reactors[4]->receive(message,start+std::chrono::seconds(101),fresh.header.timestamp);reactors[4]->tick(start+std::chrono::seconds(101),fresh.header.timestamp);
  auto resumed=reactors[4]->snapshot();bool voted=false;for(const auto& encoded:resumed.at("votes")){auto vote=decode_vote_message(encoded.get<std::string>(),chain.voting_set(),1337,domain,103,0);if(address(vote.validator)==address(newcomer)){check(vote.step==VoteStep::prevote&&vote.block_hash==fresh.header.hash(),"returning validator votes only for current proposal");voted=true;}}
  check(voted&&chain.audit().at("verified")==true,"same reactor resumes current-height voting after certified re-registration");
  auto reserved=resumed.at("votes");reactors[4].reset();nodes[4].reset();
  nodes[4]=std::make_unique<Blockchain>(root/"joining-node-4",joining_genesis);reactors[4]=std::make_unique<BftReactor>(*nodes[4],root/"joining-journal-4",newcomer_key);
  reactors[4]->tick(start+std::chrono::seconds(102),fresh.header.timestamp);
  check(reactors[4]->snapshot().at("votes")==reserved&&nodes[4]->audit().at("verified")==true,"post-re-registration vote survives chain and journal restart exactly");
 }
 FinalityVote lagged_vote;
 // Execute a weight change, then attack the parent-set boundary with correctly
 // signed certificates. Head count and the former weights cannot authorize it.
 {
  Blockchain producer(root/"weight-producer",multi),peer(root/"weight-peer",multi);
  auto original=producer.voting_set();
  {ChainStore journal(root/"weight-journal");ConsensusRound pending(peer,journal,keys[3],0,start,second,second,second);
   auto progress=pending.advance(start+second,12);check(progress.broadcast.size()==1&&!progress.broadcast[0].block_hash,"reserve a nil prevote before certified imports");lagged_vote=progress.broadcast[0];}

  auto certificate=[&](const Block& block,const VotingSet& members,const std::vector<unsigned>& signers){
   std::vector<FinalityVote> result;
   for(auto i:signers){FinalityVote v;v.height=u64(block.header.number);v.round=0;v.step=VoteStep::precommit;v.block_hash=block.header.hash();v.validator_set_hash=voting_set_hash(members);v.validator=addresses[i];sign_vote(v,keys[i],1337,multi.block.header.hash());result.push_back(v);}
   return result;
  };
  auto proposal=[&](Int time){auto selected=producer.next_validator();auto index=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*selected);})-addresses.begin();check(index<4,"known proposer after weight change");auto b=producer.block_template(*selected,time,0);sign_block(b,keys[index],1337,multi.block.header.hash());return b;};
  Transaction tx;tx.chain_id=1337;tx.nonce=producer.state().get(addresses[0]).nonce;tx.to=registry;tx.value=minimum*3;tx.gas=500000;tx.gas_price=2000000000;tx.data=unhex(staking.at("methodIdentifiers").at("addStake()"));
  auto signed_tx=SignedTransaction::sign(tx,keys[0]);check(producer.submit(signed_tx),"signed weight increase submitted");
  check(producer.voting_set()==original,"pending deposit cannot change voting power");
  auto changed=original;for(auto& member:changed)if(address(member.first)==address(addresses[0]))member.second=minimum*4;
  auto first=proposal(12);auto before_root=producer.state().root();
  bool refused=false;try{producer.import_certified_block(first,0,certificate(first,changed,{0,1}),12);}catch(const std::exception&){refused=true;}
  check(refused&&producer.tip().header.number==0&&producer.state().root()==before_root,"future weights cannot certify their own activation block");
  auto activation=certificate(first,original,{0,1,2});producer.import_certified_block(first,0,activation,12);peer.import_certified_block(first,0,activation,12);
  check(producer.store().receipt(signed_tx.hash())->success&&producer.voting_set()==changed&&peer.voting_set()==changed,"executed deposit determines next voting powers on both peers");
  auto transition=producer.store().certificate_at(1);check(transition&&transition->validators==original&&transition->next_validators==changed,"activation certificate preserves both verified sets");
  auto second_block=proposal(24);auto committed_root=peer.state().root();
  for(const auto& forged:std::vector<std::vector<FinalityVote>>{certificate(second_block,original,{1,2,3}),certificate(second_block,changed,{1,2,3}),certificate(second_block,changed,{0}),certificate(second_block,changed,{0,0})}){
   refused=false;try{peer.import_certified_block(second_block,0,forged,24);}catch(const std::exception&){refused=true;}
   check(refused&&peer.tip().header.number==1&&peer.state().root()==committed_root&&!peer.store().certificate_at(2),"old weights, insufficient weight or duplicate heavy signer cannot advance canonical state");
  }
  auto weighted_proof=certificate(second_block,changed,{0,1});
  producer.import_certified_block(second_block,0,weighted_proof,24);peer.import_certified_block(second_block,0,weighted_proof,24);
  check(peer.tip().header.hash()==producer.tip().header.hash()&&peer.state().root()==producer.state().root()&&peer.audit().at("verified")==true,"five sevenths of current weight certifies matching audited state");
 }
 {Blockchain reopened(root/"weight-peer",multi);ChainStore journal(root/"weight-journal");auto domain=multi.block.header.hash();
  check(reopened.tip().header.number==2&&reopened.stake(addresses[0])==minimum*4&&reopened.audit().at("verified")==true,"weighted transition and certified history survive reopen");
  auto before=journal.voting_position(addresses[3],1337,domain);
  check(before&&before->height==1&&before->validator_set_hash==lagged_vote.validator_set_hash,"certified import followed by close leaves journal at old height and weights");
  ConsensusRound recovered(reopened,journal,keys[3],0,start,second,second,second);
  auto after=journal.voting_position(addresses[3],1337,domain);
  check(after&&after->height==3&&after->validator_set_hash==voting_set_hash(reopened.voting_set()),"round construction recovers both imported certificates before voting");
  auto saved=journal.latest_consensus_vote(addresses[3],1337,domain);check(saved&&vote_signing_hash(*saved,1337,domain)==vote_signing_hash(lagged_vote,1337,domain),"recovery preserves old vote reservation");
  auto conflict=lagged_vote;auto block=*reopened.store().block_by_number(1);conflict.block_hash=block.header.hash();
  bool refused=false;try{journal.reserve_consensus_vote(conflict,reopened.store().certificate_at(1)->validators,{},1337,domain,&block);}catch(const std::exception&){refused=true;}
  check(refused,"recovery cannot replace old nil vote with conflicting valid proposal");
  auto next=recovered.advance(start+second,36);check(next.broadcast.size()==1&&next.broadcast[0].height==3&&next.broadcast[0].validator_set_hash==after->validator_set_hash,"only recovered current-height vote may be released");
  lagged_vote=next.broadcast[0];
  std::cout<<Json{{"scenario","certified-import-journal-lag"},{"importedTip",2},{"journalBefore",before->height},{"journalAfter",after->height},{"conflictingOldVoteRefused",true},{"newVoteHeight",lagged_vote.height}}.dump()<<std::endl;
 }
 {Blockchain reopened(root/"weight-peer",multi);ChainStore journal(root/"weight-journal");ConsensusRound resumed(reopened,journal,keys[3],0,start,second,second,second);auto result=resumed.advance(start,36);
  check(result.broadcast.size()==1&&result.broadcast[0].signature==lagged_vote.signature,"recovered-height vote resumes exact signature on second reopen");}

 {Blockchain rolled_back(root/"weight-rolled-back",multi);ChainStore journal(root/"weight-journal");auto domain=multi.block.header.hash();
  bool refused=false;try{rolled_back.recover_voting_progress(journal,addresses[3]);}catch(const std::runtime_error& error){refused=std::string(error.what())=="voting journal is ahead of canonical chain";}
  auto active=journal.voting_position(addresses[3],1337,domain);auto saved=journal.latest_consensus_vote(addresses[3],1337,domain);
  check(refused&&active&&active->height==3&&saved&&vote_signing_hash(*saved,1337,domain)==vote_signing_hash(lagged_vote,1337,domain),"rollback cannot reset an ahead journal or erase reserved votes");
 }
 // A Byzantine proposer splits the honest prevotes across two valid blocks.
 // Their gossip must still carry votes, so they can time out into a new round.
 {
  Blockchain source(root/"split-proposal-source",multi);
  auto selected=source.next_validator();
  auto attacker=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*selected);})-addresses.begin();
  check(attacker<4,"known Byzantine proposer");
  auto first=source.block_template(*selected,12,0),second_block=source.block_template(*selected,13,0);
  sign_block(first,keys[attacker],1337,multi.block.header.hash());
  sign_block(second_block,keys[attacker],1337,multi.block.header.hash());
  check(first.header.hash()!=second_block.header.hash(),"Byzantine proposer signs distinct executable blocks");
  std::vector<std::unique_ptr<Blockchain>> nodes;
  std::vector<std::unique_ptr<BftReactor>> reactors;
  for(unsigned i=0;i<4;++i)if(i!=attacker){
   nodes.push_back(std::make_unique<Blockchain>(root/("split-proposal-node-"+std::to_string(i)),multi));
   reactors.push_back(std::make_unique<BftReactor>(*nodes.back(),root/("split-proposal-journal-"+std::to_string(i)),keys[i]));
   reactors.back()->tick(start,13);
   Json envelope={{"version",1},{"height","0x1"},{"round","0x0"},{"previous",nullptr},{"votes",Json::array()},
    {"proposal",(nodes.size()==3?second_block:first).to_json(true)}};
   reactors.back()->receive(envelope,start,13);reactors.back()->tick(start,13);
  }
  auto stable=reactors[0]->snapshot();auto invalid=second_block;invalid.header.state_root[0]^=1;
  sign_block(invalid,keys[attacker],1337,multi.block.header.hash());
  auto invalid_envelope=stable;invalid_envelope["proposal"]=invalid.to_json(true);invalid_envelope["votes"]=Json::array();
  bool refused=false;try{reactors[0]->receive(invalid_envelope,start,13);}catch(const std::exception&){refused=true;}
  check(refused&&reactors[0]->snapshot()==stable,"signed alternative with invalid state is still rejected");
  for(unsigned step=0;step<150;++step){
   auto now=start+std::chrono::milliseconds(step*100);
   std::vector<Json> messages;for(auto& reactor:reactors){auto message=reactor->snapshot();if(!message.is_null())messages.push_back(message);}
   for(unsigned i=0;i<3;++i)if(nodes[i]->tip().header.number==0){
    for(const auto& message:messages)reactors[i]->receive(message,now,13);
    if(step==0)check(reactors[i]->snapshot().at("proposal")==messages[i].at("proposal"),"conflicting gossip never replaces local proposal");
    reactors[i]->tick(now,13);
   }
  }
  for(unsigned i=0;i<3;++i){
   check(nodes[i]->tip().header.number==1&&nodes[i]->tip().header.hash()==nodes[0]->tip().header.hash(),"honest reactors recover from equivocating proposer");
   auto cert=nodes[i]->store().certificate_at(1);check(cert&&cert->round>0,"split proposals cannot certify the initial round");
   check(nodes[i]->state().root()==nodes[0]->state().root()&&nodes[i]->audit().at("verified")==true,"recovered proposal has matching audited state");
  }
 }
 // One member signs arbitrary conflicting votes; the other three must still
 // commit using only their own weight, including after a locked signer restarts.
 {
  Blockchain probe(root/"byzantine-probe",multi);
  auto selected=probe.next_validator();
  auto proposer=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(*selected);})-addresses.begin();
  check(proposer<4,"known honest proposer in Byzantine test");
  unsigned attacker=(proposer+1)%4;
  std::vector<unsigned> honest;for(unsigned i=0;i<4;++i)if(i!=attacker)honest.push_back(i);
  std::vector<std::unique_ptr<Blockchain>> nodes(4);
  std::vector<std::unique_ptr<BftReactor>> reactors(4);
  auto reopen=[&](unsigned i){
   reactors[i].reset();nodes[i].reset();
   nodes[i]=std::make_unique<Blockchain>(root/("byzantine-node-"+std::to_string(i)),multi);
   reactors[i]=std::make_unique<BftReactor>(*nodes[i],root/("byzantine-journal-"+std::to_string(i)),keys[i]);
  };
  for(auto i:honest){reopen(i);reactors[i]->tick(start,12);}
  auto proposal=reactors[proposer]->snapshot();
  check(!proposal.at("proposal").is_null(),"honest proposer supplies executable block");
  for(auto i:honest){reactors[i]->receive(proposal,start,12);reactors[i]->tick(start,12);}
  auto attack=[&](unsigned salt,Clock::time_point now){
   for(auto i:honest){
    if(nodes[i]->tip().header.number!=0)continue;
    Json envelope={{"version",1},{"height","0x1"},{"round","0x0"},{"proposal",nullptr},{"previous",nullptr},{"votes",Json::array()}};
    for(auto step:{VoteStep::prevote,VoteStep::precommit}){
     FinalityVote v;v.height=1;v.round=0;v.step=step;v.validator=addresses[attacker];v.validator_set_hash=voting_set_hash(probe.voting_set());
     if(salt%2)v.block_hash=keccak(bytes(Int(1000+salt*4+i),32));
     sign_vote(v,keys[attacker],1337,multi.block.header.hash());
     envelope["votes"].push_back(encode_vote_message(v,1337,multi.block.header.hash()));
    }
    // Send each genuine signature twice, plus different targets to each peer.
    reactors[i]->receive(envelope,now,12);reactors[i]->receive(envelope,now,12);
    check(reactors[i]->snapshot().at("votes").size()<=8,"signed equivocation cannot expand outbound gossip");
   }
  };
  for(unsigned n=0;n<16;++n)attack(n,start);
  for(auto i:honest){reactors[i]->tick(start,12);check(nodes[i]->tip().header.number==0,"Byzantine signatures cannot manufacture a commit");}
  // Exchange prevotes, then lock, without delivering the resulting precommits.
  std::vector<Json> prevotes;for(auto i:honest)prevotes.push_back(reactors[i]->snapshot());
  for(auto i:honest){for(const auto& message:prevotes)reactors[i]->receive(message,start,12);reactors[i]->tick(start,12);}
  unsigned recovering=honest.front();auto locked=reactors[recovering]->snapshot();
  auto own_precommit=[&](const Json& message,unsigned index){
   for(const auto& wire:message.at("votes")){
    auto v=decode_vote_message(wire.get<std::string>(),probe.voting_set(),1337,multi.block.header.hash(),1,0);
    if(address(v.validator)==address(addresses[index])&&v.step==VoteStep::precommit)return wire.get<std::string>();
   }return std::string{};
  };
  auto reserved=own_precommit(locked,recovering);check(!reserved.empty(),"honest quorum locks despite equivocation");
  reopen(recovering);reactors[recovering]->tick(start,12);
  check(own_precommit(reactors[recovering]->snapshot(),recovering)==reserved,"locked reactor replays exact precommit after restart");
  for(unsigned step=1;step<=10;++step){
   auto now=start+std::chrono::milliseconds(step*100);attack(16+step,now);
   std::vector<Json> messages;for(auto i:honest){auto message=reactors[i]->snapshot();if(!message.is_null())messages.push_back(message);}
   for(auto i:honest)if(nodes[i]->tip().header.number==0){for(const auto& message:messages)reactors[i]->receive(message,now,12);reactors[i]->tick(now,12);}
  }
  for(auto i:honest){
   check(nodes[i]->tip().header.number==1&&nodes[i]->tip().header.hash()==nodes[proposer]->tip().header.hash()&&nodes[i]->state().root()==nodes[proposer]->state().root(),"three honest reactors converge under signed Byzantine traffic");
   auto certificate=nodes[i]->store().certificate_at(1);check(certificate&&certificate->votes.size()==3,"honest quorum alone certifies block");
   for(const auto& v:certificate->votes)check(address(v.validator)!=address(addresses[attacker]),"Byzantine vote not required in certificate");
   check(nodes[i]->audit().at("verified")==true,"Byzantine traffic leaves auditable chain");
  }
 }
 for(unsigned i=0;i<4;++i){Blockchain reopened(root/("multi-"+std::to_string(i)),multi);check(reopened.tip().header.hash()==committed_hash&&reopened.store().certificate_at(1)->votes.size()==3,"multi-validator certificate survives restart");}
 // Split precommits trigger a timeout but must not erase a local block lock.
 {Blockchain chain(root/"locked-round",multi);ChainStore journal(root/"locked-votes");
  auto selected=chain.next_validator();auto proposer=std::find_if(addresses.begin(),addresses.end(),[&](const auto& candidate){return address(candidate)==address(*selected);})-addresses.begin();check(proposer<4,"known locked-round proposer");
  auto block=chain.block_template(*selected,12);sign_block(block,keys[proposer],1337,multi.block.header.hash());
  auto members=chain.voting_set();
  auto vote=[&](unsigned index,uint64_t round,VoteStep step,std::optional<Bytes> target){FinalityVote v;v.height=1;v.round=round;v.step=step;v.validator_set_hash=voting_set_hash(members);v.block_hash=target;v.validator=addresses[index];sign_vote(v,keys[index],1337,multi.block.header.hash());return encode_vote_message(v,1337,multi.block.header.hash());};
  ConsensusRound driver(chain,journal,keys[0],0,start,second,second,second);driver.propose(block,12);driver.advance(start,12);
  driver.receive(vote(1,0,VoteStep::prevote,block.header.hash()),start);driver.receive(vote(2,0,VoteStep::prevote,block.header.hash()),start);
  auto locked=driver.advance(start,12);check(locked.broadcast.size()==1&&locked.broadcast[0].step==VoteStep::precommit,"prevote quorum establishes block lock");
  driver.receive(vote(1,0,VoteStep::precommit,std::nullopt),start);driver.receive(vote(2,0,VoteStep::precommit,std::nullopt),start);
  bool early=false;try{driver.begin_next_round(start);}catch(const std::runtime_error&){early=true;}check(early,"split participation must wait precommit timeout");
  auto timed=driver.advance(start+second,13);check(timed.next_round&&!timed.committed,"split precommits allow timeout without certificate");
  driver.begin_next_round(start+second);check(driver.round()==1,"locked validator enters next round");
  auto alternate=chain.block_template(*selected,13);sign_block(alternate,keys[proposer],1337,multi.block.header.hash());
  bool refused=false;try{driver.propose(alternate,13);}catch(const std::invalid_argument&){refused=true;}check(refused,"new round cannot replace retained locked proposal");
  auto next=driver.advance(start+second,13);check(next.broadcast.size()==1&&next.broadcast[0].round==1&&next.broadcast[0].block_hash==block.header.hash(),"next prevote preserves locked block");
  auto lock=journal.voting_lock(addresses[0],1337,multi.block.header.hash());check(lock&&lock->round==0&&lock->block_hash==block.header.hash(),"prevote does not rewrite old lock round");
  bool stale=false;try{driver.receive(vote(1,0,VoteStep::precommit,block.header.hash()),start+second);}catch(const std::exception&){stale=true;}check(stale,"old-round precommit cannot enter new quorum");
  // Rebuild coordinator from durable round-1 vote; peer votes must be retransmitted.
  ConsensusRound resumed(chain,journal,keys[0],1,start+second,second,second,second);auto replay=resumed.advance(start+second,13);check(replay.broadcast.size()==1&&replay.broadcast[0].signature==next.broadcast[0].signature&&!replay.committed,"next-round recovery keeps exact vote and old lock");
  resumed.receive(vote(1,1,VoteStep::prevote,block.header.hash()),start+second);resumed.receive(vote(2,1,VoteStep::prevote,block.header.hash()),start+second);
  auto precommit=resumed.advance(start+second,13);check(precommit.broadcast.size()==1&&precommit.broadcast[0].step==VoteStep::precommit&&!precommit.committed,"new-round quorum relocks only verified block");
  resumed.receive(vote(1,1,VoteStep::precommit,block.header.hash()),start+second);resumed.receive(vote(2,1,VoteStep::precommit,block.header.hash()),start+second);
  check(resumed.advance(start+second,13).committed&&chain.store().certificate_at(1)->round==1&&chain.tip().header.hash()==block.header.hash()&&chain.audit().at("verified")==true,"locked value certifies after timeout and same-round recovery");
 }
 // Original proposer is entirely silent. Three other validators rotate and commit.
 {std::vector<std::unique_ptr<Blockchain>> chains;std::vector<std::unique_ptr<ChainStore>> journals;std::vector<std::unique_ptr<ConsensusRound>> rounds;
  for(unsigned i=0;i<4;++i){chains.push_back(std::make_unique<Blockchain>(root/("rotation-"+std::to_string(i)),multi));journals.push_back(std::make_unique<ChainStore>(root/("rotation-votes-"+std::to_string(i))));rounds.push_back(std::make_unique<ConsensusRound>(*chains.back(),*journals.back(),keys[i],0,start,second,second,second));}
  auto initial=*chains[0]->next_validator();std::vector<unsigned> healthy;for(unsigned i=0;i<4;++i)if(address(addresses[i])!=address(initial))healthy.push_back(i);check(healthy.size()==3,"three live validators with initial proposer absent");
  auto wire=[&](const FinalityVote& vote){return encode_vote_message(vote,1337,multi.block.header.hash());};
  std::vector<FinalityVote> prevotes,precommits;
  for(auto i:healthy){auto p=rounds[i]->advance(start+second,12);check(p.broadcast.size()==1&&!p.broadcast[0].block_hash,"proposal timeout prevotes nil");prevotes.push_back(p.broadcast[0]);}
  for(auto i:healthy){for(const auto& v:prevotes)rounds[i]->receive(wire(v),start+second);auto p=rounds[i]->advance(start+second,12);check(p.broadcast.size()==1&&!p.broadcast[0].block_hash,"nil quorum precommits nil");precommits.push_back(p.broadcast[0]);}
  for(auto i:healthy){for(const auto& v:precommits)rounds[i]->receive(wire(v),start+second);check(rounds[i]->advance(start+second,12).next_round,"live majority can leave silent proposer round");rounds[i]->begin_next_round(start+second);}
  auto next=*chains[0]->next_validator(1);check(address(next)!=address(initial),"next round replaces unavailable proposer");
  auto index=std::find_if(addresses.begin(),addresses.end(),[&](const auto& a){return address(a)==address(next);})-addresses.begin();check(index<4,"rotated proposer has known test key");
  bool refused=false;try{chains[0]->block_template(initial,12,1);}catch(const std::invalid_argument&){refused=true;}check(refused,"old proposer cannot create round-1 block");
  auto block=chains[0]->block_template(next,12,1);sign_block(block,keys[index],1337,multi.block.header.hash());check(bft_proposal_round(block)==1,"proposal creation round encoded in signed header");
  auto relabeled=block;relabeled.header.extra_data=bft_round_extra(0);sign_block(relabeled,keys[index],1337,multi.block.header.hash());refused=false;try{chains[0]->verify_proposal(relabeled,12);}catch(const std::invalid_argument&){refused=true;}check(refused,"round relabeling cannot bypass proposer selection even with signature");
  prevotes.clear();precommits.clear();
  for(auto i:healthy){rounds[i]->propose(block,12);auto p=rounds[i]->advance(start+second,12);check(p.broadcast.size()==1&&p.broadcast[0].round==1,"rotated proposal prevote in new domain");prevotes.push_back(p.broadcast[0]);}
  for(auto i:healthy){for(const auto& v:prevotes)rounds[i]->receive(wire(v),start+second);auto p=rounds[i]->advance(start+second,12);check(p.broadcast.size()==1&&!p.committed,"rotated block awaits precommit quorum");precommits.push_back(p.broadcast[0]);}
  auto premature=precommits;for(unsigned n=0;n<premature.size();++n){premature[n].round=0;sign_vote(premature[n],keys[healthy[n]],1337,multi.block.header.hash());}
  refused=false;try{chains[0]->import_certified_block(block,0,premature,12);}catch(const std::invalid_argument&){refused=true;}check(refused&&chains[0]->tip().header.number==0,"valid signatures cannot certify before proposal creation round");
  for(auto i:healthy){for(const auto& v:precommits)rounds[i]->receive(wire(v),start+second);check(rounds[i]->advance(start+second,12).committed&&chains[i]->store().certificate_at(1)->round==1&&chains[i]->audit().at("verified")==true,"live validators certify after rotating away from silent proposer");}
 }
 std::cout<<"consensus round integration checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
