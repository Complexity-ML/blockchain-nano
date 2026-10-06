#include "nano/consensus_deposit_transition.hpp"
#include <algorithm>
#include <set>
#include <type_traits>
namespace nano {
namespace {
ConsensusRoot scalar(uint64_t value){ConsensusRoot r{};for(unsigned i=0;i<8;++i)r[i]=uint8_t(value>>(8*i));return r;}
void validate(const Phase0DepositState& s,const Phase0DepositRules& r){
 if(!r.effective_balance_increment||!r.maximum_effective_balance||r.maximum_effective_balance%r.effective_balance_increment||!r.validator_registry_limit||s.registry.validators.size()>r.validator_registry_limit||s.deposit_count>(uint64_t(1)<<32)||s.next_deposit_index>s.deposit_count)
  throw std::invalid_argument("invalid Phase0 deposit state/configuration");
}
void validate_registry(const Phase0DepositState& s){
 uint64_t expected=0;std::set<std::array<uint8_t,48>> keys;
 for(const auto& [index,v]:s.registry.validators){if(index!=expected++||!keys.insert(v.public_key).second||v.activation_epoch>v.exit_epoch||v.exit_epoch>v.withdrawable_epoch)throw std::invalid_argument("invalid deposit registry");}
}
Json rule_json(const Phase0DepositRules& r){return Json::array({quantity(Int(std::to_string(r.effective_balance_increment))),quantity(Int(std::to_string(r.maximum_effective_balance))),quantity(Int(std::to_string(r.validator_registry_limit))),hex(Bytes(r.genesis_fork_version.begin(),r.genesis_fork_version.end()))});}
uint64_t number(const Json& j){if(!j.is_string())throw std::invalid_argument("deposit snapshot quantity type");const auto value=quantity(j);const auto n=u64(value);if(quantity(value)!=j.get<std::string>())throw std::invalid_argument("noncanonical deposit snapshot quantity");return n;}
}
ConsensusRoot consensus_deposit_data_root(const Phase0Deposit& d){
 ConsensusRoot first{},second{},a{},b{},c{},zero{};
 std::copy_n(d.message.public_key.begin(),32,first.begin());std::copy(d.message.public_key.begin()+32,d.message.public_key.end(),second.begin());
 std::copy_n(d.signature.begin(),32,a.begin());std::copy_n(d.signature.begin()+32,32,b.begin());std::copy_n(d.signature.begin()+64,32,c.begin());
 const auto signature=consensus_signing_root(consensus_signing_root(a,b),consensus_signing_root(c,zero));
 return consensus_signing_root(consensus_signing_root(consensus_signing_root(first,second),d.message.withdrawal_credentials),consensus_signing_root(scalar(d.message.amount_gwei),signature));
}
std::optional<Phase0DepositOutcome> process_phase0_deposit(Phase0DepositState& state,const Phase0DepositRules& rules,const Phase0Deposit& deposit){
 validate(state,rules);if(state.next_deposit_index>=state.deposit_count)return std::nullopt;
 auto root=consensus_deposit_data_root(deposit);
 for(unsigned depth=0;depth<33;++depth)root=(state.next_deposit_index>>depth)&1?consensus_signing_root(deposit.proof[depth],root):consensus_signing_root(root,deposit.proof[depth]);
 if(root!=state.deposit_root)return std::nullopt;
 // Reject unproven packets before any O(N) registry validation or copy.
 validate_registry(state);
 auto found=std::find_if(state.registry.validators.begin(),state.registry.validators.end(),[&](const auto& row){return row.second.public_key==deposit.message.public_key;});
 auto next=state;++next.next_deposit_index;Phase0DepositOutcome outcome;
 if(found!=state.registry.validators.end()){
  auto& balance=next.registry.validators.at(found->first).balance;
  if(deposit.message.amount_gwei>UINT64_MAX-balance)throw std::overflow_error("deposit balance overflow");
  balance+=deposit.message.amount_gwei;outcome=Phase0DepositOutcome::ToppedUp;
 }else if(!verify_consensus_deposit_signature(deposit.message,deposit.signature,rules.genesis_fork_version)){
  outcome=Phase0DepositOutcome::IgnoredInvalidSignature;
 }else{
  if(next.registry.validators.size()>=rules.validator_registry_limit)throw std::overflow_error("configured validator registry limit reached");
  ConsensusSlashingValidator v;v.public_key=deposit.message.public_key;v.withdrawal_credentials=deposit.message.withdrawal_credentials;v.balance=deposit.message.amount_gwei;
  v.effective_balance=std::min(v.balance-v.balance%rules.effective_balance_increment,rules.maximum_effective_balance);
  next.registry.validators.emplace(next.registry.validators.size(),v);outcome=Phase0DepositOutcome::Admitted;
 }
 static_assert(std::is_nothrow_swappable_v<Phase0DepositState>);using std::swap;swap(state,next);return outcome;
}
Json phase0_deposit_snapshot(const Phase0DepositState& state,const Phase0DepositRules& rules,const Phase0SlashingRules& registry_rules){
 validate(state,rules);validate_registry(state);return {{"format","nano-phase0-deposit-v1"},{"rules",rule_json(rules)},{"depositRoot",hex(Bytes(state.deposit_root.begin(),state.deposit_root.end()))},{"depositCount",quantity(Int(std::to_string(state.deposit_count)))},{"nextDepositIndex",quantity(Int(std::to_string(state.next_deposit_index)))},{"registry",phase0_slashing_snapshot(state.registry,registry_rules)}};
}
Phase0DepositState phase0_deposit_from_snapshot(const Json& j,const Phase0DepositRules& rules,const Phase0SlashingRules& registry_rules){
 if(!j.is_object()||j.size()!=6||j.at("format")!="nano-phase0-deposit-v1"||j.at("rules")!=rule_json(rules))throw std::invalid_argument("invalid deposit snapshot envelope");
 Phase0DepositState state{};state.registry=phase0_slashing_from_snapshot(j.at("registry"),registry_rules);state.deposit_count=number(j.at("depositCount"));state.next_deposit_index=number(j.at("nextDepositIndex"));
 const auto root=unhex(j.at("depositRoot"));if(root.size()!=32||hex(root)!=j.at("depositRoot").get<std::string>())throw std::invalid_argument("invalid deposit snapshot root");std::copy(root.begin(),root.end(),state.deposit_root.begin());
 if(phase0_deposit_snapshot(state,rules,registry_rules)!=j)throw std::invalid_argument("noncanonical deposit snapshot");return state;
}
}
