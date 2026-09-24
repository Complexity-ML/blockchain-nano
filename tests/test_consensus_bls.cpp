#include "support/process.hpp"
#include "nano/consensus_bls.hpp"
using namespace test;
int main(){try{
 auto fixture=load(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/consensus-bls-verify.json");unsigned valid=0,invalid=0,mutations=0;
 for(const auto& row:fixture.at("cases")){
  const auto& t=row.at("test");const auto& in=t.at("input");auto key=unhex(in.at("pubkey")),message=unhex(in.at("message")),signature=unhex(in.at("signature"));bool expected=t.at("output");
  check(verify_consensus_bls(key,message,signature)==expected,"official BLS fixture mismatch: "+row.at("path").get<std::string>());
  if(!expected){++invalid;continue;}++valid;
  auto bad_message=message;if(bad_message.empty())bad_message.push_back(1);else bad_message[0]^=1;
  check(!verify_consensus_bls(key,bad_message,signature),"altered message must fail");++mutations;
  for(size_t length:{0u,47u,49u}){auto bad=key;bad.resize(length);check(!verify_consensus_bls(bad,message,signature),"wrong public key width must fail");++mutations;}
  for(size_t length:{0u,95u,97u}){auto bad=signature;bad.resize(length);check(!verify_consensus_bls(key,message,bad),"wrong signature width must fail");++mutations;}
  Bytes infinity_key(48),infinity_sig(96);infinity_key[0]=infinity_sig[0]=0xc0;
  check(!verify_consensus_bls(infinity_key,message,signature)&&!verify_consensus_bls(key,message,infinity_sig)&&!verify_consensus_bls(infinity_key,message,infinity_sig),"infinity cannot validate a vote");mutations+=3;
 }
 check(valid>0&&invalid>0,"fixture contains positive and negative cases");
 std::cout<<Json{{"scenario","ethereum-consensus-bls-verify"},{"officialCases",valid+invalid},{"valid",valid},{"invalid",invalid},{"mutationsRejected",mutations},{"aggregateVerification",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
