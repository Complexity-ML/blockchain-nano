#include "nano/consensus_capella_store_snapshot.hpp"
#include "nano/consensus_bellatrix_store_snapshot.hpp"
namespace nano {
CapellaForkBlockRules load_capella_fork_profile(std::string_view bytes,const Bytes& digest,size_t maximum){
 if(!maximum||bytes.size()>maximum||digest.size()!=32||sha256(Bytes(bytes.begin(),bytes.end()))!=digest)throw std::invalid_argument("Capella profile size or commitment mismatch");
 const auto j=Json::parse(bytes);
 if(!j.is_object()||j.size()!=5||j.at("format")!="nano-capella-profile-v1"||j.dump()!=bytes)throw std::invalid_argument("Capella profile must be canonical");
 const auto encoded=j.at("common").dump();const auto common=load_bellatrix_fork_profile(encoded,sha256(Bytes(encoded.begin(),encoded.end())),maximum);
 auto num=[](const Json& value){if(!value.is_string())throw std::invalid_argument("Capella profile quantity required");const auto n=quantity(value);if(n<0||n>Int("18446744073709551615")||quantity(n)!=value.get<std::string>())throw std::invalid_argument("Capella profile quantity range");return u64(n);};
 const auto& w=j.at("withdrawals");if(!w.is_array()||w.size()!=3)throw std::invalid_argument("Capella withdrawal profile");
 const auto version=unhex(j.at("genesisVersion").get<std::string>());if(version.size()!=4||hex(version)!=j.at("genesisVersion").get<std::string>())throw std::invalid_argument("Capella genesis version");ConsensusBytes4 genesis{};std::copy(version.begin(),version.end(),genesis.begin());
 const auto changes=num(j.at("maximumBlsChanges"));if(changes>UINT32_MAX)throw std::invalid_argument("Capella BLS limit width");
 const auto& t=common.transition;
 CapellaForkBlockRules out{{t.consensus,t.execution,t.maximum_state_bytes,t.seconds_per_slot,{num(w[0]),num(w[1]),num(w[2])},genesis,uint32_t(changes),t.chain_id,t.replay_policy},common.slot_duration_ms,common.attestation_due_ms,common.proposer_score_boost,static_cast<CapellaProposerBoostProfile>(common.boost_profile),common.deposit_contract_code_hash,common.native_deposit_votes};
 if(capella_fork_profile(out)!=j)throw std::invalid_argument("Capella profile round trip mismatch");return out;
}
}
