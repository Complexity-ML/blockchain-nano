#pragma once
// WELL-CONSTRUCTED MEMORY DOUBLES ONLY. Not runtime integration or real storage.
#include "nano/types.hpp"
#include <array>
#include <functional>
#include <memory>
#include <set>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
namespace nano {
using ConsensusRoot=std::array<uint8_t,32>;
struct ConsensusCheckpoint {uint64_t epoch=0;ConsensusRoot root{};};
struct Phase0SignedAttestation {struct Data {uint64_t slot=0;ConsensusRoot beacon_block_root{};ConsensusCheckpoint source,target;} data;};
struct ForkCheckpointToken {uint64_t sequence=1;Bytes digest=Bytes(32,0);};
struct MockPublishedState {
 struct Timed {struct Clock {uint64_t genesis_time_ms=0,time_ms=384000;} clock;
 struct View {struct Message {uint64_t epoch=0;ConsensusRoot root{};};std::map<uint64_t,Message> latest_messages;ConsensusCheckpoint justified,finalized;std::optional<ConsensusRoot> proposer_boost;} view;} timed;
};
struct MockPublishedStore {std::shared_ptr<const MockPublishedState> state;ForkCheckpointToken token;};
class BeaconSigningJournal {
public:
 using Signer=std::function<Bytes(const ConsensusRoot&)>;
 bool is_retired()const{return false;}
 std::optional<uint64_t> latest_proposal_slot()const{return std::nullopt;}
 std::optional<uint64_t> latest_attestation_target()const{return std::nullopt;}
 ForkCheckpointToken commitment()const{return {};}
};
class StaleCapellaDuty:public std::invalid_argument {public:using std::invalid_argument::invalid_argument;};
class ExpiredCapellaDuty:public std::runtime_error {public:using std::runtime_error::runtime_error;};
class CapellaDurableReactor {
public:
 std::shared_ptr<MockPublishedState> memory=std::make_shared<MockPublishedState>();
 bool projected_available=false;uint64_t prepare_calls=0,enumerations=0,proposer_calls=0,signing_paths=0,admitted_head_generation=1;
 std::pair<uint64_t,uint64_t> local_duty_timing()const{return {6000,8};}
 bool execution_bound()const{return true;}
 uint64_t local_duty_due_ms()const{return 2000;}
 MockPublishedStore current()const{return {memory,{}};}
 bool prepare_current_duties(uint64_t,const std::function<void()>& check){++prepare_calls;if(check)check();if(!projected_available)throw std::invalid_argument("empty Altair sync active set");return true;}
 void replace_with_ready_head(){++admitted_head_generation;projected_available=true;}
 std::map<Bytes,uint64_t> current_validator_indices(const std::set<Bytes>&,const std::function<void()>&){++enumerations;return {};}
 std::optional<uint64_t> current_proposer(uint64_t,const std::function<void()>&){++proposer_calls;return std::nullopt;}
 bool current_duties_ready(uint64_t,const std::function<void()>&){return false;}
 std::set<uint64_t> current_attesters(uint64_t,const std::function<void()>&){return {};}
 std::set<uint64_t> current_sync_members(uint64_t,const std::function<void()>&){return {};}
 std::optional<Bytes> propose_and_admit_current(uint64_t,uint64_t,const std::string&,size_t,size_t,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&,const Bytes&){++signing_paths;throw std::logic_error("unexpected mock signing path");}
 std::optional<Phase0SignedAttestation> attest_current(uint64_t,uint64_t,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&,const Bytes&){++signing_paths;throw std::logic_error("unexpected mock signing path");}
 std::optional<Bytes> sync_current(uint64_t,uint64_t,const std::function<uint64_t()>&,BeaconSigningJournal&,const BeaconSigningJournal::Signer&,const std::function<void()>&,const Bytes&){++signing_paths;throw std::logic_error("unexpected mock signing path");}
 bool vote(const Phase0SignedAttestation&,const std::function<void()>&){throw std::logic_error("unexpected mock admission");}
};
class CapellaLocalClock {public:uint64_t polls=0;bool poll(const std::function<void()>& check){++polls;if(check)check();return false;}};
}
