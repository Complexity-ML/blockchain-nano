#include "nano/consensus_block_auth.hpp"
#include <algorithm>
#include <stdexcept>
namespace nano {
namespace {
uint64_t little(std::span<const uint8_t> bytes,size_t start,size_t width){uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(bytes[start+i])<<(8*i);return value;}
}
std::optional<AuthenticatedPhase0Block> authenticate_phase0_block(
 std::span<const uint8_t> bytes,const Phase0BlockContext& context,
 const Phase0BodyLimits& limits,const ConsensusRandaoRules& rules,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const std::function<void()>& checkpoint){
 if(!rules.slots_per_epoch||context.slot/rules.slots_per_epoch!=randao.epoch||registry.epoch!=randao.epoch||
    !context.maximum_effective_balance||!context.shuffle_rounds||context.shuffle_rounds>256||context.maximum_serialized_bytes<404||
    !limits.proposer_slashings||!limits.attester_slashings||!limits.attestations||!limits.deposits||!limits.voluntary_exits||!limits.validators_per_committee)
  throw std::invalid_argument("invalid authenticated Phase0 block context");
 if(checkpoint)checkpoint();
 // SignedBeaconBlock fixed section: message offset + 96-byte signature.
 // BeaconBlock fixed section: slot, proposer, parent, state, body offset.
 if(bytes.size()<404||bytes.size()>context.maximum_serialized_bytes||little(bytes,0,4)!=100||little(bytes,180,4)!=84)return std::nullopt;
 ConsensusBeaconHeader header;header.slot=little(bytes,100,8);header.proposer_index=little(bytes,108,8);
 std::copy_n(bytes.begin()+116,32,header.parent_root.begin());std::copy_n(bytes.begin()+148,32,header.state_root.begin());
 if(header.slot!=context.slot||header.parent_root!=context.parent_root||
    (randao.last_reveal_slot&&header.slot<=*randao.last_reveal_slot))return std::nullopt;
 const auto proposer=registry.validators.find(header.proposer_index);
 if(proposer==registry.validators.end()||proposer->second.slashed||proposer->second.activation_epoch>randao.epoch||randao.epoch>=proposer->second.exit_epoch)return std::nullopt;
 try{header.body_root=phase0_beacon_body_root(bytes.subspan(184),limits);}catch(const std::invalid_argument&){return std::nullopt;}
 if(checkpoint)checkpoint();
 if(!verify_consensus_beacon_header(header,context.fork_version,context.genesis_root,proposer->second.public_key,bytes.subspan(4,96)))return std::nullopt;
 // Mutate only a private candidate; rejection or cancellation never leaks a mix.
 auto candidate=randao;
 if(!process_phase0_selected_randao(candidate,registry,rules,header.slot,header.proposer_index,context.maximum_effective_balance,context.shuffle_rounds,context.fork_version,context.genesis_root,bytes.subspan(184,96),checkpoint))return std::nullopt;
 if(checkpoint)checkpoint();
 return AuthenticatedPhase0Block{header,std::move(candidate)};
}
}
