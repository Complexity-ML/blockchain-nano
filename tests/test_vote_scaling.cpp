#include "support/process.hpp"
#include "nano/finality.hpp"
#include <sys/resource.h>
using namespace test;
int main(int argc,char** argv){try{
 std::vector<unsigned> sizes{4u,16u,64u,256u,1000u,1001u,4096u};
 if(argc==2){std::string value=argv[1];check(!value.empty()&&value.find_first_not_of("0123456789")==std::string::npos,"expected decimal validator count");auto n=std::stoul(value);check(n>=1&&n<=1000000,"local test budget is 1 to 1000000 validators");sizes={static_cast<unsigned>(n)};alarm(3600);}
 else check(argc==1,"expected optional validator count");
 auto began_all=std::chrono::steady_clock::now();
 auto progress=[&](const char* stage,unsigned completed,unsigned total){if(argc==1||completed%10000!=0)return;struct rusage usage{};check(getrusage(RUSAGE_SELF,&usage)==0,"read resource use");
 #ifdef __APPLE__
 auto rss=static_cast<uint64_t>(usage.ru_maxrss);
 #else
 auto rss=static_cast<uint64_t>(usage.ru_maxrss)*1024;
 #endif
 check(rss<6ull*1024*1024*1024,"local test exceeded 6 GiB RSS budget");
 std::cout<<Json{{"stage",stage},{"completed",completed},{"validators",total},{"peakRssBytes",rss},{"elapsedSeconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-began_all).count()}}.dump()<<std::endl;};
 const auto genesis=keccak(Bytes{41}),block=keccak(Bytes{42});
 for(unsigned n:sizes){
  VotingSet members;for(unsigned i=1;i<=n;++i){members.emplace_back(public_address(public_key(bytes(Int(i),32))),1);progress("members",i,n);}
  const auto set_hash=voting_set_hash(members);std::vector<std::string> wires;size_t wire_bytes=0;
  for(unsigned i=1;i<=n;++i){FinalityVote v;v.height=1;v.round=0;v.step=VoteStep::precommit;v.validator_set_hash=set_hash;v.block_hash=block;v.validator=members[i-1].first;sign_vote(v,bytes(Int(i),32),1337,genesis);wires.push_back(encode_vote_message(v,1337,genesis));wire_bytes+=wires.back().size();progress("signed",i,n);}
  auto began=std::chrono::steady_clock::now();RoundVoteCollector collector(members,1337,genesis,1,0);auto constructed=std::chrono::steady_clock::now();
  for(unsigned i=0;i<n;++i){check(collector.receive(wires[i])==VoteReception::added,"unique vote accepted");check(collector.has_participation_quorum(VoteStep::precommit)==((i+1)*3>n*2),"strict quorum maintained at each prefix");progress("received",i+1,n);}
  auto received=std::chrono::steady_clock::now();for(unsigned i=0;i<n;++i){check(collector.receive(wires[i])==VoteReception::duplicate,"duplicate does not add stake");progress("duplicates",i+1,n);}auto duplicated=std::chrono::steady_clock::now();
  auto polling_begin=std::chrono::steady_clock::now();for(unsigned poll=0;poll<5000;++poll)check(collector.has_participation_quorum(VoteStep::precommit)&&!collector.has_participation_quorum(VoteStep::prevote),"repeated step-specific participation queries");auto polling_end=std::chrono::steady_clock::now();
  auto certificate=collector.certificate(VoteStep::precommit,block);check(certificate.size()==n&&verify_commit_certificate(certificate,members,1,0,block,1337,genesis),"independent complete certificate verification");
  progress("certificate-verified",n,n);
  auto original=decode_vote_message(wires[0],members,1337,genesis,1,0);unsigned rejected=0;
  auto refuse=[&](const std::string& wire){bool no=false;try{collector.receive(wire);}catch(const std::exception&){no=true;}check(no,"invalid replay rejected");++rejected;};
  auto changed=members;changed[0].second=2;auto foreign=original;foreign.validator_set_hash=voting_set_hash(changed);sign_vote(foreign,bytes(Int(1),32),1337,genesis);refuse(encode_vote_message(foreign,1337,genesis));
  auto outsider=original;outsider.validator=public_address(public_key(bytes(Int(n+1),32)));sign_vote(outsider,bytes(Int(n+1),32),1337,genesis);refuse(encode_vote_message(outsider,1337,genesis));
  auto invalid=Json::parse(wires[0]);invalid["signature"]=hex(Bytes(65,0));refuse(invalid.dump());invalid=Json::parse(wires[0]);invalid["round"]="0x1";refuse(invalid.dump());
  auto conflict=original;conflict.block_hash=keccak(Bytes{43});sign_vote(conflict,bytes(Int(1),32),1337,genesis);check(collector.receive(encode_vote_message(conflict,1337,genesis))==VoteReception::equivocation&&collector.equivocations().size()==1&&collector.retained_votes()==n+1,"conflict evidence bounded and first vote preserved");
  RoundVoteCollector replacement(changed,1337,genesis,1,0);check(replacement.receive(encode_vote_message(foreign,1337,genesis))==VoteReception::added,"new collector accepts new verified membership");
  progress("complete",n,n);
  auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
  std::cout<<Json{{"scenario","vote-collector-scaling"},{"validators",n},{"constructMs",ms(began,constructed)},{"receiveAndParticipationMs",ms(constructed,received)},{"duplicateMs",ms(received,duplicated)},{"participationPolls",5000},{"participationPollingMs",ms(polling_begin,polling_end)},{"wireBytes",wire_bytes},{"certificateVotes",certificate.size()},{"rejected",rejected},{"conflictEvidence",1},{"newSetAccepted",true}}.dump()<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
