#include "support/process.hpp"
#include "nano/consensus_registry_operations.hpp"
#include "blst.h"
using namespace test;
namespace {
void put(Bytes& b,size_t at,uint64_t value,size_t width){for(size_t i=0;i<width;++i)b.at(at+i)=uint8_t(value>>(8*i));}
Bytes sign(const blst_scalar& key,const ConsensusRoot& root){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&key);Bytes bytes(96);blst_p2_compress(bytes.data(),&s);return bytes;}
Bytes encode(const SignedConsensusBeaconHeader& h){Bytes bytes(208);put(bytes,0,h.message.slot,8);put(bytes,8,h.message.proposer_index,8);std::copy(h.message.parent_root.begin(),h.message.parent_root.end(),bytes.begin()+16);std::copy(h.message.state_root.begin(),h.message.state_root.end(),bytes.begin()+48);std::copy(h.message.body_root.begin(),h.message.body_root.end(),bytes.begin()+80);check(h.signature.size()==96,"test signature length");std::copy(h.signature.begin(),h.signature.end(),bytes.begin()+112);return bytes;}
}
int main(){try{
 const Phase0BodyLimits limits{16,2,128,16,16,2048};const ConsensusRandaoRules randao_rules{8,1};const Phase0SlashingRules slash_rules{2,32,4,256,64,64,512,8,1000000000,2};
 const Phase0StateFork fork{{1,0,0,0},{2,0,0,0},1};Phase0BlockContext context{16,{},fork.current_version,{},32000000000ULL,10,65536};context.parent_root[0]=8;context.genesis_root[0]=17;
 ConsensusSlashingState registry;registry.epoch=2;registry.slashings.resize(64);std::array<blst_scalar,4> keys;
 for(size_t i=0;i<keys.size();++i){std::array<uint8_t,32> seed{};seed[0]=i+11;blst_keygen(&keys[i],seed.data(),seed.size(),nullptr,0);blst_p1 pub;blst_sk_to_pk_in_g1(&pub,&keys[i]);ConsensusSlashingValidator v;blst_p1_compress(v.public_key.data(),&pub);v.balance=v.effective_balance=32000000000ULL;v.activation_epoch=0;if(i<3)registry.validators.emplace(i,v);}
 ConsensusRandaoState randao;randao.epoch=2;randao.mixes.resize(64);randao.last_reveal_slot=15;
 std::vector<Phase0ProposerValidator> candidates;for(const auto& [i,v]:registry.validators)candidates.push_back({i,v.effective_balance,v.activation_epoch,v.exit_epoch});
 const auto proposer=phase0_proposer_index(candidates,randao,randao_rules,context.slot,context.maximum_effective_balance,context.shuffle_rounds);const auto victim=(proposer+1)%3,other=(proposer+2)%3;
 using Proof=std::pair<SignedConsensusBeaconHeader,SignedConsensusBeaconHeader>;
 auto proof=[&](uint64_t index,uint64_t slot,const ConsensusBytes4& version){Proof out;out.first.message={slot,index,{},{},{}};out.second.message=out.first.message;out.second.message.body_root[0]=1;
  for(auto* header:{&out.first,&out.second})header->signature=sign(keys[index],consensus_signing_root(consensus_beacon_header_root(header->message),consensus_domain({0,0,0,0},version,context.genesis_root)));return out;};
 const Phase0DepositRules deposit_rules{1000000000,32000000000ULL,uint64_t(1)<<40,{7,0,0,0}};
 const Phase0ExitRules exit_rules{2,32,4,256,1};
 auto exit=[&](uint64_t index,uint64_t epoch,const ConsensusBytes4& version){return SignedConsensusVoluntaryExit{epoch,index,sign(keys[index],consensus_signing_root(consensus_voluntary_exit_root(epoch,index),consensus_domain({4,0,0,0},version,context.genesis_root)))};};
 auto deposit=[&](size_t index,bool valid_signature){Phase0Deposit d;blst_p1 pub;blst_sk_to_pk_in_g1(&pub,&keys[index]);blst_p1_compress(d.message.public_key.data(),&pub);d.message.amount_gwei=1000000000;d.message.withdrawal_credentials[0]=19;
  if(valid_signature){auto sig=sign(keys[index],consensus_signing_root(consensus_deposit_root(d.message),consensus_domain({3,0,0,0},deposit_rules.genesis_fork_version,{})));std::copy(sig.begin(),sig.end(),d.signature.begin());}return d;};
 auto bind=[&](std::vector<Phase0Deposit>& ds){check(ds.size()<=2,"test tree size");Phase0DepositState s{registry,{},ds.size(),0};if(ds.empty())return s;
  std::array<ConsensusRoot,33> zero{};for(size_t i=1;i<33;++i)zero[i]=consensus_signing_root(zero[i-1],zero[i-1]);
  ConsensusRoot count{};count[0]=ds.size();
  for(size_t i=0;i<ds.size();++i){for(size_t j=0;j<32;++j)ds[i].proof[j]=zero[j];if(ds.size()==2)ds[i].proof[0]=consensus_deposit_data_root(ds[1-i]);ds[i].proof[32]=count;}
  auto root=consensus_deposit_data_root(ds[0]);for(size_t i=0;i<33;++i)root=consensus_signing_root(root,ds[0].proof[i]);s.deposit_root=root;return s;};
 auto make=[&](const std::vector<Proof>& proofs,const std::vector<Phase0Deposit>& ds,const std::vector<SignedConsensusVoluntaryExit>& exits,bool attestation=false,bool attester_slashing=false,const Phase0SignedAttestation* signed_attestation=nullptr){
  Bytes bytes(404);put(bytes,0,100,4);put(bytes,100,context.slot,8);put(bytes,108,proposer,8);put(bytes,180,84,4);std::copy(context.parent_root.begin(),context.parent_root.end(),bytes.begin()+116);
  ConsensusRoot epoch{};epoch[0]=2;auto reveal=sign(keys[proposer],consensus_signing_root(epoch,consensus_domain({2,0,0,0},context.fork_version,context.genesis_root)));std::copy(reveal.begin(),reveal.end(),bytes.begin()+184);
  put(bytes,384,220,4);for(const auto& pair:proofs)for(const auto* h:{&pair.first,&pair.second}){auto raw=encode(*h);bytes.insert(bytes.end(),raw.begin(),raw.end());}
  put(bytes,388,bytes.size()-184,4);
  if(attester_slashing){Bytes raw(4+8+228*2);put(raw,0,4,4);put(raw,4,8,4);put(raw,8,236,4);put(raw,12,228,4);put(raw,240,228,4);bytes.insert(bytes.end(),raw.begin(),raw.end());}
  put(bytes,392,bytes.size()-184,4);
  if(attestation){Bytes raw(4+229);put(raw,0,4,4);put(raw,4,228,4);raw.back()=1;
   if(signed_attestation){const auto& a=*signed_attestation;check(a.aggregation_bits.size()==1&&a.signature.size()==96,"local signed attestation schema");put(raw,8,a.data.slot,8);put(raw,16,a.data.index,8);std::copy(a.data.beacon_block_root.begin(),a.data.beacon_block_root.end(),raw.begin()+24);put(raw,56,a.data.source.epoch,8);std::copy(a.data.source.root.begin(),a.data.source.root.end(),raw.begin()+64);put(raw,96,a.data.target.epoch,8);std::copy(a.data.target.root.begin(),a.data.target.root.end(),raw.begin()+104);std::copy(a.signature.begin(),a.signature.end(),raw.begin()+136);raw.back()=uint8_t(2|a.aggregation_bits[0]);}
   bytes.insert(bytes.end(),raw.begin(),raw.end());}
  put(bytes,396,bytes.size()-184,4);
  for(const auto& d:ds){Bytes raw(1240);for(size_t i=0;i<33;++i)std::copy(d.proof[i].begin(),d.proof[i].end(),raw.begin()+32*i);std::copy(d.message.public_key.begin(),d.message.public_key.end(),raw.begin()+1056);std::copy(d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end(),raw.begin()+1104);put(raw,1136,d.message.amount_gwei,8);std::copy(d.signature.begin(),d.signature.end(),raw.begin()+1144);bytes.insert(bytes.end(),raw.begin(),raw.end());}
  put(bytes,400,bytes.size()-184,4);
  for(const auto& e:exits){Bytes raw(112);put(raw,0,e.epoch,8);put(raw,8,e.validator_index,8);std::copy(e.signature.begin(),e.signature.end(),raw.begin()+16);bytes.insert(bytes.end(),raw.begin(),raw.end());}
  ConsensusBeaconHeader h{context.slot,proposer,context.parent_root,{},phase0_beacon_body_root(std::span<const uint8_t>(bytes).subspan(184),limits)};
  auto signature=sign(keys[proposer],consensus_signing_root(consensus_beacon_header_root(h),consensus_domain({0,0,0,0},context.fork_version,context.genesis_root)));std::copy(signature.begin(),signature.end(),bytes.begin()+4);return bytes;
 };
 auto run=[&](const Bytes& b,const Phase0DepositState& s,const std::function<void()>& cp={}){return prepare_phase0_registry_operations(b,context,limits,randao_rules,slash_rules,deposit_rules,exit_rules,fork,s,randao,cp);};
 const auto old_randao=randao;unsigned positive=0,rejected=0,unsupported=0,cancellations=0,configuration=0,overflows=0;
 const auto slash=proof(victim,1,fork.previous_version);
 const auto leave=exit(other,0,fork.previous_version),leave_proposer=exit(proposer,2,fork.current_version);
 std::vector<Phase0Deposit> ds{deposit(other,false),deposit(3,true)};const auto state=bind(ds),original=state;
 const auto block=make({slash},ds,{leave,leave_proposer});auto result=run(block,state);check(std::holds_alternative<Phase0RegistryOperationsCandidate>(result),"composed block rejected");const auto& candidate=std::get<Phase0RegistryOperationsCandidate>(result);
 check(candidate.proposer_slashings_processed==1&&candidate.deposits_processed==2&&candidate.exits_processed==2,"operation counts");
 const auto& after=candidate.deposits_after;check(after.next_deposit_index==2&&after.deposit_count==2&&after.deposit_root==state.deposit_root,"deposit cursor/root");
 check(after.registry.validators.size()==4&&after.registry.validators.at(3).activation_epoch==UINT64_MAX&&after.registry.validators.at(3).balance==1000000000,"new validator prematurely active or unfunded");
 check(after.registry.validators.at(other).balance==33000000000ULL&&after.registry.validators.at(other).withdrawal_credentials==registry.validators.at(other).withdrawal_credentials,"topup changed credentials or balance");
 check(after.registry.validators.at(victim).slashed&&after.registry.validators.at(victim).exit_epoch==7&&after.registry.validators.at(other).exit_epoch==7&&after.registry.validators.at(proposer).exit_epoch==8,"shared queue ordering/churn");
 check(after.registry.validators.at(other).withdrawable_epoch==263&&after.registry.validators.at(proposer).withdrawable_epoch==264,"withdrawal delay");check(candidate.authenticated.randao_after!=randao,"RANDAO not prepared");++positive;
 std::vector<Phase0Deposit> none;const auto empty_state=bind(none);check(std::holds_alternative<Phase0RegistryOperationsCandidate>(run(make({},none,{}),empty_state)),"empty candidate");++positive;
 std::vector<Phase0Deposit> ignored{deposit(3,false)};const auto ignored_state=bind(ignored);auto ignored_result=run(make({},ignored,{leave}),ignored_state);check(std::holds_alternative<Phase0RegistryOperationsCandidate>(ignored_result),"proven invalid PoP must consume deposit then allow exit");const auto& ignored_after=std::get<Phase0RegistryOperationsCandidate>(ignored_result).deposits_after;check(ignored_after.next_deposit_index==1&&ignored_after.registry.validators.size()==3,"invalid PoP admitted key");++positive;
 auto refuse=[&](const Bytes& b,const Phase0DepositState& s){const auto before=s;check(std::holds_alternative<InvalidPhase0RegistryBlock>(run(b,s)),"invalid block did not reject");check(s==before&&randao==old_randao,"late failure leaked changes");++rejected;};
 refuse(make({slash},ds,{leave,leave} ),state); // All deposits and one exit staged before failure.
 refuse(make({slash},ds,{exit(victim,0,fork.previous_version)}),state); // Slashing already scheduled exit.
 refuse(make({slash},ds,{exit(3,0,fork.previous_version)}),state); // Newly funded is not active.
 refuse(make({slash},ds,{exit(other,0,fork.current_version)}),state); // Historical exit domain.
 auto bad_exit=leave_proposer;bad_exit.signature[0]^=1;refuse(make({slash},ds,{leave,bad_exit}),state);
 auto bad=ds;bad.back().proof[0][0]^=1;refuse(make({slash},bad,{leave}),state); // First deposit must roll back.
 refuse(make({slash},{ds[0]},{leave}),state); // Mandatory second deposit omitted.
 refuse(make({},ds,{}),empty_state); // Unsolicited deposits.
 auto changed_root=state;changed_root.deposit_root[0]^=1;refuse(block,changed_root);
 auto bytes=block;bytes[4]^=1;refuse(bytes,state);
 for(auto [att,sl]:{std::pair{true,false},std::pair{false,true},std::pair{true,true}}){auto r=run(make({slash},ds,{leave},att,sl),state);check(std::holds_alternative<UnsupportedPhase0RegistryOperations>(r),"unprocessed preceding operations accepted");const auto& u=std::get<UnsupportedPhase0RegistryOperations>(r);check(u.attestations==size_t(att)&&u.attester_slashings==size_t(sl),"unsupported counts");++unsupported;}
 size_t total=0;check(std::holds_alternative<Phase0RegistryOperationsCandidate>(run(block,state,[&]{++total;})),"checkpoint control");
 for(size_t stop=1;stop<=total;++stop){size_t calls=0;bool aborted=false;try{run(block,state,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){aborted=std::string(e.what())=="cancelled";}check(aborted&&state==original&&randao==old_randao,"cancelled block changed canonical state");++cancellations;}
 auto bad_rules=exit_rules;bad_rules.minimum_churn_limit+=1;bool threw=false;try{prepare_phase0_registry_operations(block,context,limits,randao_rules,slash_rules,deposit_rules,bad_rules,fork,state,randao);}catch(const std::invalid_argument&){threw=true;}check(threw,"inconsistent queues accepted");++configuration;
 auto cursor=state;cursor.next_deposit_index=cursor.deposit_count+1;threw=false;try{run(block,cursor);}catch(const std::invalid_argument&){threw=true;}check(threw,"cursor underflow accepted");++configuration;
 auto overflowing=state;overflowing.registry.validators.at(other).balance=UINT64_MAX;const auto before_overflow=overflowing;threw=false;try{run(block,overflowing);}catch(const std::overflow_error&){threw=true;}check(threw&&overflowing==before_overflow&&randao==old_randao,"deposit overflow published slashing");++overflows;
 auto bad_tenure=exit_rules;bad_tenure.shard_committee_period=0;threw=false;try{prepare_phase0_registry_operations(make({},none,{}),context,limits,randao_rules,slash_rules,deposit_rules,bad_tenure,fork,empty_state,randao);}catch(const std::invalid_argument&){threw=true;}check(threw,"empty block bypassed invalid exit rules");++configuration;
 Phase0FfgState pending_state;pending_state.slot=context.slot;pending_state.block_roots.resize(64);const auto initial_pending=pending_state;
 const Phase0AttestationInclusionRules inclusion{{8,1,4,4,1000000000,32000000000ULL,10},1,1024};const Phase0AttestationStage stage{pending_state,inclusion};
 const auto duty=phase0_attestation_duties(registry,randao,inclusion.duties,1);check(duty.committees_per_slot==1&&duty.shuffled.size()==3,"local committee assumptions");const auto attester=duty.shuffled.back();
 Phase0SignedAttestation att;att.data.slot=15;att.data.target.epoch=1;att.aggregation_bits={true};att.signature=sign(keys[attester],consensus_signing_root(consensus_attestation_root(att.data),consensus_domain({1,0,0,0},fork.current_version,context.genesis_root)));
 auto run_att=[&](const Bytes& b,const std::function<void()>& cp={}){return prepare_phase0_registry_operations(b,context,limits,randao_rules,slash_rules,deposit_rules,exit_rules,fork,state,randao,cp,&stage);};
 const auto with_attestation=make({slash},ds,{leave,leave_proposer},true,false,&att);auto composed=run_att(with_attestation);check(std::holds_alternative<Phase0RegistryOperationsCandidate>(composed),"authenticated attestation block rejected");const auto& composed_candidate=std::get<Phase0RegistryOperationsCandidate>(composed);
 check(composed_candidate.attestations_processed==1&&composed_candidate.attestations_after&&composed_candidate.attestations_after->previous_attestations.size()==1&&composed_candidate.deposits_after==candidate.deposits_after,"integrated attestation candidate mismatch");const auto& included=composed_candidate.attestations_after->previous_attestations[0];check(included.data==att.data&&included.inclusion_delay==1&&included.proposer_index==proposer,"integrated inclusion metadata");
 auto wrong_att=att;wrong_att.signature[0]^=1;check(std::holds_alternative<InvalidPhase0RegistryBlock>(run_att(make({slash},ds,{leave},true,false,&wrong_att))),"invalid attestation in signed block accepted");
 check(std::holds_alternative<InvalidPhase0RegistryBlock>(run_att(make({slash},ds,{leave,leave},true,false,&att))),"late exit did not discard admitted attestation");
 size_t att_calls=0;run_att(with_attestation,[&]{++att_calls;});for(size_t stop=1;stop<=att_calls;++stop){size_t calls=0;bool threw=false;try{run_att(with_attestation,[&]{if(++calls==stop)throw std::runtime_error("cancelled");});}catch(const std::runtime_error& e){threw=std::string(e.what())=="cancelled";}check(threw&&pending_state==initial_pending&&state==original&&randao==old_randao,"integrated cancellation leaked state");}
 unsigned invalid_stage=0;for(unsigned mode=0;mode<2;++mode){auto bad_inclusion=inclusion;if(mode==0)bad_inclusion.duties.target_committee_size=0;else bad_inclusion.minimum_inclusion_delay=0;const Phase0AttestationStage bad_stage{pending_state,bad_inclusion};bool failed=false;try{prepare_phase0_registry_operations(make({},none,{}),context,limits,randao_rules,slash_rules,deposit_rules,exit_rules,fork,empty_state,randao,{},&bad_stage);}catch(const std::invalid_argument&){failed=true;}check(failed,"empty block bypassed invalid attestation stage");++invalid_stage;}
 check(pending_state==initial_pending,"integrated candidate changed pending source");
 std::cout<<Json{{"scenario","phase0-integrated-attestation-block"},{"valid",1},{"invalid",2},{"cancellations",att_calls},{"configurationRejections",invalid_stage},{"activeInConsensus",false},{"fullBlockTransition",false}}.dump()<<std::endl;
 check(state==original&&randao==old_randao,"success changed canonical state");
 std::cout<<Json{{"scenario","phase0-registry-block-operations"},{"positive",positive},{"rejected",rejected},{"unsupported",unsupported},{"cancellations",cancellations},{"configurationRejections",configuration},{"atomicOverflowRejections",overflows},{"fullStateTransition",false},{"activeInConsensus",false}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
