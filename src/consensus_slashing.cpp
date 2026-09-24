#include "nano/consensus_slashing.hpp"
namespace nano {
std::optional<std::vector<uint64_t>> verify_attester_slashing(
 const IndexedConsensusAttestation& first,const ConsensusCommittee& first_context,
 const IndexedConsensusAttestation& second,const ConsensusCommittee& second_context){
 if(!is_slashable_attestation_data(first.data,second.data))return std::nullopt;
 if(!first_context.verify(first.indices,consensus_attestation_root(first.data),first.signature)||
    !second_context.verify(second.indices,consensus_attestation_root(second.data),second.signature))return std::nullopt;
 std::vector<uint64_t> common;size_t i=0,j=0;
 while(i<first.indices.size()&&j<second.indices.size()){
  if(first.indices[i]<second.indices[j])++i;
  else if(second.indices[j]<first.indices[i])++j;
  else{
   if(!first_context.same_validator_key(first.indices[i],second_context))return std::nullopt;
   common.push_back(first.indices[i]);++i;++j;
  }
 }
 if(common.empty())return std::nullopt;
 return common;
}
}
