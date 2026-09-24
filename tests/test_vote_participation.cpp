#include "support/process.hpp"
#include "nano/finality.hpp"
using namespace test;
int main(){try{
 unsigned checks=0,rejected=0,equivocations=0;
 for(bool heavy:{false,true})for(bool reverse:{false,true}){
  VotingSet members;Int total=0;for(unsigned i=0;i<12;++i){Int weight=heavy&&i==0?Int(1)<<200:Int(i%3+1);members.emplace_back(public_address(public_key(bytes(Int(i+1),32))),weight);total+=weight;}
  auto genesis=keccak(Bytes{91}),hash=voting_set_hash(members),block=keccak(Bytes{92});RoundVoteCollector collector(members,1337,genesis,1,0);
  std::map<std::pair<unsigned,unsigned>,Int> originals;
  auto compare=[&]{for(auto step:{VoteStep::prevote,VoteStep::precommit}){Int reference=0;for(const auto& [slot,weight]:originals)if(slot.first==static_cast<unsigned>(step))reference+=weight;check(collector.has_participation_quorum(step)==(reference*3>total*2),"participation agrees with independent rescan of first valid voters");++checks;}};
  compare();
  for(unsigned k=0;k<12;++k)for(auto step:{VoteStep::prevote,VoteStep::precommit}){
   auto i=reverse?11-k:k;FinalityVote v;v.height=1;v.round=0;v.step=step;v.validator=members[i].first;v.validator_set_hash=hash;if(i%2)v.block_hash=block;auto private_key=bytes(Int(i+1),32);sign_vote(v,private_key,1337,genesis);auto wire=encode_vote_message(v,1337,genesis);
   check(collector.receive(wire)==VoteReception::added,"first voter added");originals[{static_cast<unsigned>(step),i}]=members[i].second;compare();
   check(collector.receive(wire)==VoteReception::duplicate,"duplicate classified");compare();
   auto invalid=Json::parse(wire);invalid["signature"]=hex(Bytes(65,0));bool refused=false;try{collector.receive(invalid.dump());}catch(const std::exception&){refused=true;}check(refused,"invalid signature cannot inflate participation");++rejected;compare();
   v.block_hash=v.block_hash?std::nullopt:std::optional<Bytes>(block);sign_vote(v,private_key,1337,genesis);check(collector.receive(encode_vote_message(v,1337,genesis))==VoteReception::equivocation,"conflicting vote classified");++equivocations;compare();
  }
  check(collector.retained_votes()==48&&collector.equivocations().size()==24,"evidence remains bounded per signer and step");
 }
 std::cout<<Json{{"scenario","vote-participation-model"},{"cases",4},{"checks",checks},{"invalidSignaturesRejected",rejected},{"equivocations",equivocations},{"largeWeights",true},{"reverseOrder",true},{"nilVotes",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
