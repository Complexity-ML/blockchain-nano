#include "nano/consensus_altair_attestation.hpp"
#include "nano/committee.hpp"
#include <algorithm>
namespace nano {
std::optional<Bytes> prepare_altair_attestation(std::span<const uint8_t> b,
 const Phase0SignedAttestation& att,const AltairStateLimits& l,const AltairAttestationRules& r,
 const std::function<void()>& checkpoint){
 const auto& d=r.duties;
 if(!d.slots_per_epoch||!d.effective_balance_increment||!r.base_reward_factor||!r.proposer_weight||r.proposer_weight>=r.weight_denominator)
  throw std::invalid_argument("invalid Altair attestation reward rules");
 for(auto w:r.participation_weights)if(w>r.weight_denominator)throw std::invalid_argument("invalid participation weight");
 altair_beacon_state_root(b,l,checkpoint);
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;};
 auto root=[&](size_t at){ConsensusRoot v{};std::copy_n(b.begin()+at,32,v.begin());return v;};
 const auto slot=number(40),epoch=slot/d.slots_per_epoch;
 const size_t historical=176+64*l.slots_per_historical_root,randao_at=historical+96,previous_at=randao_at+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 const auto validators=number(historical+88,4),balances=number(historical+92,4),previous=number(previous_at,4),current=number(previous_at+4,4),scores=number(previous_at+129,4),count=(balances-validators)/121;
 if((previous-balances)/8!=count||current-previous!=count||scores-current!=count||(b.size()-scores)/8!=count)
  throw std::invalid_argument("Altair attestation state list counts differ");
 ConsensusSlashingState registry;registry.epoch=epoch;
 ConsensusRandaoState randao;randao.epoch=epoch;
 for(size_t i=0;i<l.epochs_per_historical_vector;++i)randao.mixes.push_back(root(randao_at+32*i));
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();const auto at=validators+121*i;ConsensusSlashingValidator v{};std::copy_n(b.begin()+at,48,v.public_key.begin());v.effective_balance=number(at+80);v.balance=number(balances+8*i);v.slashed=b[at+88];v.activation_epoch=number(at+97);v.exit_epoch=number(at+105);v.withdrawable_epoch=number(at+113);registry.validators.emplace(i,v);}
 Phase0FfgState admission;admission.slot=slot;
 admission.checkpoints.previous_justified={number(previous_at+9),root(previous_at+17)};
 admission.checkpoints.current_justified={number(previous_at+49),root(previous_at+57)};
 Phase0StateFork fork{};std::copy_n(b.begin()+48,4,fork.previous_version.begin());std::copy_n(b.begin()+52,4,fork.current_version.begin());fork.epoch=number(56);
 // Altair retains Phase0's inclusion window, source, committee and BLS checks.
 // This private scratch list is empty and only supplies validated inclusion
 // metadata; there is no pending-attestation list or cap in the Altair state.
 if(!process_phase0_attestation(admission,registry,randao,{d,r.minimum_inclusion_delay,1},fork,root(8),att,checkpoint))return std::nullopt;
 const auto& accepted=att.data.target.epoch==epoch?admission.current_attestations.back():admission.previous_attestations.back();
 auto cached=[&](uint64_t s){if(s>=slot||slot-s>l.slots_per_historical_root)throw std::invalid_argument("attestation outside block-root history");return root(176+32*(s%l.slots_per_historical_root));};
 const bool target=att.data.target.root==cached(att.data.target.epoch*d.slots_per_epoch);
 const bool head=target&&att.data.beacon_block_root==cached(att.data.slot);
 auto amount=[](uint64_t n){return Int(std::to_string(n));};
 Int sqrt_slots;const auto slots=amount(d.slots_per_epoch);mpz_sqrt(sqrt_slots.get_mpz_t(),slots.get_mpz_t());
 const std::array<bool,3> timely{amount(accepted.inclusion_delay)<=sqrt_slots,target,head&&accepted.inclusion_delay==r.minimum_inclusion_delay};
 Int total=0;for(const auto& [index,v]:registry.validators)if(v.activation_epoch<=epoch&&epoch<v.exit_epoch)total+=amount(v.effective_balance);
 total=std::max(total,amount(d.effective_balance_increment));Int square;mpz_sqrt(square.get_mpz_t(),total.get_mpz_t());
 const Int base_per_increment=amount(d.effective_balance_increment)*amount(r.base_reward_factor)/square;
 const auto duties=phase0_attestation_duties(registry,randao,d,att.data.target.epoch,checkpoint);
 const auto committee=partition_committee(duties.shuffled,(att.data.slot%d.slots_per_epoch)*duties.committees_per_slot+att.data.index,duties.committees_per_slot*d.slots_per_epoch);
 Bytes out(b.begin(),b.end());const auto participation=att.data.target.epoch==epoch?current:previous;Int numerator=0;
 for(size_t i=0;i<committee.size();++i){if(checkpoint)checkpoint();if(!att.aggregation_bits[i])continue;const auto index=committee[i];const Int base=amount(registry.validators.at(index).effective_balance/d.effective_balance_increment)*base_per_increment;
  for(size_t flag=0;flag<3;++flag)if(timely[flag]&&!(out[participation+index]&(1U<<flag))){out[participation+index]|=uint8_t(1U<<flag);numerator+=base*amount(r.participation_weights[flag]);}
 }
 const Int denominator=amount(r.weight_denominator-r.proposer_weight)*amount(r.weight_denominator)/amount(r.proposer_weight);
 if(denominator==0)throw std::invalid_argument("zero Altair proposer denominator");
 const auto proposer=accepted.proposer_index;const Int balance=amount(number(balances+8*proposer))+numerator/denominator;
 if(balance>amount(UINT64_MAX))throw std::overflow_error("Altair attestation reward overflow");
 const auto value=u64(balance);for(size_t j=0;j<8;++j)out[balances+8*proposer+j]=uint8_t(value>>(8*j));
 if(checkpoint)checkpoint();return out;
}
}
