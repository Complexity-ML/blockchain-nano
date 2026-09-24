#include "nano/consensus_committee.hpp"
#include "nano/consensus_bls.hpp"
#include <set>
namespace nano {
ConsensusCommittee::ConsensusCommittee(std::span<const ConsensusCommitteeMember> members,
 const ConsensusBytes4& deposit_version,const ConsensusRoot& duty_domain):domain_(duty_domain){
 if(members.empty())throw std::invalid_argument("empty consensus committee");
 std::set<std::array<uint8_t,48>> keys;
 for(const auto& member:members){
  if(member.effective_weight==0)throw std::invalid_argument("zero committee weight");
  if(members_.contains(member.validator_index))throw std::invalid_argument("duplicate committee index");
  if(!keys.insert(member.possession_message.public_key).second)throw std::invalid_argument("duplicate committee key");
  if(!verify_consensus_deposit_signature(member.possession_message,member.possession_signature,deposit_version))
   throw std::invalid_argument("invalid committee possession proof");
  const auto& key=member.possession_message.public_key;
  members_.emplace(member.validator_index,Member{Bytes(key.begin(),key.end()),member.effective_weight});
  total_weight_+=Int(std::to_string(member.effective_weight));
 }
}
std::optional<ConsensusParticipation> ConsensusCommittee::verify(std::span<const uint64_t> indices,
 const ConsensusRoot& object_root,std::span<const uint8_t> signature)const{
 if(indices.empty()||indices.size()>members_.size()||signature.size()!=96)return std::nullopt;
 std::vector<Bytes> keys;keys.reserve(indices.size());Int signed_weight=0;
 for(size_t i=0;i<indices.size();++i){
  if(i&&indices[i]<=indices[i-1])return std::nullopt;
  auto found=members_.find(indices[i]);if(found==members_.end())return std::nullopt;
  keys.push_back(found->second.public_key);signed_weight+=Int(std::to_string(found->second.weight));
 }
 auto root=consensus_signing_root(object_root,domain_);
 if(!verify_consensus_bls_fast_aggregate(keys,root,signature))return std::nullopt;
 return ConsensusParticipation{signed_weight,total_weight_,indices.size()};
}
}
