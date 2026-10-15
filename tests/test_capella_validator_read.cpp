#include "support/durable_owned_fixture.hpp"
#include "../src/beacon_validator_json.hpp"
#include "nano/beacon_api.hpp"
#include "nano/consensus_capella_fork_state.hpp"
using namespace nano;
namespace fixture=nano::test::durable_owned;
namespace {
uint64_t reads=0,refusals=0;
const char* current_stage="validator_status_projection";unsigned last_status=0;
void need(bool ok){if(!ok)throw std::runtime_error("validator read fixture refused");}
std::string root_text(const ConsensusRoot& r){return hex(Bytes(r.begin(),r.end()));}
BeaconReadResponse read(const CapellaForkStore& s,const CapellaForkBlockRules& r,const std::string& p){++reads;auto out=capella_beacon_read(s,r,p,fixture::checkpoint);last_status=out.status;return out;}
void emit(const char* scenario,const Json& extra=Json::object()){
 Json row={{"scenario",scenario},{"passed",true},{"apiSignaturesCreated",0},{"fixtureBlsSignatures",fixture::test_signatures},{"databaseAccessed",false},{"runtimeStarted",false},{"productionQualified",false}};
 for(const auto& [key,value]:extra.items())row[key]=value;std::cout<<row.dump()<<'\n';
}
}
int main(){try{
 struct Case{uint64_t eligibility,activation,exit,withdrawable,balance,epoch;bool slashed;const char* expected;};
 const uint64_t f=UINT64_MAX;
 const Case cases[]={{f,f,f,f,1,0,false,"pending_initialized"},{2,10,f,f,1,5,false,"pending_queued"},
  {2,3,f,f,1,3,false,"active_ongoing"},{2,3,10,20,1,9,false,"active_exiting"},
  {2,3,10,20,1,9,true,"active_slashed"},{2,3,10,20,1,10,false,"exited_unslashed"},
  {2,3,10,20,1,19,true,"exited_slashed"},{2,3,10,20,1,20,false,"withdrawal_possible"},
  {2,3,10,20,0,20,false,"withdrawal_done"}};
 for(const auto& c:cases){
  ConsensusSlashingValidator v;v.activation_eligibility_epoch=c.eligibility;v.activation_epoch=c.activation;v.exit_epoch=c.exit;v.withdrawable_epoch=c.withdrawable;v.balance=c.balance;v.slashed=c.slashed;v.effective_balance=32000000000ull;
  const auto row=detail::beacon_validator_json(19,v,c.epoch);need(row.size()==4&&row.at("index")=="19"&&row.at("balance")==std::to_string(c.balance)&&row.at("status")==c.expected&&row.at("validator").size()==8&&row.at("validator").at("slashed")==c.slashed);
 }
 ConsensusSlashingValidator maximum;maximum.balance=UINT64_MAX;maximum.effective_balance=UINT64_MAX;
 const auto precision=detail::beacon_validator_json(UINT64_MAX,maximum,0);
 need(precision.at("index")=="18446744073709551615"&&precision.at("balance")=="18446744073709551615"&&precision.at("validator").at("effective_balance")=="18446744073709551615"&&precision.at("validator").at("activation_epoch")=="18446744073709551615");
 emit("validator_status_projection",{{"statuses",9},{"fullUint64Strings",true}});

 current_stage="validator_projection_fixture_initialization";
 auto model=fixture::fixture();auto& store=model.initial;const auto before=capella_store_snapshot(store,model.rules,model.limits);
 const auto pin=capella_fork_head(store,model.rules,fixture::checkpoint);const auto& block=*pin;
 const auto& limits=model.rules.transition.consensus.state;
 const auto context=capella_fork_block_context(block.state,block.header,{model.rules.transition.consensus.epoch,model.rules.transition.execution,model.rules.transition.maximum_state_bytes},limits,fixture::checkpoint);
 const auto& first=*context.registry.validators.begin();const auto key=hex(Bytes(first.second.public_key.begin(),first.second.public_key.end()));
 const auto route=std::string("/eth/v1/beacon/states/head/validators/");
 current_stage="validator_authenticated_selection";
 const auto by_index=read(store,model.rules,route+std::to_string(first.first));const auto by_key=read(store,model.rules,route+key);
 need(by_index.status==200&&by_key.body==by_index.body&&by_index.body.at("data").at("index")==std::to_string(first.first)&&by_index.body.at("data").at("balance")==std::to_string(first.second.balance));
 const auto& projected=by_index.body.at("data").at("validator");
 need(projected.at("pubkey")==key&&projected.at("withdrawal_credentials")==hex(Bytes(first.second.withdrawal_credentials.begin(),first.second.withdrawal_credentials.end()))&&projected.at("effective_balance")==std::to_string(first.second.effective_balance)&&projected.at("activation_eligibility_epoch")==std::to_string(first.second.activation_eligibility_epoch)&&projected.at("activation_epoch")==std::to_string(first.second.activation_epoch)&&projected.at("exit_epoch")==std::to_string(first.second.exit_epoch)&&projected.at("withdrawable_epoch")==std::to_string(first.second.withdrawable_epoch)&&projected.at("slashed")==first.second.slashed);
 need(by_index.body.at("execution_optimistic")==false&&by_index.body.contains("finalized"));
 const auto state_route=std::string("/eth/v1/beacon/states/")+root_text(block.header.state_root)+"/validators/";
 need(read(store,model.rules,state_route+key).body==by_index.body);
 need(read(store,model.rules,route+"000"+std::to_string(first.first)).body==by_index.body);
 emit("validator_authenticated_selection",{{"indexAndPublicKeyAgree",true},{"immutableStateAgrees",true},{"leadingZeroIndexAccepted",true}});

 current_stage="validator_selector_refusals";
 for(const std::string& id:std::vector<std::string>{"","-1","+1","18446744073709551616","0x01","0x"+std::string(96,'g'),"1/extra","1?x=y"}){need(read(store,model.rules,route+id).status==400);++refusals;}
 need(read(store,model.rules,route+"18446744073709551615").status==404);++refusals;
 need(read(store,model.rules,route+"0x"+std::string(96,'0')).status==404);++refusals;
 need(read(store,model.rules,"/eth/v1/beacon/states/0x"+std::string(64,'f')+"/validators/0").status==404);++refusals;
 need(read(store,model.rules,"/eth/v1/beacon/states/0x12/validators/0").status==400);++refusals;
 need(read(store,model.rules,"/eth/v1/beacon/states/head/validators").status==404);++refusals;
 emit("validator_selector_refusals",{{"expectedHttpRefusals",refusals}});

 current_stage="validator_projection_integrity";
 auto corrupt=store;corrupt.blocks.at(store.timed.view.anchor).writable().state.ssz[8]^=1;
 bool damaged=false;try{read(corrupt,model.rules,state_route+key);}catch(const std::invalid_argument&){damaged=true;}need(damaged);++refusals;
 BeaconReadPhase phase=BeaconReadPhase::publication;bool cancelled=false;
 try{capella_beacon_read(store,model.rules,route+key,[&]{if(phase==BeaconReadPhase::projection)throw ExecutionCancelled("opaque fixture cancellation");},[&](auto p){phase=p;});}catch(const ExecutionCancelled&){cancelled=true;}
 need(cancelled&&phase==BeaconReadPhase::projection);++refusals;
 need(capella_store_snapshot(store,model.rules,model.limits)==before);
 emit("validator_projection_integrity",{{"corruptStateRejected",true},{"projectionCancellationObserved",true},{"originalStoreUnchanged",true}});

 // Reconstruct the same trusted genesis anchor and replay the already signed
 // first block. Both real post-states remain retained; no state/root is forged
 // to create a historical lookup fixture.
 current_stage="validator_retained_historical_state";
 const auto& r=model.rules.transition;
 auto body=fixture::test_body(model.pre,r,Bytes(96),fixture::test_payload(model.genesis.block));
 Bytes unsigned_anchor(84);const auto pre_root=capella_beacon_state_root(model.pre.ssz,limits,r.execution,r.maximum_state_bytes,fixture::checkpoint);
 std::copy(pre_root.begin(),pre_root.end(),unsigned_anchor.begin()+48);fixture::test_put(unsigned_anchor,80,84,4);unsigned_anchor.insert(unsigned_anchor.end(),body.begin(),body.end());
 auto historical=initialize_capella_fork_store(model.pre,unsigned_anchor,model.genesis.block,model.genesis.state,{}, {},model.rules,fixture::checkpoint);
 const auto original_anchor=historical.timed.view.anchor;
 historical=prepare_capella_resumed_store_tick(historical,historical.timed.clock.genesis_time_ms+model.rules.slot_duration_ms,model.rules,model.limits.replay_slots,model.limits.sync_trials,fixture::checkpoint);
 auto next=prepare_capella_resumed_fork_block(historical,model.wire,model.rules,model.limits.replay_slots,model.limits.sync_trials,fixture::checkpoint);need(bool(next));historical=std::move(*next);
 need(historical.blocks.size()==2);
 const auto old=historical.blocks.at(original_anchor).lease();
 const auto older_context=capella_fork_block_context(old->state,old->header,{r.consensus.epoch,r.execution,r.maximum_state_bytes},limits,fixture::checkpoint);
 auto changed=std::find_if(older_context.registry.validators.begin(),older_context.registry.validators.end(),[&](const auto& row){return row.second.balance!=context.registry.validators.at(row.first).balance;});
 need(changed!=older_context.registry.validators.end());const auto old_path=std::string("/eth/v1/beacon/states/")+root_text(old->header.state_root)+"/validators/"+std::to_string(changed->first);
 const auto historical_before=capella_store_snapshot(historical,model.rules,model.limits);
 const auto older_read=read(historical,model.rules,old_path),current_read=read(historical,model.rules,route+std::to_string(changed->first));
 need(older_read.status==200&&current_read.status==200&&older_read.body.at("data").at("balance")==std::to_string(changed->second.balance)&&current_read.body.at("data").at("balance")==std::to_string(context.registry.validators.at(changed->first).balance)&&older_read.body.at("data")!=current_read.body.at("data"));
 auto removed=historical;removed.blocks.erase(original_anchor);need(read(removed,model.rules,old_path).status==404);++refusals;
 need(capella_store_snapshot(historical,model.rules,model.limits)==historical_before);
 emit("validator_retained_historical_state",{{"realSignedTransition",true},{"distinctHistoricalBalance",true},{"removedStateDoesNotFallback",true},{"sourceStoreUnchanged",true}});
 std::cout<<Json{{"scenario","validator_read_summary"},{"passed",true},{"groups",5},{"statuses",9},{"httpReads",reads},{"expectedRefusals",refusals},{"fixtureBlsSignatures",fixture::test_signatures},{"auditSignatures",0},{"databaseAccessed",false},{"runtimeStarted",false},{"networkStarted",false},{"peakRssKiB",fixture::public_peak_rss_kib()},{"productionQualified",false}}.dump()<<'\n';return 0;
}catch(...){std::cerr<<Json{{"scenario","validator_read_refused"},{"stage",current_stage},{"lastHttpStatus",last_status},{"httpReads",reads},{"fixtureBlsSignatures",fixture::test_signatures},{"passed",false},{"detailsExported",false}}.dump()<<'\n';return 1;}}
