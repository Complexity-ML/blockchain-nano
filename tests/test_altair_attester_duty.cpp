#include "support/altair_transition_rules.hpp"
#include "nano/consensus_altair_reactor.hpp"
#include "nano/consensus_altair_duty.hpp"
#include "nano/consensus_bls.hpp"
#include "blst.h"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",corpus=fixtures/"consensus-altair-fork-head-official",dir=corpus/"shorter_chain_but_heavier_weight";
 const auto manifest=load(corpus/"manifest.json");for(const auto& row:manifest.at("cases"))if(row.at("name")=="shorter_chain_but_heavier_weight")for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(corpus/path))).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");
 const auto transition=altair_transition_rules(fixtures);const auto reference=read(corpus/"minimal-config-v1.6.0-beta.0.yaml");const auto duration=parameter(reference,"SECONDS_PER_SLOT")*1000;
 const AltairForkBlockRules rules{transition,duration,duration/3,parameter(reference,"PROPOSER_SCORE_BOOST"),AltairProposerBoostProfile::Release160Beta0};
 const AltairStoreSnapshotLimits limits{32*1024*1024,100,10000,10000,1024*1024,16*1024*1024,4096,100000};
 const auto initial=initialize_altair_fork_store({binary(dir/"anchor_state.ssz"),std::nullopt},binary(dir/"anchor_block.ssz"),rules);
 Temp temp;const Bytes genesis(32,9);ChainStore disk(temp/"core");disk.bind_chain(1337,genesis,"altair-qualification");AltairDurableReactor reactor(disk,1337,genesis,rules,limits,initial);
 const auto first_token=reactor.current().token;const auto now=initial.timed.clock.genesis_time_ms+duration+rules.attestation_due_ms;reactor.tick(now);
 const auto current=reactor.current();std::optional<AltairAttestationDuty> duty;uint64_t index=0;
 for(;index<initial.timed.view.justified_registry.validators.size();++index){duty=prepare_altair_attestation_duty(*current.state,rules,index,1,limits.replay_slots,limits.sync_trials);if(duty)break;}
 check(duty.has_value(),"no assigned fixture validator");
 // Official pyspec helpers/keys.py at v1.6.0-beta.0 uses privkeys[i] = i + 1.
 uint64_t words[4]{index+1,0,0,0};blst_scalar secret;blst_scalar_from_uint64(&secret,words);blst_p1 point;blst_sk_to_pk_in_g1(&point,&secret);Bytes key(48);blst_p1_compress(key.data(),&point);check(key==duty->public_key,"fixture private/public key mapping mismatch");
 unsigned calls=0,rejected=0;BeaconSigningJournal::Signer signer=[&](const ConsensusRoot& root){++calls;static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&secret);Bytes signature(96);blst_p2_compress(signature.data(),&s);return signature;};
 ChainStore journal_disk(temp/"signer");BeaconSigningJournal journal(journal_disk,1337,genesis,duty->genesis_root,key,transition.epoch.ffg.slots_per_epoch);
 auto refuse=[&](auto action){const auto before_calls=calls;const auto before=journal.commitment();bool failed=false;try{failed=!action().has_value();}catch(const std::exception&){failed=true;}check(failed&&calls==before_calls&&journal.commitment()==before,"rejected duty reached signer or mutated journal");++rejected;};
 refuse([&]{return reactor.sign_attestation(index,1,[&]{return now;},first_token,journal,signer);});
 refuse([&]{return reactor.sign_attestation(index,2,[&]{return now;},current.token,journal,signer);});
 refuse([&]{return reactor.sign_attestation(index,1,[&]{return now-duration;},current.token,journal,signer);});
 refuse([&]{return reactor.sign_attestation(index,1,[&]{return now+duration;},current.token,journal,signer);});
 refuse([&]{return reactor.sign_attestation(UINT64_MAX,1,[&]{return now;},current.token,journal,signer);});
 refuse([&]{return reactor.sign_attestation(index,1,[&]{return now;},current.token,journal,signer,[]{throw std::runtime_error("cancel duty");});});
 for(unsigned variant=0;variant<5;++variant){
  auto wrong_genesis=duty->genesis_root;auto wrong_key=key;auto wrong_execution=genesis;auto slots=transition.epoch.ffg.slots_per_epoch;Int chain=1337;
  if(variant==0)wrong_genesis[0]^=1;
  if(variant==1){uint64_t other[4]{index+2,0,0,0};blst_scalar s;blst_scalar_from_uint64(&s,other);blst_p1 p;blst_sk_to_pk_in_g1(&p,&s);blst_p1_compress(wrong_key.data(),&p);}
  if(variant==2)wrong_execution[0]^=1;if(variant==3)++slots;if(variant==4)++chain;
  ChainStore other_disk(temp/("identity-"+std::to_string(variant)));BeaconSigningJournal other(other_disk,chain,wrong_execution,wrong_genesis,wrong_key,slots);const auto before=other.commitment();
  refuse([&]{return reactor.sign_attestation(index,1,[&]{return now;},current.token,other,signer);});check(other.commitment()==before,"wrong identity journal changed");
 }
 auto early=*current.state;early.timed.clock.time_ms=initial.timed.clock.genesis_time_ms+duration;
 check(!prepare_altair_attestation_duty(early,rules,index,1,limits.replay_slots,limits.sync_trials),"attested before due time without current block");
 auto equivocation=*current.state;equivocation.timed.view.equivocating_indices.insert(index);check(!prepare_altair_attestation_duty(equivocation,rules,index,1,limits.replay_slots,limits.sync_trials),"known equivocator assigned");
 // Synthetic eligibility variants of the pinned anchor, re-bound to new
 // state/block roots; these are guard tests, not upstream transition fixtures.
 for(unsigned variant=0;variant<2;++variant){
  auto state=initial.blocks.at(initial.timed.view.anchor).state;auto block=binary(dir/"anchor_block.ssz");
  const size_t offset=176+64*transition.state.slots_per_historical_root+88;uint32_t validators_offset=0;for(unsigned i=0;i<4;++i)validators_offset|=uint32_t(state.ssz[offset+i])<<(8*i);
  if(variant==0)state.ssz[validators_offset+121*index+88]=1;
  else{for(unsigned i=0;i<8;++i)state.ssz[validators_offset+121*index+97+i]=i==0?1:0;}
  const auto state_root=altair_beacon_state_root(state.ssz,transition.state);std::copy(state_root.begin(),state_root.end(),block.begin()+48);
  auto candidate=initialize_altair_fork_store(state,block,rules);candidate=prepare_altair_store_tick(candidate,now,rules,limits.replay_slots,limits.sync_trials);
  check(!prepare_altair_attestation_duty(candidate,rules,index,1,limits.replay_slots,limits.sync_trials),"slashed or inactive validator assigned");
 }
 unsigned clock_reads=0;
 refuse([&]{return reactor.sign_attestation(index,1,[&]{return ++clock_reads==1?now:now+duration;},current.token,journal,signer);});
 // A clock can move backwards without crossing a slot boundary or the Store
 // timestamp. Refuse it before reservation, rather than hiding the regression.
 clock_reads=0;
 refuse([&]{return reactor.sign_attestation(index,1,[&]{return ++clock_reads==1?now+2:now+1;},current.token,journal,signer);});
 // Every post-reservation failure must withhold the result while preserving
 // the durable guard, including failures after BLS verification has completed.
 unsigned temporal_withheld=0;
 for(unsigned variant=0;variant<4;++variant){
  ChainStore temporal_disk(temp/("temporal-"+std::to_string(variant)));
  BeaconSigningJournal temporal(temporal_disk,1337,genesis,duty->genesis_root,key,transition.epoch.ffg.slots_per_epoch);
  const auto before=temporal.commitment();const auto prior_calls=calls;unsigned reads=0;bool provider_done=false,failed=false;
  auto time=[&]{++reads;if(variant==0&&reads>=4)return now+1;if(variant==1&&reads>=5)return now+duration;return now+2;};
  auto provider=[&](const ConsensusRoot& root){auto signature=signer(root);provider_done=true;return signature;};
  auto cancel=[&]{if(variant==2&&provider_done)throw std::runtime_error("cancel after provider");};
  auto clock=[&]{if(variant==3&&reads==4)throw std::runtime_error("clock unavailable after verification");return time();};
  try{reactor.sign_attestation(index,1,clock,current.token,temporal,provider,cancel);}catch(const std::runtime_error&){failed=true;}
  check(failed&&calls==prior_calls+1&&temporal.commitment().sequence==before.sequence+1,"temporal failure returned signature or lost reservation");
  const auto committed=temporal.commitment();
  BeaconSigningJournal reopened(temporal_disk,1337,genesis,duty->genesis_root,key,transition.epoch.ffg.slots_per_epoch,committed);
  auto conflict=duty->attestation.data;conflict.beacon_block_root[0]^=1;const auto prior_retry=calls;bool refused=false;
  try{reopened.sign_attestation(conflict,duty->fork_version,signer);}catch(const std::runtime_error&){refused=true;}
  check(refused&&calls==prior_retry&&reopened.commitment()==committed,"temporal failure guard lost on reopen");++temporal_withheld;
 }
 const auto before_valid_calls=calls;
 const auto signed_vote=reactor.sign_attestation(index,1,[&]{return now;},current.token,journal,signer);check(signed_vote.has_value()&&calls==before_valid_calls+1,"authenticated duty did not sign");
 check(signed_vote->data==duty->attestation.data&&signed_vote->data.slot==1&&signed_vote->data.beacon_block_root==initial.timed.view.anchor&&signed_vote->data.target==ConsensusCheckpoint{0,initial.timed.view.anchor}&&std::count(signed_vote->aggregation_bits.begin(),signed_vote->aggregation_bits.end(),true)==1,"duty construction changed");
 check(verify_consensus_bls(key,consensus_signing_root(consensus_attestation_root(signed_vote->data),consensus_domain({1,0,0,0},duty->fork_version,duty->genesis_root)),signed_vote->signature),"duty signature domain");
 check(reactor.current().token==current.token&&reactor.current().signed_votes->empty(),"signing published an unadmitted vote");
 // Expiry inside the provider retains its reservation but withholds the result.
 {ChainStore late_disk(temp/"late-provider");BeaconSigningJournal late(late_disk,1337,genesis,duty->genesis_root,key,transition.epoch.ffg.slots_per_epoch);const auto before=late.commitment();unsigned reads=0;const auto prior_calls=calls;bool expired=false;
 try{reactor.sign_attestation(index,1,[&]{return ++reads<=3?now:now+duration;},current.token,late,signer);}catch(const std::runtime_error&){expired=true;}
 check(expired&&calls==prior_calls+1&&late.commitment().sequence==before.sequence+1,"expired provider result escaped or lost reservation");}
 // A valid late block changes the head; anti-slashing still forbids a second
 // vote for a different head in this epoch, before calling the provider.
 Bytes late_block;for(const auto& step:load(dir/"steps.json"))if(step.contains("block")){late_block=binary(dir/(step.at("block").get<std::string>()+".ssz"));break;}
 // Produce the official block locally from its body using the selected key,
 // and force it through independent signed-block admission afterwards.
 unsigned proposal_refusals=0,randao_refusals=0;
 {
  const auto proposer=little(late_block,108,8);uint64_t pwords[4]{proposer+1,0,0,0};blst_scalar psecret;blst_scalar_from_uint64(&psecret,pwords);blst_p1 pp;blst_sk_to_pk_in_g1(&pp,&psecret);Bytes pkey(48);blst_p1_compress(pkey.data(),&pp);
  check(std::equal(pkey.begin(),pkey.end(),initial.timed.view.justified_registry.validators.at(proposer).public_key.begin()),"proposer fixture key mismatch");
  unsigned provider_calls=0;
  auto provider=[&](const ConsensusRoot& root){++provider_calls;static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&psecret);Bytes signature(96);blst_p2_compress(signature.data(),&s);return signature;};
  ChainStore pdisk(temp/"proposer");BeaconSigningJournal pj(pdisk,1337,genesis,duty->genesis_root,pkey,transition.epoch.ffg.slots_per_epoch);
  Bytes body(late_block.begin()+184,late_block.end());
  auto deny=[&](auto action){const auto token=pj.commitment();const auto before=provider_calls;bool refused=false;try{refused=!action().has_value();}catch(const std::exception&){refused=true;}check(refused&&provider_calls==before&&pj.commitment()==token,"refused proposal reached provider or changed journal");++proposal_refusals;};
  deny([&]{return reactor.sign_proposal(proposer,1,body,[&]{return now;},first_token,pj,provider);});
  deny([&]{return reactor.sign_proposal(proposer,2,body,[&]{return now;},current.token,pj,provider);});
  deny([&]{return reactor.sign_proposal(proposer,1,body,[&]{return now+duration;},current.token,pj,provider);});
  deny([&]{return reactor.sign_proposal(UINT64_MAX,1,body,[&]{return now;},current.token,pj,provider);});
  auto invalid=body;invalid[0]^=1;
  deny([&]{return reactor.sign_proposal(proposer,1,invalid,[&]{return now;},current.token,pj,provider);});
  deny([&]{return reactor.sign_proposal(proposer,1,body,[&]{return now;},current.token,pj,provider,[]{throw std::runtime_error("cancel proposal");});});
  {ChainStore wrong_disk(temp/"wrong-proposer-journal");auto wrong_genesis=duty->genesis_root;wrong_genesis[0]^=1;BeaconSigningJournal wrong(wrong_disk,1337,genesis,wrong_genesis,pkey,transition.epoch.ffg.slots_per_epoch);const auto before=wrong.commitment();
   deny([&]{return reactor.sign_proposal(proposer,1,body,[&]{return now;},current.token,wrong,provider);});check(wrong.commitment()==before,"wrong proposer identity journal changed");}
  unsigned proposal_clock_reads=0;
  deny([&]{return reactor.sign_proposal(proposer,1,body,[&]{return ++proposal_clock_reads==1?now+2:now+1;},current.token,pj,provider);});
  auto randao_deny=[&](auto action){const auto before=pj.commitment();const auto calls_before=provider_calls;bool denied=false;try{denied=!action().has_value();}catch(const std::exception&){denied=true;}check(denied&&calls_before==provider_calls&&pj.commitment()==before,"invalid RANDAO reached provider or changed guards");++randao_refusals;};
  randao_deny([&]{return reactor.sign_randao(UINT64_MAX,1,[&]{return now;},current.token,pj,provider);});
  randao_deny([&]{return reactor.sign_randao(proposer,1,[&]{return now;},first_token,pj,provider);});
  randao_deny([&]{return reactor.sign_randao(proposer,2,[&]{return now;},current.token,pj,provider);});
  randao_deny([&]{return reactor.sign_randao(proposer,1,[&]{return now+duration;},current.token,pj,provider);});
  randao_deny([&]{return reactor.sign_randao(proposer,1,[&]{return now;},current.token,pj,provider,[]{throw std::runtime_error("cancel RANDAO");});});
  {ChainStore retired_disk(temp/"retired-randao");BeaconSigningJournal retired(retired_disk,1337,genesis,duty->genesis_root,pkey,transition.epoch.ffg.slots_per_epoch);retired.retire_and_export_interchange();
   randao_deny([&]{return reactor.sign_randao(proposer,1,[&]{return now;},current.token,retired,provider);});}
  {ChainStore wrong_disk(temp/"wrong-randao");auto g=duty->genesis_root;g[0]^=1;BeaconSigningJournal wrong(wrong_disk,1337,genesis,g,pkey,transition.epoch.ffg.slots_per_epoch);
   randao_deny([&]{return reactor.sign_randao(proposer,1,[&]{return now;},current.token,wrong,provider);});}
  const auto before_randao=pj.commitment();
  const auto reveal=reactor.sign_randao(proposer,1,[&]{return now;},current.token,pj,provider);
  check(reveal&&reveal->size()==96&&std::equal(reveal->begin(),reveal->end(),body.begin())&&pj.commitment()==before_randao&&reactor.current().token==current.token,"local RANDAO differs from official or changed guards/Store");
  std::copy(reveal->begin(),reveal->end(),body.begin());
  {bool bad=false;try{reactor.sign_randao(proposer,1,[&]{return now;},current.token,pj,[](const ConsensusRoot&){return Bytes(96,0);});}catch(const std::runtime_error&){bad=true;}check(bad&&pj.commitment()==before_randao,"invalid RANDAO provider accepted");}
  {unsigned reads=0;bool withheld=false;try{reactor.sign_randao(proposer,1,[&]{return ++reads<5?now:now+duration;},current.token,pj,provider);}catch(const std::runtime_error&){withheld=true;}check(withheld&&reads==5&&pj.commitment()==before_randao,"expired RANDAO released or changed guards");}
  // Exercise an epoch advance from the authenticated anchor. The epoch-one
  // reveal must verify against epoch one, not the epoch-zero fixture domain.
  {
   const auto next_slot=transition.epoch.ffg.slots_per_epoch+1;
   const auto& anchor=initial.blocks.at(initial.timed.view.anchor);
   const auto selected=prepare_altair_proposer_identity(anchor.state,anchor.header,next_slot,transition.epoch,transition.state,transition.attestation.duties,limits.replay_slots,limits.sync_trials);
   check(selected.has_value(),"missing next-epoch proposer");uint64_t nwords[4]{selected->index+1,0,0,0};blst_scalar ns;blst_scalar_from_uint64(&ns,nwords);blst_p1 np;blst_sk_to_pk_in_g1(&np,&ns);Bytes nk(48);blst_p1_compress(nk.data(),&np);check(nk==selected->public_key,"next-epoch proposer key mismatch");
   ChainStore ndisk(temp/"next-epoch-core");ndisk.bind_chain(1337,genesis,"altair-qualification");AltairDurableReactor next(ndisk,1337,genesis,rules,limits,initial);const auto next_time=initial.timed.clock.genesis_time_ms+next_slot*duration;next.tick(next_time);
   ChainStore njdisk(temp/"next-epoch-key");BeaconSigningJournal nj(njdisk,1337,genesis,selected->genesis_root,nk,transition.epoch.ffg.slots_per_epoch);
   auto nprovider=[&](const ConsensusRoot& root){static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";blst_p2 h,s;blst_hash_to_g2(&h,root.data(),root.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0);blst_sign_pk_in_g1(&s,&h,&ns);Bytes sig(96);blst_p2_compress(sig.data(),&s);return sig;};
   const auto nreveal=next.sign_randao(selected->index,next_slot,[&]{return next_time;},next.current().token,nj,nprovider);check(nreveal.has_value(),"next-epoch RANDAO missing");ConsensusRoot one{};one[0]=1;const auto domain=consensus_domain({2,0,0,0},selected->fork_version,selected->genesis_root);
   check(verify_consensus_bls(nk,consensus_signing_root(one,domain),*nreveal)&&!verify_consensus_bls(nk,consensus_signing_root(ConsensusRoot{},domain),*nreveal),"RANDAO epoch domain not advanced");
  }
  const auto before_proposal_calls=provider_calls;
  auto produced=reactor.sign_proposal(proposer,1,body,[&]{return now;},current.token,pj,provider);
  check(produced&&*produced==late_block&&provider_calls==before_proposal_calls+1&&reactor.current().token==current.token&&reactor.current().signed_blocks->empty(),"local proposal differs from official block or published before admission");
  // Graffiti is valid arbitrary data, so only anti-slashing may reject this
  // different otherwise valid block at the same slot.
  auto different=body;different[168]^=1;
  check(prepare_altair_unsigned_block(initial.blocks.at(initial.timed.view.anchor).state,1,proposer,initial.timed.view.anchor,different,transition,limits.replay_slots,limits.sync_trials).has_value(),"conflicting proposal not independently valid");
  const auto committed=pj.commitment();BeaconSigningJournal reopened(pdisk,1337,genesis,duty->genesis_root,pkey,transition.epoch.ffg.slots_per_epoch,committed);
  bool conflict=false;const auto before_conflict=provider_calls;
  try{reactor.sign_proposal(proposer,1,different,[&]{return now;},current.token,reopened,provider);}catch(const std::runtime_error& e){conflict=std::string(e.what())=="Conflicting or old beacon proposal refused";}
  check(conflict&&provider_calls==before_conflict&&reopened.commitment()==committed,"conflicting proposal signed after journal reopen");++proposal_refusals;
  {ChainStore expiry_disk(temp/"expired-proposal");BeaconSigningJournal expiry(expiry_disk,1337,genesis,duty->genesis_root,pkey,transition.epoch.ffg.slots_per_epoch);const auto before=expiry.commitment();unsigned reads=0;bool withheld=false;
   try{reactor.sign_proposal(proposer,1,body,[&]{return ++reads<5?now:now+duration;},current.token,expiry,provider);}catch(const std::runtime_error&){withheld=true;}
   check(withheld&&reads==5&&expiry.commitment().sequence==before.sequence+1,"expired proposal escaped or lost reservation");}
  // Independent reactor initialized from the same anchor admits only the
  // complete signed output, and publishes it into its durable archive.
  ChainStore receiver_disk(temp/"proposal-receiver");receiver_disk.bind_chain(1337,genesis,"altair-qualification");AltairDurableReactor receiver(receiver_disk,1337,genesis,rules,limits,initial);receiver.tick(now);
  check(receiver.block(*produced)&&receiver.current().signed_blocks->size()==1,"locally signed proposal failed independent admission");
  const auto received=receiver.current();const auto before_reject=provider_calls;
  check(!receiver.sign_proposal(proposer,1,body,[&]{return now;},received.token,reopened,provider),"proposed on head already at current slot");++proposal_refusals;
  AltairDurableReactor stale_receiver(receiver_disk,1337,genesis,rules,limits,received.token);
  receiver.tick(initial.timed.clock.genesis_time_ms+2*duration);bool stale_denied=false;
  try{stale_receiver.sign_proposal(proposer,1,body,[&]{return now;},received.token,reopened,provider);}catch(const std::exception&){stale_denied=true;}
  check(stale_denied&&provider_calls==before_reject&&reopened.commitment()==committed,"stale proposal coordinator reached provider");++proposal_refusals;
  const auto recovered_token=receiver.current().token;AltairDurableReactor recovered(receiver_disk,1337,genesis,rules,limits,recovered_token);
  check(recovered.current().signed_blocks->size()==1&&*recovered.current().signed_blocks->begin()->second==*produced,"signed proposal archive differs after recovery");

 }
 check(reactor.block(late_block),"official slot-one block rejected");const auto changed=reactor.current();
 auto boundary_delay=rules;boundary_delay.attestation_due_ms=duration;
 check(prepare_altair_attestation_duty(*changed.state,boundary_delay,index,1,limits.replay_slots,limits.sync_trials).has_value(),"current block did not permit attesting before fallback delay");
 const auto changed_duty=prepare_altair_attestation_duty(*changed.state,rules,index,1,limits.replay_slots,limits.sync_trials);check(changed_duty&&changed_duty->attestation.data.beacon_block_root!=signed_vote->data.beacon_block_root,"late block did not yield a valid different-head duty");
 {const auto before=journal.commitment();const auto prior_calls=calls;bool conflict=false;try{reactor.sign_attestation(index,1,[&]{return now;},changed.token,journal,signer);}catch(const std::runtime_error& e){conflict=std::string(e.what())=="Conflicting or old beacon attestation refused";}check(conflict&&calls==prior_calls&&journal.commitment()==before,"changed head was not refused specifically by anti-slashing");++rejected;}
 // Another coordinator with the same DB cannot sign from an old checkpoint.
 AltairDurableReactor stale(disk,1337,genesis,rules,limits,changed.token);
 reactor.tick(initial.timed.clock.genesis_time_ms+2*duration);refuse([&]{return stale.sign_attestation(index,1,[&]{return now;},changed.token,journal,signer);});
 check(reactor.vote(*signed_vote)&&reactor.current().signed_votes->size()==1,"locally produced vote rejected by independent admission");
 std::cout<<Json{{"scenario","altair-attester-duty"},{"validatorIndex",index},{"signedVotes",1},{"signedOfficialProposals",1},{"officialRandaoReproduced",true},{"nextEpochRandaoVerified",true},{"randaoRefusalsBeforeProvider",randao_refusals},{"proposalRefusalsBeforeProvider",proposal_refusals},{"expiredProviderResultsWithheld",1},{"additionalTemporalResultsWithheld",temporal_withheld},{"syntheticEligibilityVariants",2},{"rejectedBeforeProvider",rejected},{"independentAdmission",true},{"staleCoordinatorRefused",true},{"source","official Altair anchor and synthetic scheduled duty"},{"executionBound",false},{"autonomousValidator",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
