#pragma once
#include "nano/consensus_slashing.hpp"
#include "nano/types.hpp"

namespace nano::detail {
// Projection of a validator already authenticated in the selected Beacon state.
// Epochs and balances retain their full uint64 range as decimal JSON strings.
inline Json beacon_validator_json(uint64_t index,const ConsensusSlashingValidator& v,uint64_t epoch){
 const char* status;
 if(epoch<v.activation_epoch)status=v.activation_eligibility_epoch==UINT64_MAX?"pending_initialized":"pending_queued";
 else if(epoch<v.exit_epoch)status=v.exit_epoch==UINT64_MAX?"active_ongoing":v.slashed?"active_slashed":"active_exiting";
 else if(epoch<v.withdrawable_epoch)status=v.slashed?"exited_slashed":"exited_unslashed";
 else status=v.balance?"withdrawal_possible":"withdrawal_done";
 return {{"index",std::to_string(index)},{"balance",std::to_string(v.balance)},{"status",status},
  {"validator",{{"pubkey",hex(Bytes(v.public_key.begin(),v.public_key.end()))},
   {"withdrawal_credentials",hex(Bytes(v.withdrawal_credentials.begin(),v.withdrawal_credentials.end()))},
   {"effective_balance",std::to_string(v.effective_balance)},{"slashed",v.slashed},
   {"activation_eligibility_epoch",std::to_string(v.activation_eligibility_epoch)},
   {"activation_epoch",std::to_string(v.activation_epoch)},{"exit_epoch",std::to_string(v.exit_epoch)},
   {"withdrawable_epoch",std::to_string(v.withdrawable_epoch)}}}};
}
}
