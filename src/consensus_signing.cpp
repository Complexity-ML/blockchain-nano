#include "nano/consensus_signing.hpp"
#include <algorithm>
#include <openssl/evp.h>
#include <memory>
#include <stdexcept>
namespace nano {
ConsensusRoot consensus_signing_root(const ConsensusRoot& object,const ConsensusRoot& domain){
 std::array<uint8_t,64> input{};
 std::copy(object.begin(),object.end(),input.begin());
 std::copy(domain.begin(),domain.end(),input.begin()+32);
 // Fetch the immutable SHA-256 implementation once; SSZ Merkle nodes must
 // not repeat provider selection and its shared locks for every 64-byte pair.
 // Contexts remain private to EVP_Digest, so concurrent callers share no state.
 static const auto algorithm=[]{
  std::unique_ptr<EVP_MD,decltype(&EVP_MD_free)> md(EVP_MD_fetch(nullptr,"SHA256",nullptr),EVP_MD_free);
  if(!md)throw std::runtime_error("Consensus SHA256 provider unavailable");
  return md;
 }();
 ConsensusRoot result{};unsigned length=0;
 if(EVP_Digest(input.data(),input.size(),result.data(),&length,algorithm.get(),nullptr)!=1||length!=result.size())
  throw std::runtime_error("Consensus SHA256 failed");
 return result;
}
ConsensusRoot consensus_fork_data_root(const ConsensusBytes4& version,const ConsensusRoot& genesis){
 ConsensusRoot padded{};std::copy(version.begin(),version.end(),padded.begin());
 return consensus_signing_root(padded,genesis);
}
ConsensusRoot consensus_domain(const ConsensusBytes4& type,const ConsensusBytes4& version,const ConsensusRoot& genesis){
 auto root=consensus_fork_data_root(version,genesis);ConsensusRoot result{};
 std::copy(type.begin(),type.end(),result.begin());std::copy_n(root.begin(),28,result.begin()+4);return result;
}
}
