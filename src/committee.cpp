#include "nano/committee.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <limits>
namespace nano {
std::vector<uint64_t> shuffled_validator_indices(uint64_t count,const std::array<uint8_t,32>& seed,uint16_t rounds){
 // The hash input encodes bucket numbers in four bytes, 256 indices per bucket.
 if(count>(uint64_t(1)<<40)||count>std::numeric_limits<size_t>::max()||rounds==0||rounds>256)throw std::invalid_argument("invalid shuffle domain or rounds");
 std::vector<uint64_t> result(static_cast<size_t>(count));std::iota(result.begin(),result.end(),uint64_t(0));if(!count)return result;
 std::array<uint8_t,37> input{};std::copy(seed.begin(),seed.end(),input.begin());
 std::vector<std::array<uint8_t,32>> buckets(static_cast<size_t>((count+255)/256));
 for(uint16_t round=0;round<rounds;++round){
  input[32]=static_cast<uint8_t>(round);std::array<uint8_t,32> digest{};SHA256(input.data(),33,digest.data());
  uint64_t pivot=0;for(unsigned b=0;b<8;++b)pivot|=uint64_t(digest[b])<<(8*b);pivot%=count;
  for(size_t bucket=0;bucket<buckets.size();++bucket){for(unsigned b=0;b<4;++b)input[33+b]=static_cast<uint8_t>(uint64_t(bucket)>>(8*b));SHA256(input.data(),input.size(),buckets[bucket].data());}
  for(auto& index:result){auto flip=(pivot+count-index)%count;auto position=std::max(index,flip);auto bit=(buckets[position/256][(position%256)/8]>>(position%8))&1;if(bit)index=flip;}
 }
 return result;
}
std::vector<uint64_t> partition_committee(std::span<const uint64_t> shuffled,uint64_t index,uint64_t count){
 if(!count||index>=count)throw std::invalid_argument("invalid committee index or count");
 // Wide intermediate keeps partition boundaries exact for all uint64 inputs.
 auto boundary=[&](uint64_t i){return static_cast<size_t>((static_cast<__uint128_t>(shuffled.size())*i)/count);};
 auto begin=boundary(index),end=boundary(index+1);return {shuffled.begin()+begin,shuffled.begin()+end};
}
}
