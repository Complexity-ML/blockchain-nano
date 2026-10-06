#include "nano/consensus_attestation_inclusion.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/committee.hpp"
namespace nano {
bool process_phase0_attestation(Phase0FfgState& state,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const Phase0AttestationInclusionRules& rules,const Phase0StateFork& fork,
 const ConsensusRoot& genesis_root,const Phase0SignedAttestation& attestation,
 const std::function<void()>& checkpoint){
 const auto& cfg=rules.duties;
 if(!cfg.slots_per_epoch||!cfg.maximum_effective_balance||!rules.minimum_inclusion_delay||
    rules.minimum_inclusion_delay>cfg.slots_per_epoch||!rules.maximum_pending_attestations||
    state.slot/cfg.slots_per_epoch!=registry.epoch||randao.epoch!=registry.epoch||fork.epoch>registry.epoch)
  throw std::invalid_argument("invalid Phase0 attestation inclusion context");
 if(checkpoint)checkpoint();
 const auto& data=attestation.data;const auto current=registry.epoch,previous=current?current-1:0;
 if((data.target.epoch!=current&&data.target.epoch!=previous)||data.slot/cfg.slots_per_epoch!=data.target.epoch||data.slot>state.slot)return false;
 const auto delay=state.slot-data.slot;
 if(delay<rules.minimum_inclusion_delay||delay>cfg.slots_per_epoch)return false;
 const bool is_current=data.target.epoch==current;
 if(data.source!=(is_current?state.checkpoints.current_justified:state.checkpoints.previous_justified))return false;
 auto& pending=is_current?state.current_attestations:state.previous_attestations;
 if(pending.size()>=rules.maximum_pending_attestations)return false;
 const auto duties=phase0_attestation_duties(registry,randao,cfg,data.target.epoch,checkpoint);
 if(data.index>=duties.committees_per_slot)return false;
 const auto committee=partition_committee(duties.shuffled,(data.slot%cfg.slots_per_epoch)*duties.committees_per_slot+data.index,duties.committees_per_slot*cfg.slots_per_epoch);
 if(attestation.aggregation_bits.size()!=committee.size())return false;
 std::vector<Bytes> keys;
 for(size_t i=0;i<committee.size();++i)if(attestation.aggregation_bits[i]){const auto& key=registry.validators.at(committee[i]).public_key;keys.emplace_back(key.begin(),key.end());}
 if(keys.empty())return false;
 const auto version=data.target.epoch<fork.epoch?fork.previous_version:fork.current_version;
 const auto root=consensus_signing_root(consensus_attestation_root(data),consensus_domain({1,0,0,0},version,genesis_root));
 if(!verify_consensus_bls_fast_aggregate(keys,root,attestation.signature))return false;
 if(checkpoint)checkpoint();
 std::vector<Phase0ProposerValidator> proposers;proposers.reserve(registry.validators.size());
 for(const auto& [index,v]:registry.validators)proposers.push_back({index,v.effective_balance,v.activation_epoch,v.exit_epoch});
 const auto proposer=phase0_proposer_index(proposers,randao,{cfg.slots_per_epoch,cfg.min_seed_lookahead},state.slot,cfg.maximum_effective_balance,cfg.shuffle_rounds,checkpoint);
 Phase0PendingAttestation accepted{data,attestation.aggregation_bits,delay,proposer};
 if(checkpoint)checkpoint();
 pending.push_back(std::move(accepted));return true;
}
}
