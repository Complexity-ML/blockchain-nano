#include "nano/consensus_registry_operations.hpp"
#include <algorithm>
namespace nano {
namespace {
uint64_t little(std::span<const uint8_t> b,size_t at,size_t width){uint64_t v=0;for(size_t i=0;i<width;++i)v|=uint64_t(b[at+i])<<(8*i);return v;}
Phase0Deposit decode_deposit(std::span<const uint8_t> b){
 Phase0Deposit d;for(size_t i=0;i<33;++i)std::copy_n(b.begin()+32*i,32,d.proof[i].begin());
 std::copy_n(b.begin()+1056,48,d.message.public_key.begin());std::copy_n(b.begin()+1104,32,d.message.withdrawal_credentials.begin());d.message.amount_gwei=little(b,1136,8);std::copy_n(b.begin()+1144,96,d.signature.begin());return d;
}
}
Phase0RegistryOperationsResult prepare_phase0_registry_operations(
 std::span<const uint8_t> bytes,const Phase0BlockContext& context,
 const Phase0BodyLimits& limits,const ConsensusRandaoRules& randao_rules,
 const Phase0SlashingRules& slashing_rules,const Phase0DepositRules& deposit_rules,
 const Phase0ExitRules& exit_rules,const Phase0StateFork& fork,
 const Phase0DepositState& state,const ConsensusRandaoState& randao,
 const std::function<void()>& checkpoint){
 if(!exit_rules.minimum_churn_limit||!exit_rules.churn_limit_quotient||!exit_rules.shard_committee_period||
    !deposit_rules.maximum_effective_balance||exit_rules.minimum_churn_limit!=slashing_rules.minimum_churn_limit||
    exit_rules.churn_limit_quotient!=slashing_rules.churn_limit_quotient||
    exit_rules.max_seed_lookahead!=slashing_rules.max_seed_lookahead||
    exit_rules.minimum_withdrawability_delay!=slashing_rules.minimum_withdrawability_delay||
    deposit_rules.effective_balance_increment!=slashing_rules.effective_balance_increment||
    deposit_rules.maximum_effective_balance!=context.maximum_effective_balance||
    !deposit_rules.effective_balance_increment||!deposit_rules.validator_registry_limit||
    deposit_rules.maximum_effective_balance%deposit_rules.effective_balance_increment||
    state.registry.validators.size()>deposit_rules.validator_registry_limit||
    state.deposit_count>(uint64_t(1)<<32)||state.next_deposit_index>state.deposit_count)
  throw std::invalid_argument("inconsistent Phase0 registry block rules/state");
 auto prepared=prepare_phase0_proposer_operations(bytes,context,limits,randao_rules,slashing_rules,fork,state.registry,randao,checkpoint);
 if(!prepared)return InvalidPhase0RegistryBlock{};
 const auto& pending=prepared->unprocessed_operations;
 if(pending[0]||pending[1])return UnsupportedPhase0RegistryOperations{pending[0],pending[1]};
 if(pending[2]!=std::min<uint64_t>(limits.deposits,state.deposit_count-state.next_deposit_index))return InvalidPhase0RegistryBlock{};
 Phase0DepositState next{std::move(prepared->registry_after_proposer_slashings),state.deposit_root,state.deposit_count,state.next_deposit_index};
 // Offsets/list sizes were validated while authenticating these immutable bytes.
 const auto body=bytes.subspan(184);const size_t deposits_begin=little(body,212,4),exits_begin=little(body,216,4);
 for(size_t i=0;i<pending[2];++i){
  if(checkpoint)checkpoint();
  if(!process_phase0_deposit(next,deposit_rules,decode_deposit(body.subspan(deposits_begin+1240*i,1240))))return InvalidPhase0RegistryBlock{};
  if(checkpoint)checkpoint();
 }
 for(size_t i=0;i<pending[3];++i){
  if(checkpoint)checkpoint();
  const auto b=body.subspan(exits_begin+112*i,112);
  SignedConsensusVoluntaryExit exit{little(b,0,8),little(b,8,8),Bytes(b.begin()+16,b.end())};
  const auto version=exit.epoch<fork.epoch?fork.previous_version:fork.current_version;
  if(!process_phase0_voluntary_exit(next.registry,exit_rules,exit,version,context.genesis_root))return InvalidPhase0RegistryBlock{};
  if(checkpoint)checkpoint();
 }
 if(checkpoint)checkpoint();
 return Phase0RegistryOperationsCandidate{std::move(prepared->authenticated),std::move(next),prepared->proposer_slashings_processed,pending[2],pending[3]};
}
}
