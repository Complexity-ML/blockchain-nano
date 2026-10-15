#include "nano/consensus_message_ssz.hpp"
#include <bit>
namespace nano {
namespace {
using View=std::span<const uint8_t>;
// Fixed fields are accessed only after callers checked the complete 228 bytes.
uint64_t number(View b,size_t at,size_t width=8){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
ConsensusRoot root(View b,size_t at){ConsensusRoot r{};std::copy_n(b.begin()+at,32,r.begin());return r;}
ConsensusAttestationData data(View b){return {number(b,4),number(b,12),root(b,20),{number(b,52),root(b,60)},{number(b,92),root(b,100)}};}
std::optional<IndexedConsensusAttestation> indexed(View b,uint32_t limit,const std::function<void()>& checkpoint){
 if(b.size()<228||b.size()>228+uint64_t(limit)*8||(b.size()-228)%8||number(b,0,4)!=228)return std::nullopt;
 IndexedConsensusAttestation a{data(b),{},Bytes(b.begin()+132,b.begin()+228)};a.indices.reserve((b.size()-228)/8);
 for(size_t i=228;i<b.size();i+=8){if(checkpoint)checkpoint();a.indices.push_back(number(b,i));}return a;
}
}
Bytes encode_consensus_proposer_slashing_ssz(SignedConsensusBeaconHeader first,SignedConsensusBeaconHeader second,const std::function<void()>& callback){
 if(first.signature.size()!=96||second.signature.size()!=96)throw std::invalid_argument("Invalid proposer slashing signature width");
 const auto check=callback;
 Bytes out(416);auto encode=[&](const SignedConsensusBeaconHeader& signed_header,size_t at){
  const auto& h=signed_header.message;
  for(size_t i=0;i<8;++i){out[at+i]=uint8_t(h.slot>>(8*i));out[at+8+i]=uint8_t(h.proposer_index>>(8*i));}
  std::copy(h.parent_root.begin(),h.parent_root.end(),out.begin()+at+16);
  std::copy(h.state_root.begin(),h.state_root.end(),out.begin()+at+48);
  std::copy(h.body_root.begin(),h.body_root.end(),out.begin()+at+80);
  std::copy(signed_header.signature.begin(),signed_header.signature.end(),out.begin()+at+112);
 };
 if(check)check();encode(first,0);encode(second,208);if(check)check();return out;
}
std::optional<std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>> decode_consensus_proposer_slashing_ssz(View input,const std::function<void()>& callback){
 if(input.size()!=416)return std::nullopt;
 const Bytes owned(input.begin(),input.end());const View b=owned;const auto check=callback;
 if(check)check();auto header=[&](size_t at){
  return SignedConsensusBeaconHeader{{number(b,at),number(b,at+8),root(b,at+16),root(b,at+48),root(b,at+80)},Bytes(b.begin()+at+112,b.begin()+at+208)};
 };
 auto first=header(0),second=header(208);if(check)check();return std::pair{std::move(first),std::move(second)};
}
Bytes encode_consensus_attestation_ssz(const Phase0SignedAttestation& input,uint32_t limit,const std::function<void()>& callback){
 if(!limit||input.aggregation_bits.size()>limit||input.signature.size()!=96)throw std::invalid_argument("invalid attestation encoding bounds");
 const auto vote=input;const auto checkpoint=callback;
 if(checkpoint)checkpoint();const auto count=vote.aggregation_bits.size();Bytes out(228+count/8+1,0);
 auto number=[&](size_t at,uint64_t n,size_t width=8){for(size_t i=0;i<width;++i)out[at+i]=uint8_t(n>>(8*i));};
 auto root=[&](size_t at,const ConsensusRoot& r){std::copy(r.begin(),r.end(),out.begin()+at);};
 const auto& d=vote.data;number(0,228,4);number(4,d.slot);number(12,d.index);root(20,d.beacon_block_root);number(52,d.source.epoch);root(60,d.source.root);number(92,d.target.epoch);root(100,d.target.root);
 std::copy(vote.signature.begin(),vote.signature.end(),out.begin()+132);
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();if(vote.aggregation_bits[i])out[228+i/8]|=uint8_t(1u<<(i%8));}
 out[228+count/8]|=uint8_t(1u<<(count%8));if(checkpoint)checkpoint();return out;
}
std::optional<Phase0SignedAttestation> decode_consensus_attestation_ssz(View input,uint32_t limit,const std::function<void()>& callback){
 if(!limit)throw std::invalid_argument("zero attestation committee limit");
 if(input.size()<229||input.size()>228+uint64_t(limit)/8+1)return std::nullopt;
 const Bytes owned(input.begin(),input.end());const View b=owned;const auto checkpoint=callback;if(checkpoint)checkpoint();
 if(number(b,0,4)!=228||b.back()==0)return std::nullopt;
 const uint64_t count=(b.size()-229)*8+std::bit_width(unsigned(b.back()))-1;if(count>limit)return std::nullopt;
 Phase0SignedAttestation a{data(b),{},Bytes(b.begin()+132,b.begin()+228)};a.aggregation_bits.reserve(count);
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();a.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);}if(checkpoint)checkpoint();return a;
}
Bytes encode_consensus_voluntary_exit_ssz(const SignedConsensusVoluntaryExit& input,const std::function<void()>& callback){
 if(input.signature.size()!=96)throw std::invalid_argument("Invalid voluntary exit signature width");
 const auto exit=input;const auto check=callback;if(check)check();
 Bytes out(112);for(size_t i=0;i<8;++i){out[i]=uint8_t(exit.epoch>>(8*i));out[8+i]=uint8_t(exit.validator_index>>(8*i));}
 std::copy(exit.signature.begin(),exit.signature.end(),out.begin()+16);return out;
}
std::optional<SignedConsensusVoluntaryExit> decode_consensus_voluntary_exit_ssz(std::span<const uint8_t> input,const std::function<void()>& callback){
 if(input.size()!=112)return std::nullopt;
 const Bytes owned(input.begin(),input.end());const View b=owned;const auto check=callback;if(check)check();
 return SignedConsensusVoluntaryExit{number(b,0),number(b,8),Bytes(b.begin()+16,b.end())};
}
Bytes encode_consensus_attester_slashing_ssz(const IndexedConsensusAttestation& first_input,const IndexedConsensusAttestation& second_input,uint32_t limit,const std::function<void()>& callback){
 auto bounded=[&](const auto& a){return limit&&a.indices.size()<=limit&&a.signature.size()==96&&a.indices.size()<=(UINT32_MAX-228)/8;};
 if(!bounded(first_input)||!bounded(second_input))throw std::invalid_argument("Invalid indexed attestation encoding bounds");
 const auto first=first_input,second=second_input;const auto checkpoint=callback;
 auto encode=[&](const IndexedConsensusAttestation& a){
  if(!limit||a.indices.size()>limit||a.signature.size()!=96||a.indices.size()>(UINT32_MAX-228)/8)throw std::invalid_argument("Invalid indexed attestation encoding bounds");
  // Reuse the fixed AttestationData/signature fields; replace the bitlist with indices.
  auto out=encode_consensus_attestation_ssz({a.data,{},a.signature},limit,checkpoint);out.resize(228+8*a.indices.size());
  for(size_t i=0;i<a.indices.size();++i){if(checkpoint)checkpoint();for(size_t j=0;j<8;++j)out[228+8*i+j]=uint8_t(a.indices[i]>>(8*j));}return out;
 };
 auto a=encode(first),b=encode(second);if(a.size()>UINT32_MAX-8||b.size()>UINT32_MAX-8-a.size())throw std::length_error("Slashing SSZ size overflow");
 Bytes out(8);out[0]=8;const auto second_offset=8+a.size();for(size_t i=0;i<4;++i)out[4+i]=uint8_t(second_offset>>(8*i));out.insert(out.end(),a.begin(),a.end());out.insert(out.end(),b.begin(),b.end());return out;
}
std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>> decode_consensus_attester_slashing_ssz(View input,uint32_t limit,const std::function<void()>& callback){
 if(!limit)throw std::invalid_argument("zero slashing committee limit");
 if(input.size()<464||input.size()>464+uint64_t(limit)*16)return std::nullopt;
 const Bytes owned(input.begin(),input.end());const View b=owned;const auto checkpoint=callback;if(checkpoint)checkpoint();
 if(number(b,0,4)!=8)return std::nullopt;
 const auto second=number(b,4,4);if(second<236||second>b.size()-228)return std::nullopt;
 auto first=indexed(b.subspan(8,second-8),limit,checkpoint);if(!first)return std::nullopt;
 auto other=indexed(b.subspan(second),limit,checkpoint);if(!other)return std::nullopt;
 if(checkpoint)checkpoint();return std::pair{std::move(*first),std::move(*other)};
}
}
