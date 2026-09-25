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
Bytes encode_consensus_attestation_ssz(const Phase0SignedAttestation& vote,uint32_t limit,const std::function<void()>& checkpoint){
 if(!limit||vote.aggregation_bits.size()>limit||vote.signature.size()!=96)throw std::invalid_argument("invalid attestation encoding bounds");
 if(checkpoint)checkpoint();const auto count=vote.aggregation_bits.size();Bytes out(228+count/8+1,0);
 auto number=[&](size_t at,uint64_t n,size_t width=8){for(size_t i=0;i<width;++i)out[at+i]=uint8_t(n>>(8*i));};
 auto root=[&](size_t at,const ConsensusRoot& r){std::copy(r.begin(),r.end(),out.begin()+at);};
 const auto& d=vote.data;number(0,228,4);number(4,d.slot);number(12,d.index);root(20,d.beacon_block_root);number(52,d.source.epoch);root(60,d.source.root);number(92,d.target.epoch);root(100,d.target.root);
 std::copy(vote.signature.begin(),vote.signature.end(),out.begin()+132);
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();if(vote.aggregation_bits[i])out[228+i/8]|=uint8_t(1u<<(i%8));}
 out[228+count/8]|=uint8_t(1u<<(count%8));if(checkpoint)checkpoint();return out;
}
std::optional<Phase0SignedAttestation> decode_consensus_attestation_ssz(View b,uint32_t limit,const std::function<void()>& checkpoint){
 if(!limit)throw std::invalid_argument("zero attestation committee limit");if(checkpoint)checkpoint();
 if(b.size()<229||b.size()>228+uint64_t(limit)/8+1||number(b,0,4)!=228||b.back()==0)return std::nullopt;
 const uint64_t count=(b.size()-229)*8+std::bit_width(unsigned(b.back()))-1;if(count>limit)return std::nullopt;
 Phase0SignedAttestation a{data(b),{},Bytes(b.begin()+132,b.begin()+228)};a.aggregation_bits.reserve(count);
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();a.aggregation_bits.push_back((b[228+i/8]>>(i%8))&1);}if(checkpoint)checkpoint();return a;
}
std::optional<std::pair<IndexedConsensusAttestation,IndexedConsensusAttestation>> decode_consensus_attester_slashing_ssz(View b,uint32_t limit,const std::function<void()>& checkpoint){
 if(!limit)throw std::invalid_argument("zero slashing committee limit");if(checkpoint)checkpoint();
 if(b.size()<464||b.size()>464+uint64_t(limit)*16||number(b,0,4)!=8)return std::nullopt;
 const auto second=number(b,4,4);if(second<236||second>b.size()-228)return std::nullopt;
 auto first=indexed(b.subspan(8,second-8),limit,checkpoint);if(!first)return std::nullopt;
 auto other=indexed(b.subspan(second),limit,checkpoint);if(!other)return std::nullopt;
 if(checkpoint)checkpoint();return std::pair{std::move(*first),std::move(*other)};
}
}
