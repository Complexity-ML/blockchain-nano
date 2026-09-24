#include "support/process.hpp"
#include "nano/consensus_bls.hpp"
using namespace test;
int main(){try{
 auto fixture=load(std::string(NANO_TEST_SOURCE)+"/tests/fixtures/consensus-bls-fast-aggregate.json");unsigned valid=0,invalid=0,mutations=0;
 for(const auto& row:fixture.at("cases")){
  const auto& t=row.at("test");const auto& in=t.at("input");std::vector<Bytes> keys;for(const auto& k:in.at("pubkeys"))keys.push_back(unhex(k));
  auto message=unhex(in.at("message")),signature=unhex(in.at("signature"));bool expected=t.at("output");
  check(verify_consensus_bls_fast_aggregate(keys,message,signature)==expected,"official fast aggregate mismatch: "+row.at("path").get<std::string>());
  if(!expected){++invalid;continue;}++valid;
  auto altered=message;altered[0]^=1;check(!verify_consensus_bls_fast_aggregate(keys,altered,signature),"message substitution rejected");++mutations;
  auto removed=keys;removed.pop_back();check(!verify_consensus_bls_fast_aggregate(removed,message,signature),"omitted signer rejected");++mutations;
  auto extra=keys;extra.push_back(keys[0]);check(!verify_consensus_bls_fast_aggregate(extra,message,signature),"extra signer rejected for unchanged signature");++mutations;
  for(size_t length:{0u,47u,49u}){auto bad=keys;bad[0].resize(length);check(!verify_consensus_bls_fast_aggregate(bad,message,signature),"malformed key rejected");++mutations;}
  for(size_t length:{0u,95u,97u}){auto bad=signature;bad.resize(length);check(!verify_consensus_bls_fast_aggregate(keys,message,bad),"malformed signature rejected");++mutations;}
  auto infinity=keys;infinity[0]=Bytes(48);infinity[0][0]=0xc0;check(!verify_consensus_bls_fast_aggregate(infinity,message,signature),"infinity key rejected");++mutations;
  // Negating a valid key preserves subgroup membership but cancels the sum.
  auto opposite=keys[0];opposite[0]^=0x20;std::vector<Bytes> cancellation{keys[0],opposite};
  check(!verify_consensus_bls_fast_aggregate(cancellation,message,signature),"infinity aggregate rejected");++mutations;
 }
 check(valid==3&&invalid==9,"complete official fast aggregate corpus");
 std::cout<<Json{{"scenario","consensus-bls-fast-aggregate"},{"officialCases",valid+invalid},{"valid",valid},{"invalid",invalid},{"mutationsRejected",mutations},{"possessionEstablished",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
