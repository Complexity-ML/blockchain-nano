#include "nano/consensus_altair_sync_aggregate.hpp"
#include "nano/consensus_bls.hpp"
#include "nano/consensus_proposer.hpp"
#include <algorithm>
namespace nano {
namespace {
struct VerifiedSync {std::vector<bool> bits;std::vector<Bytes> committee;};
std::optional<VerifiedSync> verified_sync(std::span<const uint8_t> b,
 std::span<const uint8_t> aggregate,const AltairStateLimits& l,const AltairSyncAggregateRules& r,
 const std::function<void()>& checkpoint){
 if(!r.slots_per_epoch||!r.effective_balance_increment||!r.base_reward_factor||!r.weight_denominator||r.proposer_weight>=r.weight_denominator||!r.maximum_effective_balance||r.sync_reward_weight>r.weight_denominator||!r.shuffle_rounds||r.shuffle_rounds>256||r.min_seed_lookahead>=l.epochs_per_historical_vector)
  throw std::invalid_argument("invalid Altair sync aggregate rules");
 altair_beacon_state_root(b,l,checkpoint);
 const auto bit_bytes=(l.sync_committee_size+7)/8;
 if(aggregate.size()!=bit_bytes+96||(l.sync_committee_size%8&&(aggregate[bit_bytes-1]>>(l.sync_committee_size%8))))return std::nullopt;
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;};
 auto root=[&](size_t at){ConsensusRoot v{};std::copy_n(b.begin()+at,32,v.begin());return v;};
 const auto slot=number(40);if(!slot)throw std::invalid_argument("sync aggregate at genesis slot");
 const size_t historical=176+64*l.slots_per_historical_root,randao_at=historical+96,previous_at=randao_at+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 const auto validators=number(historical+88,4),balances=number(historical+92,4),previous=number(previous_at,4),count=(balances-validators)/121;
 if((previous-balances)/8!=count)throw std::invalid_argument("sync aggregate balance count mismatch");
 const auto committee_at=previous_at+133;
 std::vector<Bytes> selected;std::vector<bool> bits(l.sync_committee_size);std::vector<Bytes> committee;
 for(size_t i=0;i<l.sync_committee_size;++i){if(checkpoint)checkpoint();committee.emplace_back(b.begin()+committee_at+48*i,b.begin()+committee_at+48*i+48);bits[i]=(aggregate[i/8]>>(i%8))&1;if(bits[i])selected.push_back(committee.back());}
 const auto fork_epoch=number(56);ConsensusBytes4 version{};std::copy_n(b.begin()+((slot-1)/r.slots_per_epoch<fork_epoch?48:52),4,version.begin());
 const auto signing_root=consensus_signing_root(root(176+32*((slot-1)%l.slots_per_historical_root)),consensus_domain({7,0,0,0},version,root(8)));
 const auto signature=aggregate.subspan(bit_bytes);
 // eth_fast_aggregate_verify explicitly accepts infinity for zero participants.
 // For nonempty committees, retain repeated pubkeys: each position is a vote.
 // Aggregating participants directly is equivalent to the cached-key shortcut
 // only for authenticated states whose stored aggregate matches the committee.
 if(selected.empty()){if(signature[0]!=0xc0||!std::all_of(signature.begin()+1,signature.end(),[](uint8_t x){return x==0;}))return std::nullopt;}
 else if(!verify_consensus_bls_fast_aggregate(selected,signing_root,signature))return std::nullopt;
 return VerifiedSync{std::move(bits),std::move(committee)};
}
}
bool verify_altair_sync_aggregate(std::span<const uint8_t> state,std::span<const uint8_t> aggregate,
 const AltairStateLimits& limits,const AltairSyncAggregateRules& rules,const std::function<void()>& checkpoint){
 return verified_sync(state,aggregate,limits,rules,checkpoint).has_value();
}
std::optional<Bytes> prepare_altair_sync_aggregate(std::span<const uint8_t> b,
 std::span<const uint8_t> aggregate,const AltairStateLimits& l,const AltairSyncAggregateRules& r,
 const std::function<void()>& checkpoint){
 const auto verified=verified_sync(b,aggregate,l,r,checkpoint);if(!verified)return std::nullopt;
 const auto& bits=verified->bits;const auto& committee=verified->committee;
 auto number=[&](size_t at,size_t width=8){uint64_t n=0;for(size_t j=0;j<width;++j)n|=uint64_t(b[at+j])<<(8*j);return n;};
 auto root=[&](size_t at){ConsensusRoot v{};std::copy_n(b.begin()+at,32,v.begin());return v;};
 const auto slot=number(40),epoch=slot/r.slots_per_epoch;
 const size_t historical=176+64*l.slots_per_historical_root,randao_at=historical+96;
 const auto validators=number(historical+88,4),balances=number(historical+92,4),count=(balances-validators)/121;
 std::map<Bytes,uint64_t> indices;std::vector<Phase0ProposerValidator> registry;Int total=0;auto amount=[](uint64_t n){return Int(std::to_string(n));};
 for(size_t i=0;i<count;++i){if(checkpoint)checkpoint();const auto at=validators+121*i;const auto effective=number(at+80),activation=number(at+97),exit=number(at+105);registry.push_back({i,effective,activation,exit});indices.emplace(Bytes(b.begin()+at,b.begin()+at+48),i);if(activation<=epoch&&epoch<exit)total+=amount(effective);}
 total=std::max(total,amount(r.effective_balance_increment));Int square;mpz_sqrt(square.get_mpz_t(),total.get_mpz_t());
 const Int increments=total/amount(r.effective_balance_increment),base_per_increment=amount(r.effective_balance_increment)*amount(r.base_reward_factor)/square;
 const Int participant=base_per_increment*increments*amount(r.sync_reward_weight)/amount(r.weight_denominator)/amount(r.slots_per_epoch)/amount(l.sync_committee_size);
 const Int proposer_reward=participant*amount(r.proposer_weight)/amount(r.weight_denominator-r.proposer_weight);
 ConsensusRandaoState randao;randao.epoch=epoch;randao.last_reveal_slot=number(64);for(size_t i=0;i<l.epochs_per_historical_vector;++i)randao.mixes.push_back(root(randao_at+32*i));
 const auto proposer=phase0_proposer_index(registry,randao,{r.slots_per_epoch,r.min_seed_lookahead},slot,r.maximum_effective_balance,r.shuffle_rounds,checkpoint);
 auto checked=[&](const Int& n){if(n<0||n>amount(UINT64_MAX))throw std::overflow_error("sync aggregate reward overflow");return u64(n);};const auto debit=checked(participant);
 std::vector<uint64_t> live(count);for(size_t i=0;i<count;++i)live[i]=number(balances+8*i);
 // Preserve order, repeated members and proposer/member aliases. Netting deltas
 // before saturating each debit would produce a different state at low balances.
 for(size_t i=0;i<committee.size();++i){if(checkpoint)checkpoint();const auto found=indices.find(committee[i]);if(found==indices.end())throw std::invalid_argument("sync committee member absent from registry");const auto member=found->second;if(bits[i]){live[member]=checked(amount(live[member])+participant);live[proposer]=checked(amount(live[proposer])+proposer_reward);}else live[member]-=std::min(live[member],debit);}
 Bytes out(b.begin(),b.end());for(size_t i=0;i<count;++i)for(size_t j=0;j<8;++j)out[balances+8*i+j]=uint8_t(live[i]>>(8*j));if(checkpoint)checkpoint();return out;
}
}
