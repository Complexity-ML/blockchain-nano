#include "nano/consensus_signing.hpp"
#include <algorithm>
#include <openssl/sha.h>
namespace nano {
ConsensusRoot consensus_signing_root(const ConsensusRoot& object,const ConsensusRoot& domain){
 std::array<uint8_t,64> input{};
 std::copy(object.begin(),object.end(),input.begin());
 std::copy(domain.begin(),domain.end(),input.begin()+32);
 ConsensusRoot result{};SHA256(input.data(),input.size(),result.data());return result;
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
