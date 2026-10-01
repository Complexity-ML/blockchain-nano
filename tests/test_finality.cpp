#include "support/rocksdb.hpp"
#include "nano/finality.hpp"
#include "nano/crypto.hpp"
#include <iostream>
#include "nano/storage.hpp"
#include <unistd.h>
#include <rocksdb/db.h>
using namespace nano;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 VotingSet members;for(unsigned i=1;i<=4;++i)members.emplace_back(public_address(public_key(bytes(Int(i),32))),Int(1));
 auto genesis=keccak(Bytes{1}),block=keccak(Bytes{2}),set_hash=voting_set_hash(members);
 auto reordered=members;std::reverse(reordered.begin(),reordered.end());check(voting_set_hash(reordered)==set_hash,"canonical voting set order");
 auto make=[&](unsigned key,const VotingSet& set){FinalityVote v;v.height=10;v.round=2;v.step=VoteStep::precommit;v.validator_set_hash=voting_set_hash(set);v.block_hash=block;v.validator=public_address(public_key(bytes(Int(key),32)));sign_vote(v,bytes(Int(key),32),1337,genesis);return v;};
 auto vote=make(1,members);check(verify_vote(vote,1337,genesis),"valid signature");
 auto wire=encode_vote_message(vote,1337,genesis);
 auto decoded=decode_vote_message(wire,members,1337,genesis,10,2);
 check(decoded.signature==vote.signature&&vote_signing_hash(decoded,1337,genesis)==vote_signing_hash(vote,1337,genesis),"wire round trip preserves signed digest");
 auto refuse_wire=[&](const std::string& message){bool refused=false;try{decode_vote_message(message,members,1337,genesis,10,2);}catch(const std::exception&){refused=true;}check(refused,"malformed or foreign wire vote refused");};
 auto message=Json::parse(wire);
 for(const auto& field:std::vector<std::string>{"type","version","chainId","genesisHash","height","round","step","validatorSetHash","blockHash","validator","signature"}){auto changed=message;changed.erase(field);refuse_wire(changed.dump());}
 auto changed=message;changed["power"]="0xffff";refuse_wire(changed.dump());
 changed=message;changed["version"]=1.0;refuse_wire(changed.dump());
 changed=message;changed["height"]="0x0a";refuse_wire(changed.dump());
 changed=message;changed["height"]="10";refuse_wire(changed.dump());
 changed=message;changed["height"]=10;refuse_wire(changed.dump());
 changed=message;changed["height"]="0x10000000000000000";refuse_wire(changed.dump());
 changed=message;changed["round"]="0x1";refuse_wire(changed.dump());
 changed=message;changed["chainId"]="0x53a";refuse_wire(changed.dump());
 changed=message;changed["genesisHash"]=hex(keccak(Bytes{99}));refuse_wire(changed.dump());
 changed=message;changed["validatorSetHash"]=hex(keccak(Bytes{99}));refuse_wire(changed.dump());
 changed=message;changed["blockHash"]="0x";refuse_wire(changed.dump());
 changed=message;changed["signature"]=hex(Bytes(65,0));refuse_wire(changed.dump());
 changed=message;changed["step"]="commit";refuse_wire(changed.dump());
 changed=message;changed["blockHash"]=Json{{"unexpected",true}};refuse_wire(changed.dump());
 changed=message;changed["blockHash"]=Json::array();refuse_wire(changed.dump());
 refuse_wire("{\"height\":\"0xa\","+wire.substr(1));
 refuse_wire(std::string(2049,' '));refuse_wire(wire+" {}");refuse_wire("[");
 refuse_wire(encode_vote_message(make(5,members),1337,genesis));
 auto nil_wire=vote;nil_wire.block_hash.reset();sign_vote(nil_wire,bytes(Int(1),32),1337,genesis);
 check(!decode_vote_message(encode_vote_message(nil_wire,1337,genesis),members,1337,genesis,10,2).block_hash,"wire nil is explicit null");
 auto boundary=vote;boundary.round=UINT64_MAX;sign_vote(boundary,bytes(Int(1),32),1337,genesis);
 check(decode_vote_message(encode_vote_message(boundary,1337,genesis),members,1337,genesis,10,UINT64_MAX).round==UINT64_MAX,"wire round preserves full uint64 range");
 check(!verify_vote(vote,1338,genesis)&&!verify_vote(vote,1337,keccak(Bytes{3})),"chain domain binding");
 for(unsigned field=0;field<7;++field){auto changed=vote;
  if(field==0)++changed.height;if(field==1)++changed.round;if(field==2)changed.step=VoteStep::prevote;
  if(field==3)changed.validator_set_hash[0]^=1;if(field==4)changed.block_hash.reset();if(field==5)changed.validator=members[1].first;if(field==6)changed.signature[0]^=1;
  check(!verify_vote(changed,1337,genesis),"vote tampering rejected");}
 auto nil=vote;nil.block_hash.reset();sign_vote(nil,bytes(Int(1),32),1337,genesis);check(verify_vote(nil,1337,genesis),"nil vote independently signed");
 std::vector<FinalityVote> votes{make(1,members),make(2,members),make(3,members)};
 auto valid=[&](const auto& certificate,const VotingSet& set){return verify_commit_certificate(certificate,set,10,2,block,1337,genesis);};
 check(valid(votes,members),"three of four precommits form strict quorum");
 auto duplicate=votes;duplicate[2]=duplicate[0];check(!valid(duplicate,members),"duplicate signer cannot inflate quorum");
 auto outsider=votes;outsider[2]=make(5,members);check(!valid(outsider,members),"unknown signer rejected");
 auto mixed=votes;mixed[2]=nil;check(!valid(mixed,members),"nil cannot certify a block");
 mixed=votes;mixed[2].step=VoteStep::prevote;sign_vote(mixed[2],bytes(Int(3),32),1337,genesis);check(!valid(mixed,members),"prevote cannot certify a block");
 auto three=members;three.pop_back();std::vector<FinalityVote> two{make(1,three),make(2,three)};check(!valid(two,three),"exactly two thirds is insufficient");
 auto weighted=members;weighted[0].second=Int(1)<<200;std::vector<FinalityVote> heavy{make(1,weighted)};check(valid(heavy,weighted),"quorum uses exact stake weight without overflow");
 std::vector<FinalityVote> light{make(2,weighted),make(3,weighted),make(4,weighted)};check(!valid(light,weighted),"head count cannot substitute for stake weight");
 check(!valid(votes,weighted),"votes cannot be reused for changed validator weights");
 auto invalid=members;invalid[1]=invalid[0];check(!valid(votes,invalid),"duplicate members rejected");invalid=members;invalid[0].second=0;check(!valid(votes,invalid),"zero voting power rejected");
 auto high=vote;const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");auto low=integer(Bytes(high.signature.begin()+32,high.signature.begin()+64));auto s=bytes(Int(order-low),32);std::copy(s.begin(),s.end(),high.signature.begin()+32);high.signature[64]^=1;check(!verify_vote(high,1337,genesis),"high-s rejected");
 RoundVoteCollector collector(members,1337,genesis,10,2);
 auto deliver=[&](const FinalityVote& item){return collector.receive(encode_vote_message(item,1337,genesis));};
 check(deliver(votes[0])==VoteReception::added,"collector accepts first vote");
 for(unsigned i=0;i<20;++i)check(deliver(votes[0])==VoteReception::duplicate,"repeated relays do not add voting power");
 check(collector.retained_votes()==1&&!collector.has_participation_quorum(VoteStep::precommit)&&collector.certificate(VoteStep::precommit,block).empty(),"one signer cannot create quorum by repetition");
 auto contradictory=votes[0];contradictory.block_hash=keccak(Bytes{77});sign_vote(contradictory,bytes(Int(1),32),1337,genesis);
 check(deliver(contradictory)==VoteReception::equivocation,"collector detects double vote");
 for(unsigned i=0;i<20;++i){contradictory.block_hash=keccak(Bytes{uint8_t(i)});sign_vote(contradictory,bytes(Int(1),32),1337,genesis);deliver(contradictory);}
 auto evidence=collector.equivocations();check(collector.retained_votes()==2&&evidence.size()==1&&evidence[0].first.block_hash!=evidence[0].second.block_hash,"equivocation evidence memory is bounded");
 check(verify_vote(evidence[0].first,1337,genesis)&&verify_vote(evidence[0].second,1337,genesis),"equivocation retains both verifiable signatures");
 deliver(votes[1]);check(collector.certificate(VoteStep::precommit,block).empty(),"two of four remains below quorum despite equivocation");
 deliver(votes[2]);auto collected=collector.certificate(VoteStep::precommit,block);
 check(collector.has_participation_quorum(VoteStep::precommit)&&valid(collected,members),"collector certificate passes independent weighted verification");
 RoundVoteCollector reverse_collector(members,1337,genesis,10,2);for(auto it=votes.rbegin();it!=votes.rend();++it)reverse_collector.receive(encode_vote_message(*it,1337,genesis));
 auto reversed_certificate=reverse_collector.certificate(VoteStep::precommit,block);check(reversed_certificate.size()==collected.size(),"arrival order preserves certificate size");
 for(size_t i=0;i<collected.size();++i)check(collected[i].signature==reversed_certificate[i].signature,"certificate order is deterministic by signer");
 RoundVoteCollector fragmented(members,1337,genesis,10,2);
 for(unsigned i=1;i<=3;++i){auto item=make(i,members);item.block_hash=i==1?std::optional<Bytes>(block):std::nullopt;sign_vote(item,bytes(Int(i),32),1337,genesis);fragmented.receive(encode_vote_message(item,1337,genesis));}
 check(fragmented.has_participation_quorum(VoteStep::precommit)&&fragmented.certificate(VoteStep::precommit,block).empty()&&fragmented.certificate(VoteStep::precommit,std::nullopt).empty(),"participation and nil votes do not imply block finality");
 for(unsigned i=1;i<=3;++i){auto item=make(i,members);item.step=VoteStep::prevote;item.block_hash.reset();sign_vote(item,bytes(Int(i),32),1337,genesis);fragmented.receive(encode_vote_message(item,1337,genesis));}
 check(verify_prevote_certificate(fragmented.certificate(VoteStep::prevote,std::nullopt),members,10,2,std::nullopt,1337,genesis),"nil prevote quorum remains available for lock transitions");
 RoundVoteCollector heavy_collector(weighted,1337,genesis,10,2);heavy_collector.receive(encode_vote_message(heavy[0],1337,genesis));
 check(valid(heavy_collector.certificate(VoteStep::precommit,block),weighted),"collector uses stake rather than signer count");
 auto retained=collector.retained_votes();bool bad_message=false;try{collector.receive(encode_vote_message(boundary,1337,genesis));}catch(const std::exception&){bad_message=true;}check(bad_message&&collector.retained_votes()==retained,"future-round message cannot mutate current collection");
 using Millis=std::chrono::milliseconds;auto epoch=RoundWait::Clock::time_point{};
 auto waiting=[&](){return RoundWait(members,1337,genesis,10,2,epoch,Millis(1000),Millis(1000),Millis(1000));};
 auto silent=waiting();check(!silent.proposal_expired(epoch+Millis(999))&&silent.proposal_expired(epoch+Millis(1000)),"proposal timeout boundary");
 check(!silent.prevote_ready(epoch+Millis(10000))&&!silent.next_round_ready(epoch+Millis(10000)),"silence alone never starts vote timeouts");
 auto split_wait=waiting();std::vector<FinalityVote> split_votes;
 for(unsigned i=1;i<=3;++i){auto item=make(i,members);item.step=VoteStep::prevote;item.block_hash=i==1?std::nullopt:std::optional<Bytes>(keccak(Bytes{uint8_t(i)}));sign_vote(item,bytes(Int(i),32),1337,genesis);split_votes.push_back(item);split_wait.receive(encode_vote_message(item,1337,genesis),epoch+Millis(i==3?100:i*10));}
 check(!split_wait.prevote_ready(epoch+Millis(1099)),"prevote timeout starts at quorum arrival rather than round start");
 split_wait.receive(encode_vote_message(split_votes[0],1337,genesis),epoch+Millis(1099));
 check(split_wait.prevote_ready(epoch+Millis(1100))&&!split_wait.prevote_quorum(),"duplicate cannot restart split-prevote timeout");
 for(unsigned i=0;i<split_votes.size();++i){auto item=split_votes[i];item.step=VoteStep::precommit;sign_vote(item,bytes(Int(i+1),32),1337,genesis);split_wait.receive(encode_vote_message(item,1337,genesis),epoch+Millis(1100));}
 check(!split_wait.next_round_ready(epoch+Millis(2099))&&split_wait.next_round_ready(epoch+Millis(2100)),"split precommit timeout boundary");
 bool reversed_time=false;try{split_wait.next_round_ready(epoch+Millis(2099));}catch(const std::exception&){reversed_time=true;}check(reversed_time,"deadline state rejects backwards clock");
 auto commit_wait=waiting();for(const auto& item:votes)commit_wait.receive(encode_vote_message(item,1337,genesis),epoch);
 check(!commit_wait.next_round_ready(epoch+Millis(10000))&&valid(commit_wait.commit_quorum()->votes,members),"commit quorum takes priority over next-round timeout");
 auto nil_wait=waiting();for(unsigned i=1;i<=3;++i){auto item=make(i,members);item.step=VoteStep::prevote;item.block_hash.reset();sign_vote(item,bytes(Int(i),32),1337,genesis);nil_wait.receive(encode_vote_message(item,1337,genesis),epoch);}
 check(nil_wait.prevote_ready(epoch)&&nil_wait.prevote_quorum()&&!nil_wait.prevote_quorum()->block_hash,"nil prevote quorum ends wait immediately");
 for(unsigned i=1;i<=3;++i){auto item=make(i,members);item.block_hash.reset();sign_vote(item,bytes(Int(i),32),1337,genesis);nil_wait.receive(encode_vote_message(item,1337,genesis),epoch);}
 check(nil_wait.next_round_ready(epoch)&&!nil_wait.commit_quorum(),"nil precommit quorum advances round without committing block");
 VoteLock unlocked{10,set_hash,std::nullopt,std::nullopt};
 auto prevotes=votes;for(unsigned i=0;i<prevotes.size();++i){prevotes[i].step=VoteStep::prevote;sign_vote(prevotes[i],bytes(Int(i+1),32),1337,genesis);}
 check(verify_prevote_certificate(prevotes,members,10,2,block,1337,genesis),"valid prevote certificate");
 auto locked=advance_vote_lock(unlocked,vote,prevotes,members,1337,genesis);
 check(locked.round==2&&locked.block_hash==block,"prevote quorum locks block before precommit");
 auto rejected=[&](const VoteLock& state,const FinalityVote& v,const std::vector<FinalityVote>& proof){bool failed=false;try{advance_vote_lock(state,v,proof,members,1337,genesis);}catch(const std::exception&){failed=true;}check(failed,"invalid lock transition rejected");};
 rejected(unlocked,vote,{});rejected(unlocked,vote,votes);
 auto future=vote;future.round=4;future.step=VoteStep::prevote;
 check(advance_vote_lock(locked,future,{},members,1337,genesis).block_hash==block,"locked validator can prevote same block in later round");
 auto different=keccak(Bytes{8});future.block_hash=different;rejected(locked,future,{});
 auto proof=prevotes;auto resign=[&](){for(unsigned i=0;i<proof.size();++i)sign_vote(proof[i],bytes(Int(i+1),32),1337,genesis);};
 for(auto& v:proof)v.block_hash=different;resign();rejected(locked,future,proof);
 for(auto& v:proof)v.round=3;resign();auto released=advance_vote_lock(locked,future,proof,members,1337,genesis);
 check(!released.round&&!released.block_hash,"newer earlier-round quorum permits conflicting prevote and unlocks");
 auto third=keccak(Bytes{91});auto third_proposal=future;third_proposal.block_hash=third;
 auto third_unlocked=advance_vote_lock(locked,third_proposal,proof,members,1337,genesis);
 check(!third_unlocked.block_hash,"newer PoLC releases old lock before choosing a different valid proposal");
 auto same_lock_proof=proof;for(unsigned i=0;i<same_lock_proof.size();++i){same_lock_proof[i].block_hash=block;sign_vote(same_lock_proof[i],bytes(Int(i+1),32),1337,genesis);}
 rejected(locked,third_proposal,same_lock_proof);
 auto nil_unlock_proof=proof;for(unsigned i=0;i<nil_unlock_proof.size();++i){nil_unlock_proof[i].block_hash.reset();sign_vote(nil_unlock_proof[i],bytes(Int(i+1),32),1337,genesis);}
 check(!advance_vote_lock(locked,third_proposal,nil_unlock_proof,members,1337,genesis).block_hash,"newer nil PoLC permits current valid proposal");
 auto kept_proposal=future;kept_proposal.block_hash=block;
 check(!advance_vote_lock(locked,kept_proposal,proof,members,1337,genesis).block_hash,"newer different PoLC unlocks even when current proposal repeats old block");
 future.step=VoteStep::precommit;rejected(locked,future,proof);
 for(auto& v:proof)v.round=4;resign();auto relocked=advance_vote_lock(locked,future,proof,members,1337,genesis);
 check(relocked.round==4&&relocked.block_hash==different,"current quorum relocks before precommit");
 future.block_hash.reset();check(advance_vote_lock(relocked,future,{},members,1337,genesis).block_hash==different,"nil without quorum preserves lock");
 for(auto& v:proof)v.block_hash.reset();resign();check(!advance_vote_lock(relocked,future,proof,members,1337,genesis).round,"nil quorum unlocks");
 auto bad_lock=relocked;bad_lock.height=11;rejected(bad_lock,future,proof);bad_lock=relocked;bad_lock.validator_set_hash[0]^=1;rejected(bad_lock,future,proof);
 proof.pop_back();rejected(relocked,future,proof);
 auto path=std::filesystem::temp_directory_path()/("nano-voting-"+std::to_string(getpid()));
 auto reserved=vote;reserved.signature.clear();
 {ChainStore journal(path);journal.reserve_vote(reserved,1337,genesis);}
 {ChainStore journal(path);sign_vote_durable(journal,reserved,bytes(Int(1),32),1337,genesis);
  check(reserved.signature==vote.signature,"reservation resumes identical signature after reopen");
  auto refuse=[&](FinalityVote proposed){proposed.signature.clear();bool rejected=false;try{sign_vote_durable(journal,proposed,bytes(Int(1),32),1337,genesis);}catch(const std::exception&){rejected=true;}check(rejected&&proposed.signature.empty(),"rejected vote releases no signature");};
  auto conflict=reserved;conflict.block_hash=keccak(Bytes{9});refuse(conflict);
  conflict=reserved;conflict.block_hash.reset();refuse(conflict);
  conflict=reserved;conflict.validator_set_hash[0]^=1;refuse(conflict);
  auto older=reserved;older.round=1;refuse(older);
  older=reserved;older.step=VoteStep::prevote;refuse(older);
  auto next=reserved;++next.round;next.step=VoteStep::prevote;sign_vote_durable(journal,next,bytes(Int(1),32),1337,genesis);check(verify_vote(next,1337,genesis),"next round can reserve a new vote");
  sign_vote_durable(journal,reserved,bytes(Int(1),32),1337,genesis);check(reserved.signature==vote.signature,"exact old vote is idempotent after watermark advances");
  auto next_height=reserved;++next_height.height;next_height.round=0;next_height.step=VoteStep::prevote;sign_vote_durable(journal,next_height,bytes(Int(1),32),1337,genesis);
  auto back=reserved;++back.round;back.step=VoteStep::precommit;refuse(back);
  auto other_chain=reserved;sign_vote_durable(journal,other_chain,bytes(Int(1),32),1338,genesis);check(verify_vote(other_chain,1338,genesis),"journal separates chain domains");
 }
 {ChainStore journal(path);auto conflict=reserved;conflict.block_hash.reset();bool refused=false;try{sign_vote_durable(journal,conflict,bytes(Int(1),32),1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"conflict still refused after second reopen");}
 auto first_vote=vote;first_vote.height=1;sign_vote(first_vote,bytes(Int(1),32),1337,genesis);
 auto first_prevotes=prevotes;for(unsigned i=0;i<first_prevotes.size();++i){first_prevotes[i].height=1;sign_vote(first_prevotes[i],bytes(Int(i+1),32),1337,genesis);}
 auto locked_path=path.string()+"-locked";
 {ChainStore journal(locked_path);check(!journal.latest_consensus_vote(first_vote.validator,1337,genesis)&&!journal.voting_lock(first_vote.validator,1337,genesis),"fresh journal has no reserved vote or lock");journal.reserve_consensus_vote(first_vote,members,first_prevotes,1337,genesis);}
 {ChainStore journal(locked_path);auto saved=journal.latest_consensus_vote(first_vote.validator,1337,genesis);
  check(saved&&saved->signature.empty()&&vote_signing_hash(*saved,1337,genesis)==vote_signing_hash(first_vote,1337,genesis),"journal recovers exact unsigned reservation after reopen");
  check(!journal.latest_consensus_vote(first_vote.validator,1338,genesis),"vote recovery separates chain domains");auto resumed=*saved;
  sign_consensus_vote(journal,resumed,bytes(Int(1),32),members,{},1337,genesis);
  check(resumed.signature==first_vote.signature,"atomic lock and vote reservation resume after reopen");
  auto lock=journal.voting_lock(first_vote.validator,1337,genesis);check(lock&&lock->height==1&&lock->round==2&&lock->block_hash==block&&lock->validator_set_hash==voting_set_hash(members),"active lock recovered with parent set and round");
  auto conflicting=first_vote;conflicting.round=3;conflicting.step=VoteStep::prevote;conflicting.block_hash=different;conflicting.signature.clear();bool refused=false;
  try{sign_consensus_vote(journal,conflicting,bytes(Int(1),32),members,{},1337,genesis);}catch(const std::exception&){refused=true;}
  check(refused&&conflicting.signature.empty(),"persisted lock forbids unsupported conflicting prevote");
  refused=false;try{sign_vote_durable(journal,conflicting,bytes(Int(1),32),1337,genesis);}catch(const std::exception&){refused=true;}
  check(refused,"raw reservation cannot bypass locked journal mode");
  conflicting.block_hash=block;sign_consensus_vote(journal,conflicting,bytes(Int(1),32),members,{},1337,genesis);
  auto nil_commit=conflicting;nil_commit.step=VoteStep::precommit;nil_commit.block_hash.reset();sign_consensus_vote(journal,nil_commit,bytes(Int(1),32),members,{},1337,genesis);
 }
 {ChainStore journal(locked_path);auto conflicting=first_vote;conflicting.round=4;conflicting.step=VoteStep::prevote;conflicting.block_hash=different;conflicting.signature.clear();bool refused=false;
  try{sign_consensus_vote(journal,conflicting,bytes(Int(1),32),members,{},1337,genesis);}catch(const std::exception&){refused=true;}
  check(refused,"nil precommit did not erase persisted lock");
  auto retained=journal.voting_lock(first_vote.validator,1337,genesis);check(retained&&retained->round==2&&retained->block_hash==block,"nil precommit recovery exposes retained block");
  auto newer=first_prevotes;for(unsigned i=0;i<newer.size();++i){newer[i].round=3;newer[i].block_hash=different;sign_vote(newer[i],bytes(Int(i+1),32),1337,genesis);}
  conflicting.block_hash=third;
  sign_consensus_vote(journal,conflicting,bytes(Int(1),32),members,newer,1337,genesis);
  check(verify_vote(conflicting,1337,genesis),"verified newer quorum unlocks persisted lock");
  auto unlocked=journal.voting_lock(first_vote.validator,1337,genesis);check(unlocked&&!unlocked->round&&!unlocked->block_hash,"newer proof clears recovered lock");
 }
 {ChainStore journal(path);bool refused=false;try{journal.reserve_consensus_vote(first_vote,members,first_prevotes,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"raw journal cannot silently upgrade without lock history");}
 auto certificate=first_prevotes;for(unsigned i=0;i<certificate.size();++i){certificate[i].step=VoteStep::precommit;sign_vote(certificate[i],bytes(Int(i+1),32),1337,genesis);}
 auto next_members=members;next_members[0].second=2;
 {ChainStore journal(locked_path);auto next=first_vote;next.height=2;next.round=0;next.step=VoteStep::prevote;next.signature.clear();bool refused=false;
  try{sign_consensus_vote(journal,next,bytes(Int(1),32),members,{},1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"next height cannot be signed without certificate");
  auto insufficient=certificate;insufficient.pop_back();refused=false;try{journal.advance_voting_height(next.validator,1,2,block,insufficient,members,next_members,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"insufficient quorum cannot advance height");
  journal.advance_voting_height(next.validator,1,2,block,certificate,members,next_members,1337,genesis);
  auto next_lock=journal.voting_lock(first_vote.validator,1337,genesis);check(next_lock&&next_lock->height==2&&!next_lock->block_hash&&next_lock->validator_set_hash==voting_set_hash(next_members),"certified new height starts without old block lock");
 }
 {ChainStore journal(locked_path);auto next=first_vote;next.height=2;next.round=0;next.step=VoteStep::prevote;next.signature.clear();bool refused=false;
  try{sign_consensus_vote(journal,next,bytes(Int(1),32),members,{},1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"old weights cannot replace certified next set after reopen");
  next.validator_set_hash=voting_set_hash(next_members);sign_consensus_vote(journal,next,bytes(Int(1),32),next_members,{},1337,genesis);check(verify_vote(next,1337,genesis),"certified next height and set survive reopen");
  journal.advance_voting_height(next.validator,1,2,block,certificate,members,next_members,1337,genesis);
  refused=false;try{journal.advance_voting_height(next.validator,1,2,block,certificate,members,members,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"accepted certificate cannot silently change next set");
  auto skipped=next;skipped.height=4;refused=false;try{sign_consensus_vote(journal,skipped,bytes(Int(1),32),next_members,{},1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"uncertified height skip rejected");
 }
 auto observer_path=path.string()+"-observer";
 {ChainStore journal(observer_path);journal.advance_voting_height(first_vote.validator,1,2,block,certificate,members,next_members,1337,genesis);}
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,observer_path,db).ok(),"open observer corruption fixture");
  auto active_key="voting/v1/"+hex(bytes(Int(1337),32))+"/"+hex(genesis)+"/"+address(first_vote.validator)+"/active";
  rocksdb::WriteOptions write;write.sync=true;check(db->Delete(write,active_key).ok(),"remove active height metadata fixture");}
 {ChainStore journal(observer_path);bool refused=false;try{journal.reserve_consensus_vote(first_vote,members,first_prevotes,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"missing active metadata cannot restart certified observer at height one");
  refused=false;try{journal.advance_voting_height(first_vote.validator,1,2,block,certificate,members,next_members,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"idempotent certificate does not hide missing active metadata");}
 auto recovery_path=path.string()+"-recovery";
 auto nil_reserved=first_vote;nil_reserved.step=VoteStep::prevote;nil_reserved.block_hash.reset();nil_reserved.signature.clear();
 {ChainStore journal(recovery_path);sign_consensus_vote(journal,nil_reserved,bytes(Int(1),32),members,{},1337,genesis);}
 {ChainStore journal(recovery_path);auto saved=journal.latest_consensus_vote(first_vote.validator,1337,genesis);check(saved&&!saved->block_hash,"nil reservation survives reopen");}
 auto prefix="voting/v1/"+hex(bytes(Int(1337),32))+"/"+hex(genesis)+"/"+address(first_vote.validator)+"/";
 const auto fields_key=prefix+"reserved/"+hex(bytes(Int(1),8))+"/"+hex(bytes(Int(2),8))+"/0x01";
 std::string original_fields;
 {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,recovery_path,db).ok(),"open recovery fixture");check(db->Get(rocksdb::ReadOptions(),fields_key,&original_fields).ok(),"read reservation fixture");}
 for(unsigned fault=0;fault<3;++fault){
  {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,recovery_path,db).ok(),"open recovery corruption fixture");rocksdb::WriteOptions write;write.sync=true;
   auto changed=Json::parse(original_fields);changed["blockHash"]=hex(block);
   if(fault==1)changed["step"]=257;
   check((fault==2?db->Delete(write,fields_key):db->Put(write,fields_key,changed.dump())).ok(),"mutate reserved fields fixture");}
  {ChainStore journal(recovery_path);bool refused=false;try{journal.latest_consensus_vote(first_vote.validator,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"recovery refuses changed or missing reserved fields");}
 }
 for(bool locked:{false,true}){
  auto damaged=path.string()+(locked?"-missing-locked-watermark":"-missing-raw-watermark");
  auto initial=nil_reserved;initial.signature.clear();auto next=initial;next.round=3;
  auto reserve=[&](ChainStore& journal,const FinalityVote& value){if(locked)journal.reserve_consensus_vote(value,members,{},1337,genesis);else journal.reserve_vote(value,1337,genesis);};
  {ChainStore journal(damaged);reserve(journal,initial);}
  std::string old_watermark;
  {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,damaged,db).ok(),"open watermark fixture");check(db->Get(rocksdb::ReadOptions(),prefix+"watermark",&old_watermark).ok(),"read original watermark");}
  {ChainStore journal(damaged);reserve(journal,next);}
  for(bool missing:{false,true}){
   {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,damaged,db).ok(),"open damaged watermark fixture");rocksdb::WriteOptions write;write.sync=true;check((missing?db->Delete(write,prefix+"watermark"):db->Put(write,prefix+"watermark",old_watermark)).ok(),"damage watermark");}
   {ChainStore journal(damaged);auto later=next;later.round=4;bool refused=false;try{reserve(journal,later);}catch(const std::exception&){refused=true;}check(refused,"missing or regressed watermark cannot authorize a new slot");
    if(locked){refused=false;try{journal.latest_consensus_vote(initial.validator,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"damaged watermark cannot report fresh or obsolete vote");
     refused=false;try{journal.voting_position(initial.validator,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"damaged watermark blocks voting recovery position");
     refused=false;try{journal.advance_voting_height(initial.validator,1,2,block,certificate,members,next_members,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"certificate cannot conceal damaged voting watermark");}
   }
  }
  std::filesystem::remove_all(damaged);
 }
 auto bad_lock_path=path.string()+"-bad-lock";
 {ChainStore journal(bad_lock_path);journal.reserve_consensus_vote(first_vote,members,first_prevotes,1337,genesis);}
 auto lock_key=prefix+"lock/"+hex(bytes(Int(1),8));
 Json original_lock={{"validatorSetHash",hex(voting_set_hash(members))},{"round","0x2"},{"blockHash",hex(block)}};
 for(unsigned fault=0;fault<5;++fault){
  {rocksdb::Options options;std::unique_ptr<rocksdb::DB> db;check(test::open_rocksdb(options,bad_lock_path,db).ok(),"open lock corruption fixture");auto changed=original_lock;
   if(fault==0)changed["validatorSetHash"]=hex(keccak(Bytes{99}));
   if(fault==1)changed["round"]="0x3";
   if(fault==2)changed["blockHash"]=nullptr;
   if(fault==3)changed["blockHash"]="0x01";
   rocksdb::WriteOptions write;write.sync=true;check((fault==4?db->Delete(write,lock_key):db->Put(write,lock_key,changed.dump())).ok(),"damage active lock");}
  {ChainStore journal(bad_lock_path);bool refused=false;try{journal.voting_lock(first_vote.validator,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"invalid or absent active lock cannot be recovered as unlocked");
   refused=false;try{journal.voting_position(first_vote.validator,1337,genesis);}catch(const std::exception&){refused=true;}check(refused,"position recovery also checks lock integrity");}
 }
 std::filesystem::remove_all(bad_lock_path);
 std::filesystem::remove_all(recovery_path);
 std::filesystem::remove_all(observer_path);
 std::filesystem::remove_all(locked_path);
 std::filesystem::remove_all(path);
 std::cout<<"Finality vote domains and weighted certificates passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
