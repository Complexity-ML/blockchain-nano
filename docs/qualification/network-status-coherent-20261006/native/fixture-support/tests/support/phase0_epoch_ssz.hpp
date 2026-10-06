#pragma once
#include "phase0_ffg_ssz.hpp"
#include "nano/consensus_epoch.hpp"
namespace test::phase0_minimal {
inline Phase0EpochState epoch_state(const Bytes& b){
 Phase0EpochState s;s.registry=decode(b);s.randao=ffg_randao(b);s.ffg=ffg_state(b);
 for(size_t i=0;i<64;++i)s.state_roots.push_back(root_at(b,2224+32*i));
 const auto h=little(b,4272,4),votes=little(b,4348,4),end=little(b,4360,4);
 check(h<=votes&&votes<=end&&end<=b.size()&&(votes-h)%32==0&&(end-votes)%72==0,"epoch history layout");
 for(size_t i=h;i<votes;i+=32)s.historical_roots.push_back(root_at(b,i));
 for(size_t i=votes;i<end;i+=72)s.eth1_votes.push_back({root_at(b,i),little(b,i+32,8),root_at(b,i+40)});
 return s;
}
inline Phase0EpochRules epoch_rules(const fs::path& refs){
 const auto m=load(refs/"manifest.json");for(const auto& [name,info]:m.at("referenceFiles").items())check(hex(sha256(binary(refs/name))).substr(2)==info.at("sha256").get<std::string>(),"config digest");
 const auto preset=read(refs/"minimal-phase0.yaml"),config=read(refs/"minimal-config.yaml");auto p=[&](const char* k){return parameter(preset,k);};auto c=[&](const char* k){return parameter(config,k);};
 Phase0EpochRules r;
 r.rewards={{p("SLOTS_PER_EPOCH"),p("MIN_SEED_LOOKAHEAD"),p("TARGET_COMMITTEE_SIZE"),p("MAX_COMMITTEES_PER_SLOT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),uint16_t(p("SHUFFLE_ROUND_COUNT"))},p("BASE_REWARD_FACTOR"),4,p("PROPOSER_REWARD_QUOTIENT"),p("MIN_EPOCHS_TO_INACTIVITY_PENALTY"),p("INACTIVITY_PENALTY_QUOTIENT")};
 r.registry={c("MIN_PER_EPOCH_CHURN_LIMIT"),c("CHURN_LIMIT_QUOTIENT"),p("MAX_SEED_LOOKAHEAD"),c("MIN_VALIDATOR_WITHDRAWABILITY_DELAY"),p("MAX_EFFECTIVE_BALANCE"),c("EJECTION_BALANCE")};
 r.slashing={r.registry.minimum_churn_limit,r.registry.churn_limit_quotient,r.registry.max_seed_lookahead,r.registry.minimum_withdrawability_delay,p("EPOCHS_PER_SLASHINGS_VECTOR"),p("MIN_SLASHING_PENALTY_QUOTIENT"),p("WHISTLEBLOWER_REWARD_QUOTIENT"),p("PROPOSER_REWARD_QUOTIENT"),p("EFFECTIVE_BALANCE_INCREMENT"),p("PROPORTIONAL_SLASHING_MULTIPLIER")};
 r.effective={p("EFFECTIVE_BALANCE_INCREMENT"),p("MAX_EFFECTIVE_BALANCE"),p("HYSTERESIS_QUOTIENT"),p("HYSTERESIS_DOWNWARD_MULTIPLIER"),p("HYSTERESIS_UPWARD_MULTIPLIER")};
 r.history={p("SLOTS_PER_EPOCH"),p("SLOTS_PER_HISTORICAL_ROOT"),p("HISTORICAL_ROOTS_LIMIT")};
 r.epochs_per_randao_vector=p("EPOCHS_PER_HISTORICAL_VECTOR");r.epochs_per_eth1_voting_period=p("EPOCHS_PER_ETH1_VOTING_PERIOD");r.maximum_pending_attestations=p("MAX_ATTESTATIONS")*p("SLOTS_PER_EPOCH");return r;
}
}
