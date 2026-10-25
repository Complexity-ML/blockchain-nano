#include "support/process.hpp"
#include "nano/consensus_exit.hpp"
#include "nano/consensus_registry_epoch.hpp"
#include "nano/evm.hpp"
#include "blst.h"
#include <exception>
using namespace test;
namespace {
constexpr size_t count=8192;
constexpr uint64_t balance=32000;
const ConsensusBytes4 version{1,0,0,0};
const ConsensusRoot genesis=[] {ConsensusRoot r{};r[0]=7;return r;}();
const Phase0SlashingRules slashing{1,65536,1,2,64,32,512,8,1000,1};
const Phase0ExitRules exits{1,65536,1,2,2};
const Phase0RegistryUpdateRules updates{1,65536,1,2,balance,16000};
struct Key {
 blst_scalar secret{};std::array<uint8_t,48> public_key{};
 explicit Key(uint8_t index){std::array<uint8_t,32> seed{};seed[0]=index;blst_keygen(&secret,seed.data(),seed.size(),nullptr,0);blst_p1 p;blst_sk_to_pk_in_g1(&p,&secret);blst_p1_compress(public_key.data(),&p);}
 Bytes sign(const ConsensusRoot& root)const{
  static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
  blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes out(96);blst_p2_compress(out.data(),&s);return out;
 }
};
Bytes aggregate(const Bytes& a,const Bytes& b){blst_p2_affine x,y;check(blst_p2_uncompress(&x,a.data())==BLST_SUCCESS&&blst_p2_uncompress(&y,b.data())==BLST_SUCCESS,"fixture signature decode");blst_p2 sum;blst_p2_from_affine(&sum,&x);blst_p2_add_or_double_affine(&sum,&sum,&y);Bytes out(96);blst_p2_compress(out.data(),&sum);return out;}
ConsensusSlashingState registry(const std::array<Key,3>& keys){
 ConsensusSlashingState state;state.epoch=10;state.slashings.resize(slashing.epochs_per_slashings_vector);
 for(size_t i=0;i<count;++i){ConsensusSlashingValidator v;v.balance=v.effective_balance=balance;v.activation_epoch=v.activation_eligibility_epoch=0;
  // Only the first three fixed, public fixture keys sign anything. The remaining
  // distinct opaque rows exercise component traversal, not network admission/PoP.
  if(i<keys.size())v.public_key=keys[i].public_key;else{v.public_key[0]=0x55;for(size_t j=0;j<8;++j)v.public_key[40+j]=uint8_t(i>>(8*j));}
  state.validators.emplace(i,v);
 }return state;
}
template<class Call> void cancelled(const ConsensusSlashingState& input,const Call& call,size_t stop,size_t& cases){
 auto state=input;const auto before=state;size_t visited=0;bool caught=false;const auto sentinel=std::make_exception_ptr(ExecutionCancelled("public fixture cancellation"));
 try{(void)call(state,[&]{if(++visited==stop){check(state==before,"caller state mutated before cancellation");std::rethrow_exception(sentinel);}});}
 catch(...){check(std::current_exception()==sentinel,"cancellation converted into another exception");caught=true;}
 check(caught&&visited==stop&&state==before,"cancellation absent, continued, or published partial state");++cases;
}
template<class Call> size_t checked_call(ConsensusSlashingState& state,const Call& call){size_t checks=0;(void)call(state,[&]{++checks;});check(checks>2*count,"large registry/copy did not expose cooperative progress");return checks;}
}
int main(){try{
 const std::array<Key,3> keys{Key(1),Key(2),Key(3)};const auto initial=registry(keys);size_t cases=0;std::vector<std::string> groups;
 // Link the original overloads explicitly: these symbols must remain available
 // to callers/objects compiled before the cooperative overloads were added.
 using OldExit=bool(*)(ConsensusSlashingState&,const Phase0ExitRules&,const SignedConsensusVoluntaryExit&,const ConsensusBytes4&,const ConsensusRoot&);
 using OldProposer=std::optional<std::vector<uint64_t>>(*)(ConsensusSlashingState&,const Phase0SlashingRules&,const SignedConsensusBeaconHeader&,const SignedConsensusBeaconHeader&,const ConsensusBytes4&,const ConsensusRoot&,uint64_t,uint64_t);
 check(static_cast<OldExit>(&process_phase0_voluntary_exit)&&static_cast<OldProposer>(&process_phase0_proposer_slashing),"legacy symbols absent");

 // A late duplicate is intentionally invalid: cancellation must happen DURING
 // inspection, before reaching it. The old registry-update code instead throws
 // invalid_argument before its next checkpoint; a count-only test would miss it.
 auto invalid=initial;invalid.validators.at(count-1).public_key=invalid.validators.at(0).public_key;
 const auto update=[&](auto& state,const auto& cp){return prepare_phase0_registry_updates(state,updates,state.epoch,cp);};
 cancelled(invalid,update,64,cases);bool rejected=false;try{(void)prepare_phase0_registry_updates(invalid,updates,invalid.epoch);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"late duplicate accepted without cancellation");++cases;groups.push_back("registry-inspection-before-late-invalid-row");

 auto state=initial;const auto legacy_update=prepare_phase0_registry_updates(state,updates,state.epoch);size_t update_checks=0;const auto candidate=prepare_phase0_registry_updates(state,updates,state.epoch,[&]{++update_checks;});
 check(state==initial&&candidate.registry_after==legacy_update.registry_after&&candidate.activated==legacy_update.activated&&candidate.ejected==legacy_update.ejected&&candidate.newly_eligible==legacy_update.newly_eligible&&update_checks>3*count,"registry update changed semantics or lost progress");++cases;
 cancelled(initial,update,count+16,cases);cancelled(initial,update,update_checks,cases);
 // Also traverse a real activation queue and its cancellable comparisons.
 auto pending=initial;for(size_t i=count/2;i<count;++i)pending.validators.at(i).activation_epoch=UINT64_MAX;
 auto wider=updates;wider.minimum_churn_limit=64;const auto old_pending=prepare_phase0_registry_updates(pending,wider,pending.epoch);size_t pending_checks=0;const auto new_pending=prepare_phase0_registry_updates(pending,wider,pending.epoch,[&]{++pending_checks;});check(old_pending.registry_after==new_pending.registry_after&&old_pending.activated==new_pending.activated&&new_pending.activated.size()==64,"activation ordering/churn changed");
 cancelled(pending,[&](auto& s,const auto& cp){return prepare_phase0_registry_updates(s,wider,s.epoch,cp);},pending_checks,cases);groups.push_back("registry-staging-activation-and-cancellation");

 SignedConsensusVoluntaryExit exit{initial.epoch,0,{}};exit.signature=keys[0].sign(consensus_signing_root(consensus_voluntary_exit_root(exit.epoch,0),consensus_domain({4,0,0,0},version,genesis)));
 const auto voluntary=[&](auto& s,const auto& cp){return process_phase0_voluntary_exit(s,exits,exit,version,genesis,cp);};
 auto old=initial;check(process_phase0_voluntary_exit(old,exits,exit,version,genesis),"legacy exit rejected");state=initial;const auto exit_checks=checked_call(state,voluntary);check(state==old&&state.validators.at(0).exit_epoch==12&&state.validators.at(0).withdrawable_epoch==14,"exit queue semantics changed");++cases;
 cancelled(initial,voluntary,64,cases);cancelled(initial,voluntary,exit_checks,cases);
 const VoluntaryExitDomainContext context{VoluntaryExitProfile::Phase0,version,version,0,version,genesis};
 auto dispatched=initial;check(process_consensus_voluntary_exit(dispatched,exits,exit,context)&&dispatched==old,"legacy versioned exit dispatch changed");++cases;
 cancelled(initial,[&](auto& s,const auto& cp){return process_consensus_voluntary_exit(s,exits,exit,context,cp);},64,cases);groups.push_back("voluntary-exit-stages-and-versioned-dispatch");

 SignedConsensusBeaconHeader a,b;a.message.slot=80;a.message.proposer_index=1;a.message.state_root[0]=42;b=a;b.message.state_root[0]=43;
 a.signature=keys[1].sign(consensus_signing_root(consensus_beacon_header_root(a.message),consensus_domain({0,0,0,0},version,genesis)));b.signature=keys[1].sign(consensus_signing_root(consensus_beacon_header_root(b.message),consensus_domain({0,0,0,0},version,genesis)));
 const auto proposer=[&](auto& s,const auto& cp){return process_phase0_proposer_slashing(s,slashing,a,b,version,genesis,0,2,cp);};
 old=initial;check(bool(process_phase0_proposer_slashing(old,slashing,a,b,version,genesis,0,2)),"legacy proposer proof rejected");state=initial;const auto proposer_checks=checked_call(state,proposer);check(state==old&&state.validators.at(1).slashed&&state.validators.at(1).balance==31000&&state.validators.at(0).balance==32007&&state.validators.at(2).balance==32055,"proposer economics changed");++cases;
 cancelled(initial,proposer,64,cases);cancelled(initial,proposer,proposer_checks,cases);groups.push_back("proposer-slashing-progress-and-late-rollback");

 ConsensusAttestationData data;data.slot=64;data.source.epoch=1;data.target.epoch=2;data.beacon_block_root[0]=42;
 auto make_vote=[&](ConsensusAttestationData d){const auto root=consensus_signing_root(consensus_attestation_root(d),consensus_domain({1,0,0,0},version,genesis));return IndexedConsensusAttestation{d,{1,2},aggregate(keys[1].sign(root),keys[2].sign(root))};};
 const auto first=make_vote(data);data.beacon_block_root[0]=43;const auto second=make_vote(data);
 const auto attester=[&](auto& s,const auto& cp){return process_phase0_registry_attester_slashing(s,slashing,first,second,2048,version,version,genesis,0,2,cp);};
 old=initial;check(bool(process_phase0_registry_attester_slashing(old,slashing,first,second,2048,version,version,genesis,0,2)),"legacy attester proof rejected");state=initial;const auto attester_checks=checked_call(state,attester);check(state==old&&state.validators.at(1).slashed&&state.validators.at(2).slashed&&state.validators.at(0).balance==32014&&state.validators.at(2).balance==31110&&state.slashings.at(10)==64000,"attester economics changed");++cases;
 cancelled(initial,attester,64,cases);cancelled(initial,attester,attester_checks,cases);groups.push_back("registry-attester-slashing-progress-and-late-rollback");

 std::vector<ConsensusCommitteeMember> members;
 for(size_t i=0;i<keys.size();++i){ConsensusCommitteeMember m;m.validator_index=i;m.effective_weight=balance;m.possession_message.public_key=keys[i].public_key;m.possession_message.amount_gwei=32'000'000'000;m.possession_signature=keys[i].sign(consensus_signing_root(consensus_deposit_root(m.possession_message),consensus_domain({3,0,0,0},version,{})));members.push_back(std::move(m));}
 const ConsensusCommittee committee(members,version,consensus_domain({1,0,0,0},version,genesis));
 const auto legacy_verification=committee.verify(first.indices,consensus_attestation_root(first.data),first.signature);check(legacy_verification&&legacy_verification->signers==2,"legacy committee verifier rejected");
 const auto legacy_common=verify_attester_slashing(first,committee,second,committee);check(legacy_common&&*legacy_common==std::vector<uint64_t>({1,2}),"legacy attester verifier changed");
 auto legacy_committee=initial;check(bool(process_phase0_attester_slashing(legacy_committee,slashing,first,committee,second,committee,0,2))&&legacy_committee==old,"legacy committee transition changed");++cases;
 const auto committee_slash=[&](auto& s,const auto& cp){return process_phase0_attester_slashing(s,slashing,first,committee,second,committee,0,2,cp);};
 state=initial;const auto committee_checks=checked_call(state,committee_slash);check(state==old,"committee slash differs from registry slash");++cases;cancelled(initial,committee_slash,64,cases);cancelled(initial,committee_slash,committee_checks,cases);
 size_t verify_calls=0;const auto sentinel=std::make_exception_ptr(ExecutionCancelled("committee loop cancellation"));bool caught=false;try{(void)verify_attester_slashing(first,committee,second,committee,[&]{if(++verify_calls==3)std::rethrow_exception(sentinel);});}catch(...){check(std::current_exception()==sentinel,"committee cancellation replaced");caught=true;}check(caught&&verify_calls==3,"committee verification did not cooperate");++cases;groups.push_back("committee-attester-overloads-and-index-loop");

 auto penalized=initial;penalized.validators.at(1).slashed=true;penalized.validators.at(1).exit_epoch=12;penalized.validators.at(1).withdrawable_epoch=penalized.epoch+slashing.epochs_per_slashings_vector/2;penalized.slashings.at(0)=balance*count;
 const auto penalty=[&](auto& s,const auto& cp){return phase0_slashing_penalties(s,slashing,cp);};
 const auto old_debits=phase0_slashing_penalties(penalized,slashing);size_t penalty_checks=0;const auto debits=phase0_slashing_penalties(penalized,slashing,[&]{++penalty_checks;});check(debits==old_debits&&debits.size()==1&&debits.at(1)==balance,"delayed penalty changed");++cases;cancelled(penalized,penalty,64,cases);cancelled(penalized,penalty,penalty_checks,cases);groups.push_back("delayed-penalty-read-loops");

 const auto epoch=[&](auto& s,const auto& cp){return advance_phase0_slashing_epoch(s,slashing,initial.epoch,cp);};
 old=penalized;const auto old_epoch=advance_phase0_slashing_epoch(old,slashing,initial.epoch);state=penalized;size_t epoch_checks=0;const auto new_epoch=advance_phase0_slashing_epoch(state,slashing,initial.epoch,[&]{++epoch_checks;});check(old_epoch==new_epoch&&state==old&&state.epoch==11&&state.validators.at(1).balance==0,"epoch penalty commit changed");++cases;cancelled(penalized,epoch,64,cases);cancelled(penalized,epoch,epoch_checks,cases);groups.push_back("slashing-epoch-staging-and-final-swap");

 std::cout<<Json{{"scenario","consensus-cooperative-registry-cancellation"},{"groups",groups},{"cases",cases},{"syntheticRegistryEntries",count},{"persistent",false},{"newNetworkIdentities",false},{"consensusCampaignExecuted",false},{"productionQualified",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
