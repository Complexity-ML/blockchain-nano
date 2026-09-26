#include "support/phase0_indexed_ssz.hpp"
#include "nano/consensus_message_ssz.hpp"
#include <bit>
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";constexpr uint32_t limit=2048;size_t votes=0,proofs=0,rejected=0;Bytes example_vote,example_proof;
 for(const std::string group:{"consensus-altair-attestation-official","consensus-altair-attester-slashing-official"}){
  const auto dir=fixtures/group;const auto manifest=load(dir/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908","corpus pin");
  for(const auto& row:manifest.at("cases"))for(const auto& [path,info]:row.at("files").items())if(fs::path(path).filename()=="attestation.ssz"||fs::path(path).filename()=="attester_slashing.ssz"){
   const auto b=binary(dir/path);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
   if(fs::path(path).filename()=="attestation.ssz"){
    const auto a=decode_consensus_attestation_ssz(b,limit);check(bool(a),"official vote encoding rejected");const auto reference=indexed_attestation(Bytes(b.begin(),b.begin()+228),0,228);
    check(a->data==reference.data&&a->signature==reference.signature,"vote fields changed");check(encode_consensus_attestation_ssz(*a,limit)==b,"canonical encoder changed official bytes");const size_t bits=(b.size()-229)*8+std::bit_width(unsigned(b.back()))-1;
    check(a->aggregation_bits.size()==bits,"bit count changed");for(size_t i=0;i<bits;++i)check(a->aggregation_bits[i]==bool((b[228+i/8]>>(i%8))&1),"bit changed");
    ++votes;if(example_vote.empty()&&bits>1)example_vote=b;
   }else{
    const auto a=decode_consensus_attester_slashing_ssz(b,limit);check(bool(a),"official proof encoding rejected");const auto [x,y]=attester_slashing(b);
    check(a->first.data==x.data&&a->first.signature==x.signature&&a->first.indices==x.indices&&a->second.data==y.data&&a->second.signature==y.signature&&a->second.indices==y.indices,"proof fields changed");
    check(encode_consensus_attester_slashing_ssz(a->first,a->second,limit)==b,"slashing encoder changed official bytes");
    ++proofs;if(example_proof.empty()&&x.indices.size()>1)example_proof=b;
   }
  }
 }
 check(!example_vote.empty()&&!example_proof.empty(),"missing codec examples");
 auto no_vote=[&](const Bytes& b,uint32_t cap=limit){check(!decode_consensus_attestation_ssz(b,cap),"malformed vote encoding accepted");++rejected;};
 auto no_proof=[&](const Bytes& b,uint32_t cap=limit){check(!decode_consensus_attester_slashing_ssz(b,cap),"malformed proof encoding accepted");++rejected;};
 for(size_t n=0;n<229;++n)no_vote(Bytes(example_vote.begin(),example_vote.begin()+n));
 for(size_t n=0;n<464;++n)no_proof(Bytes(example_proof.begin(),example_proof.begin()+n));
 for(unsigned bit=0;bit<32;++bit){auto b=example_vote;b[bit/8]^=1U<<(bit%8);no_vote(b);b=example_proof;b[bit/8]^=1U<<(bit%8);no_proof(b);}
 auto b=example_vote;b.back()=0;no_vote(b);b=example_vote;b.push_back(0);no_vote(b);
 no_vote(example_vote,uint32_t(decode_consensus_attestation_ssz(example_vote,limit)->aggregation_bits.size()-1));
 no_proof(example_proof,1);b=example_proof;b.push_back(0);no_proof(b);
 for(uint32_t offset:{0U,7U,235U,uint32_t(example_proof.size()-227),uint32_t(example_proof.size()+1),UINT32_MAX}){b=example_proof;for(unsigned i=0;i<4;++i)b[4+i]=uint8_t(offset>>(8*i));no_proof(b);}
 for(bool proof:{false,true}){bool bad_config=false,cancelled=false;try{if(proof)decode_consensus_attester_slashing_ssz(example_proof,0);else decode_consensus_attestation_ssz(example_vote,0);}catch(const std::invalid_argument&){bad_config=true;}try{auto cancel=[]{throw std::runtime_error("cancelled");};if(proof)decode_consensus_attester_slashing_ssz(example_proof,limit,cancel);else decode_consensus_attestation_ssz(example_vote,limit,cancel);}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancelled";}check(bad_config&&cancelled,"codec swallowed configuration/cancellation");}

 for(size_t bits:{size_t(0),size_t(1),size_t(7),size_t(8),size_t(9),size_t(limit)}){
  auto a=*decode_consensus_attestation_ssz(example_vote,limit);a.aggregation_bits.assign(bits,true);const auto encoded=encode_consensus_attestation_ssz(a,limit);const auto decoded=decode_consensus_attestation_ssz(encoded,limit);check(decoded&&decoded->aggregation_bits==a.aggregation_bits&&decoded->data==a.data&&decoded->signature==a.signature,"encoder bitlist boundary");
 }
 for(int mode:{0,1,2,3}){auto a=*decode_consensus_attestation_ssz(example_vote,limit);bool refused=false;if(mode==1)a.aggregation_bits.resize(limit+1);if(mode==2)a.signature.pop_back();try{encode_consensus_attestation_ssz(a,mode==0?0:limit,mode==3?std::function<void()>([]{throw std::runtime_error("cancelled");}):std::function<void()>());}catch(const std::exception&){refused=true;}check(refused,"encoder accepted invalid bounds or ignored cancellation");}
 for(int mode:{0,1,2,3}){
  auto pair=*decode_consensus_attester_slashing_ssz(example_proof,limit);bool refused=false;
  if(mode==1)pair.first.indices.resize(limit+1);if(mode==2)pair.second.signature.pop_back();
  try{encode_consensus_attester_slashing_ssz(pair.first,pair.second,mode==0?0:limit,mode==3?std::function<void()>([]{throw std::runtime_error("cancelled");}):std::function<void()>());}catch(const std::exception&){refused=true;}
  check(refused,"slashing encoder accepted invalid bounds or ignored cancellation");
 }
 std::cout<<Json{{"scenario","consensus-message-ssz"},{"officialVotes",votes},{"officialProofs",proofs},{"malformedOrOversizeRejected",rejected},{"networkActive",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
