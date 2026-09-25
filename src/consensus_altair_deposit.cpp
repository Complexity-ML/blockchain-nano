#include "nano/consensus_altair_deposit.hpp"
#include <algorithm>
namespace nano {
std::optional<AltairDepositCandidate> prepare_altair_deposit(
 std::span<const uint8_t> b,const Phase0Deposit& deposit,const AltairStateLimits& l,
 const Phase0DepositRules& rules,uint64_t slots_per_epoch,const std::function<void()>& checkpoint){
 if(!slots_per_epoch||rules.validator_registry_limit!=l.validator_registry_limit)
  throw std::invalid_argument("inconsistent Altair deposit rules");
 altair_beacon_state_root(b,l,checkpoint);
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;};
 auto root=[&](size_t at){ConsensusRoot v{};std::copy_n(b.begin()+at,32,v.begin());return v;};
 const size_t historical=176+64*l.slots_per_historical_root,randao_at=historical+96,slashings_at=randao_at+32*l.epochs_per_historical_vector,previous_at=slashings_at+8*l.epochs_per_slashings_vector;
 const std::array<size_t,7> fields{historical,historical+76,historical+88,historical+92,previous_at,previous_at+4,previous_at+129};
 std::array<size_t,8> offsets{};for(size_t i=0;i<7;++i)offsets[i]=number(fields[i],4);offsets[7]=b.size();
 const auto count=(offsets[3]-offsets[2])/121;
 if((offsets[4]-offsets[3])/8!=count||offsets[5]-offsets[4]!=count||offsets[6]-offsets[5]!=count||(offsets[7]-offsets[6])/8!=count)
  throw std::invalid_argument("Altair deposit state list counts differ");
 Phase0DepositState state{};state.registry.epoch=number(40)/slots_per_epoch;state.deposit_root=root(historical+4);state.deposit_count=number(historical+36);state.next_deposit_index=number(historical+80);
 for(size_t i=0;i<l.epochs_per_slashings_vector;++i)state.registry.slashings.push_back(number(slashings_at+8*i));
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();const auto at=offsets[2]+121*i;ConsensusSlashingValidator v{};std::copy_n(b.begin()+at,48,v.public_key.begin());v.withdrawal_credentials=root(at+48);v.effective_balance=number(at+80);v.slashed=b[at+88];v.activation_eligibility_epoch=number(at+89);v.activation_epoch=number(at+97);v.exit_epoch=number(at+105);v.withdrawable_epoch=number(at+113);v.balance=number(offsets[3]+8*i);state.registry.validators.emplace(i,v);}
 if(checkpoint)checkpoint();const auto outcome=process_phase0_deposit(state,rules,deposit);if(!outcome)return std::nullopt;if(checkpoint)checkpoint();
 std::array<Bytes,7> lists;for(size_t i=0;i<7;++i)lists[i]=Bytes(b.begin()+offsets[i],b.begin()+offsets[i+1]);
 auto put=[](Bytes& bytes,size_t at,uint64_t n,size_t width=8){for(size_t j=0;j<width;++j)bytes[at+j]=uint8_t(n>>(8*j));};
 if(*outcome==Phase0DepositOutcome::Admitted){
  const auto& v=state.registry.validators.at(count);Bytes record(121,0);std::copy(v.public_key.begin(),v.public_key.end(),record.begin());std::copy(v.withdrawal_credentials.begin(),v.withdrawal_credentials.end(),record.begin()+48);put(record,80,v.effective_balance);put(record,89,v.activation_eligibility_epoch);put(record,97,v.activation_epoch);put(record,105,v.exit_epoch);put(record,113,v.withdrawable_epoch);lists[2].insert(lists[2].end(),record.begin(),record.end());lists[3].resize(lists[3].size()+8);lists[4].push_back(0);lists[5].push_back(0);lists[6].resize(lists[6].size()+8,0);
 }
 for(const auto& [index,v]:state.registry.validators){if(checkpoint)checkpoint();put(lists[3],8*index,v.balance);}
 Bytes out(b.begin(),b.begin()+offsets[0]);put(out,historical+80,state.next_deposit_index);
 for(size_t i=0;i<7;++i){if(checkpoint)checkpoint();if(out.size()>UINT32_MAX||lists[i].size()>UINT32_MAX-out.size())throw std::overflow_error("Altair deposit SSZ offset overflow");put(out,fields[i],out.size(),4);out.insert(out.end(),lists[i].begin(),lists[i].end());}
 if(checkpoint)checkpoint();return AltairDepositCandidate{std::move(out),*outcome};
}
}
