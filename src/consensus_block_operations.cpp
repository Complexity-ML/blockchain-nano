#include "nano/consensus_block_operations.hpp"
#include "consensus_registry_checkpoint.hpp"
#include <algorithm>
namespace nano {
namespace {
uint64_t little(std::span<const uint8_t> b,size_t at,size_t width){uint64_t value=0;for(size_t i=0;i<width;++i)value|=uint64_t(b[at+i])<<(8*i);return value;}
SignedConsensusBeaconHeader signed_header(std::span<const uint8_t> b){
 SignedConsensusBeaconHeader out;out.message.slot=little(b,0,8);out.message.proposer_index=little(b,8,8);
 std::copy_n(b.begin()+16,32,out.message.parent_root.begin());std::copy_n(b.begin()+48,32,out.message.state_root.begin());std::copy_n(b.begin()+80,32,out.message.body_root.begin());out.signature=Bytes(b.begin()+112,b.end());return out;
}
}
std::optional<Phase0ProposerOperationsCandidate> prepare_phase0_proposer_operations(
 std::span<const uint8_t> bytes,const Phase0BlockContext& context,
 const Phase0BodyLimits& limits,const ConsensusRandaoRules& randao_rules,
 const Phase0SlashingRules& slashing_rules,const Phase0StateFork& fork,
 const ConsensusSlashingState& registry,const ConsensusRandaoState& randao,
 const std::function<void()>& checkpoint){
 if(!randao_rules.slots_per_epoch||fork.epoch>registry.epoch||
    context.fork_version!=(context.slot/randao_rules.slots_per_epoch<fork.epoch?fork.previous_version:fork.current_version))
  throw std::invalid_argument("inconsistent Phase0 block/evidence fork context");
 auto authenticated=authenticate_phase0_block(bytes,context,limits,randao_rules,registry,randao,checkpoint);
 if(!authenticated)return std::nullopt;
 auto next=consensus_registry_detail::copy(registry,checkpoint);
 // The authentication call has checked all canonical body offsets and list
 // lengths, so these exact slices are safe for immutable input snapshots.
 const auto body=bytes.subspan(184);
 std::array<size_t,6> offsets{};for(size_t i=0;i<5;++i)offsets[i]=little(body,200+4*i,4);offsets[5]=body.size();
 const size_t count=(offsets[1]-offsets[0])/416;
 for(size_t i=0;i<count;++i){
  if(checkpoint)checkpoint();
  const auto proof=body.subspan(offsets[0]+416*i,416);
  const auto first=signed_header(proof.first(208)),second=signed_header(proof.subspan(208));
  const auto version=first.message.slot/randao_rules.slots_per_epoch<fork.epoch?fork.previous_version:fork.current_version;
  if(!process_phase0_proposer_slashing(next,slashing_rules,first,second,version,context.genesis_root,authenticated->header.proposer_index,authenticated->header.proposer_index,checkpoint))return std::nullopt;
  if(checkpoint)checkpoint();
 }
 std::array<size_t,4> pending{};
 for(size_t i=1;i<5;++i){const auto part=body.subspan(offsets[i],offsets[i+1]-offsets[i]);pending[i-1]=i<3?(part.empty()?0:little(part,0,4)/4):part.size()/(i==3?1240:112);}
 if(checkpoint)checkpoint();
 return Phase0ProposerOperationsCandidate{std::move(*authenticated),std::move(next),count,pending};
}
}
