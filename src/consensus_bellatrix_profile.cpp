#include "nano/consensus_bellatrix_store_snapshot.hpp"
#include <limits>
namespace nano {
BellatrixForkBlockRules load_bellatrix_fork_profile(std::string_view bytes,const Bytes& digest,size_t maximum){
 if(!maximum||bytes.size()>maximum||digest.size()!=32||sha256(Bytes(bytes.begin(),bytes.end()))!=digest)
  throw std::invalid_argument("Bellatrix profile size or commitment mismatch");
 const auto j=Json::parse(bytes);
 // Canonical equality also rejects duplicate object keys and alternate encodings.
 if(!j.is_array()||(j.size()!=12&&j.size()!=13)||j.dump()!=bytes)throw std::invalid_argument("Bellatrix profile must be canonical");
 auto number=[&](size_t i){const auto& value=j.at(i);if(!value.is_string())throw std::invalid_argument("Bellatrix profile quantity required");const auto n=quantity(value);if(quantity(n)!=value.get<std::string>())throw std::invalid_argument("Bellatrix profile noncanonical quantity");return u64(n);};
 BellatrixForkBlockRules r{};
 r.transition.consensus=altair_transition_from_profile(j.at(0));
 r.slot_duration_ms=number(1);r.attestation_due_ms=number(2);r.proposer_score_boost=number(3);
 const auto boost=number(4),replay=number(6);
 if(boost>static_cast<uint64_t>(BellatrixProposerBoostProfile::ShufflingDependent)||replay>static_cast<uint64_t>(ReplayPolicy::AllowLegacyUnprotected))throw std::invalid_argument("Unknown Bellatrix profile enum");
 r.boost_profile=static_cast<BellatrixProposerBoostProfile>(boost);
 r.transition.chain_id=quantity(j.at(5));
 if(!j.at(5).is_string()||quantity(r.transition.chain_id)!=j.at(5).get<std::string>()||r.transition.chain_id<=0||r.transition.chain_id>=(Int(1)<<256))throw std::invalid_argument("Bellatrix chain ID range");
 r.transition.replay_policy=static_cast<ReplayPolicy>(replay);r.transition.seconds_per_slot=number(7);
 auto size=[&](size_t i){const auto n=number(i);if(n>std::numeric_limits<size_t>::max())throw std::invalid_argument("Bellatrix profile size range");return size_t(n);};
 r.transition.maximum_state_bytes=size(8);
 auto width=[&](size_t i){auto n=number(i);if(n>UINT32_MAX)throw std::invalid_argument("Bellatrix SSZ limit exceeds uint32");return uint32_t(n);};
 r.transition.execution.extra_data_bytes=width(9);r.transition.execution.transaction_bytes=width(10);r.transition.execution.transactions=width(11);
 if(!r.transition.seconds_per_slot||r.transition.seconds_per_slot>UINT64_MAX/1000||r.slot_duration_ms!=r.transition.seconds_per_slot*1000||r.attestation_due_ms>r.slot_duration_ms||!r.transition.maximum_state_bytes||!r.transition.execution.transaction_bytes||!r.transition.execution.transactions)
  throw std::invalid_argument("Bellatrix profile clock or execution bounds");
 if(j.size()==13){if(!j[12].is_string())throw std::invalid_argument("Bellatrix deposit profile code hash");r.deposit_contract_code_hash=unhex(j[12].get<std::string>());if(r.deposit_contract_code_hash.size()!=32||hex(r.deposit_contract_code_hash)!=j[12].get<std::string>())throw std::invalid_argument("Bellatrix deposit profile code hash");}
 if(bellatrix_fork_profile(r)!=j)throw std::invalid_argument("Bellatrix profile round trip mismatch");
 return r;
}
}
