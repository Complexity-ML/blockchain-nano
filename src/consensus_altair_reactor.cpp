#include "nano/consensus_altair_reactor.hpp"
#include <type_traits>
#include "nano/consensus_message_ssz.hpp"
namespace nano {
static_assert(std::is_nothrow_move_assignable_v<std::optional<ForkCheckpointToken>>);
static_assert(std::is_nothrow_move_assignable_v<std::shared_ptr<const AltairForkStore>>);
void AltairDurableReactor::require_available()const{if(unavailable_||!state_||!token_)throw std::runtime_error("Altair reactor unavailable; reopen and resolve checkpoint required");}
void AltairDurableReactor::publish(AltairForkStore candidate,const std::function<void()>& checkpoint){
 auto next=std::make_shared<const AltairForkStore>(std::move(candidate));
 const auto encoded=altair_store_snapshot(*next,rules_,limits_,checkpoint);
 if(checkpoint)checkpoint();
 // All candidate preparation/allocation/cancellation occurs before the write.
 // After a successful write, publishing shared_ptr and vector-backed token moves
 // cannot throw. No callback is invoked between commit and publication.
 ForkCheckpointToken committed;
 try{committed=disk_.commit_fork_checkpoint(chain_id_,genesis_,token_,encoded,limits_.envelope_bytes);}
 catch(...){unavailable_=true;throw;}
 token_=std::move(committed);state_=std::move(next);
}
AltairDurableReactor::AltairDurableReactor(ChainStore& disk,Int chain,Bytes genesis,AltairForkBlockRules rules,
 AltairStoreSnapshotLimits limits,AltairForkStore initial,const std::function<void()>& checkpoint):
 disk_(disk),chain_id_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits){publish(std::move(initial),checkpoint);}
AltairDurableReactor::AltairDurableReactor(ChainStore& disk,Int chain,Bytes genesis,AltairForkBlockRules rules,
 AltairStoreSnapshotLimits limits,const ForkCheckpointToken& expected,const std::function<void()>& checkpoint):
 disk_(disk),chain_id_(std::move(chain)),genesis_(std::move(genesis)),rules_(std::move(rules)),limits_(limits){
 const auto record=disk_.fork_checkpoint(chain_id_,genesis_,limits_.envelope_bytes,expected);
 if(!record)throw std::runtime_error("missing Altair restart checkpoint");
 auto restored=altair_store_from_snapshot(record->snapshot,expected.digest,rules_,limits_,checkpoint);
 state_=std::make_shared<const AltairForkStore>(std::move(restored));token_=expected;
}
AltairPublishedStore AltairDurableReactor::current()const{std::lock_guard lock(mutex_);require_available();return {state_,*token_};}
void AltairDurableReactor::tick(uint64_t ms,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();publish(prepare_altair_store_tick(*state_,ms,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint),checkpoint);
}
bool AltairDurableReactor::block(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_altair_received_block(*state_,bytes,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!next)return false;publish(std::move(*next),checkpoint);return true;
}
bool AltairDurableReactor::vote(const Phase0SignedAttestation& vote,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_altair_store_vote(*state_,vote,rules_,limits_.replay_slots,limits_.sync_trials,checkpoint);if(!next)return false;publish(std::move(*next),checkpoint);return true;
}
bool AltairDurableReactor::slashing(const IndexedConsensusAttestation& a,const IndexedConsensusAttestation& b,const std::function<void()>& checkpoint){
 std::lock_guard lock(mutex_);require_available();auto next=prepare_altair_store_slashing(*state_,a,b,rules_,checkpoint);if(!next)return false;publish(std::move(*next),checkpoint);return true;
}
}

namespace nano {
bool AltairDurableReactor::vote_ssz(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 auto decoded=decode_consensus_attestation_ssz(bytes,rules_.transition.body.validators_per_committee,checkpoint);
 if(!decoded){std::lock_guard lock(mutex_);require_available();return false;}
 return vote(*decoded,checkpoint);
}
bool AltairDurableReactor::slashing_ssz(std::span<const uint8_t> bytes,const std::function<void()>& checkpoint){
 auto decoded=decode_consensus_attester_slashing_ssz(bytes,rules_.transition.body.validators_per_committee,checkpoint);
 if(!decoded){std::lock_guard lock(mutex_);require_available();return false;}
 return slashing(decoded->first,decoded->second,checkpoint);
}
}
