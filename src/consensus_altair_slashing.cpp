#include "nano/consensus_altair_slashing.hpp"
#include "nano/consensus_proposer.hpp"
#include <algorithm>
namespace nano {
namespace {
struct Context {
 std::span<const uint8_t> bytes;
 const AltairSlashingRules& rules;
 std::function<void()> checkpoint;
 size_t validators_at,balances_at,slashings_at;
 ConsensusSlashingState registry;
 ConsensusRandaoState randao;
 uint64_t number(size_t at,size_t width=8)const{uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(bytes[at+j])<<(8*j);return n;}
 ConsensusRoot root(size_t at)const{ConsensusRoot r{};std::copy_n(bytes.begin()+at,32,r.begin());return r;}
 ConsensusBytes4 version(uint64_t epoch)const{ConsensusBytes4 v{};std::copy_n(bytes.begin()+(epoch<number(56)?48:52),4,v.begin());return v;}
 Context(std::span<const uint8_t> b,const AltairStateLimits& l,const AltairSlashingRules& r,const std::function<void()>& cp):bytes(b),rules(r),checkpoint(cp){
  if(!r.duties.slots_per_epoch||r.economics.epochs_per_slashings_vector!=l.epochs_per_slashings_vector||r.economics.effective_balance_increment!=r.duties.effective_balance_increment)
   throw std::invalid_argument("inconsistent Altair slashing rules");
  altair_beacon_state_root(b,l,cp);
  const size_t historical=176+64*l.slots_per_historical_root,randao_at=historical+96;slashings_at=randao_at+32*l.epochs_per_historical_vector;const auto previous_at=slashings_at+8*l.epochs_per_slashings_vector;
  validators_at=number(historical+88,4);balances_at=number(historical+92,4);const auto previous=number(previous_at,4),current=number(previous_at+4,4),scores=number(previous_at+129,4);const auto count=(balances_at-validators_at)/121;
  if((previous-balances_at)/8!=count||current-previous!=count||scores-current!=count||(b.size()-scores)/8!=count)throw std::invalid_argument("Altair slashing state list counts differ");
  registry.epoch=number(40)/r.duties.slots_per_epoch;randao.epoch=registry.epoch;
  if(number(56)>registry.epoch)throw std::invalid_argument("future Altair state fork");
  for(size_t i=0;i<l.epochs_per_historical_vector;++i)randao.mixes.push_back(root(randao_at+32*i));
  for(size_t i=0;i<l.epochs_per_slashings_vector;++i)registry.slashings.push_back(number(slashings_at+8*i));
  for(size_t i=0;i<count;++i){if(cp)cp();const auto at=validators_at+121*i;ConsensusSlashingValidator v{};std::copy_n(b.begin()+at,48,v.public_key.begin());v.withdrawal_credentials=root(at+48);v.effective_balance=number(at+80);v.slashed=b[at+88];v.activation_eligibility_epoch=number(at+89);v.activation_epoch=number(at+97);v.exit_epoch=number(at+105);v.withdrawable_epoch=number(at+113);v.balance=number(balances_at+8*i);registry.validators.emplace(i,v);}
 }
 uint64_t proposer()const{
  std::vector<Phase0ProposerValidator> candidates;for(const auto& [i,v]:registry.validators){if(checkpoint)checkpoint();candidates.push_back({i,v.effective_balance,v.activation_epoch,v.exit_epoch});}const auto& d=rules.duties;
  return phase0_proposer_index(candidates,randao,{d.slots_per_epoch,d.min_seed_lookahead},number(40),d.maximum_effective_balance,d.shuffle_rounds,checkpoint);
 }
 AltairSlashingCandidate result(std::vector<uint64_t> indices)const{
  Bytes out(bytes.begin(),bytes.end());auto put=[&](size_t at,uint64_t n){for(size_t j=0;j<8;++j)out[at+j]=uint8_t(n>>(8*j));};
  for(const auto& [i,v]:registry.validators){if(checkpoint)checkpoint();const auto at=validators_at+121*i;out[at+88]=v.slashed;put(at+105,v.exit_epoch);put(at+113,v.withdrawable_epoch);put(balances_at+8*i,v.balance);}
  for(size_t i=0;i<registry.slashings.size();++i){if(checkpoint)checkpoint();put(slashings_at+8*i,registry.slashings[i]);}if(checkpoint)checkpoint();return {std::move(out),std::move(indices)};
 }
};
}
std::optional<AltairSlashingCandidate> prepare_altair_proposer_slashing(
 std::span<const uint8_t> b,const SignedConsensusBeaconHeader& first,const SignedConsensusBeaconHeader& second,
 const AltairStateLimits& l,const AltairSlashingRules& r,const std::function<void()>& checkpoint){
 Context c(b,l,r,checkpoint);const auto proposer=c.proposer();if(checkpoint)checkpoint();
 auto indices=process_phase0_proposer_slashing(c.registry,r.economics,first,second,c.version(first.message.slot/r.duties.slots_per_epoch),c.root(8),proposer,proposer,checkpoint);
 if(!indices)return std::nullopt;return c.result(std::move(*indices));
}
std::optional<AltairSlashingCandidate> prepare_altair_attester_slashing(
 std::span<const uint8_t> b,const IndexedConsensusAttestation& first,const IndexedConsensusAttestation& second,
 const AltairStateLimits& l,const AltairSlashingRules& r,const std::function<void()>& checkpoint){
 Context c(b,l,r,checkpoint);const auto proposer=c.proposer();
 auto indices=process_phase0_registry_attester_slashing(c.registry,r.economics,first,second,r.maximum_attesting_indices,c.version(first.data.target.epoch),c.version(second.data.target.epoch),c.root(8),proposer,proposer,checkpoint);
 if(!indices)return std::nullopt;return c.result(std::move(*indices));
}
}
