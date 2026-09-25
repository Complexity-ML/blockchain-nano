#include "support/altair_transition_rules.hpp"
#include "nano/consensus_bellatrix_slots.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const fs::path fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",root=fixtures/"consensus-bellatrix-slots-official",refs=fixtures/"consensus-bellatrix-containers-official";
 const auto base=altair_transition_rules(fixtures);const auto l=base.state;
 const auto manifest=load(refs/"manifest.json");const auto bp=read(refs/"minimal-bellatrix.yaml");
 for(const auto& file:manifest.at("presets")){auto bytes=binary(refs/file.at("name").get<std::string>());check(hex(sha256(bytes)).substr(2)==file.at("sha256").get<std::string>(),"Bellatrix preset digest");}
 auto b=[&](const char* name){return parameter(bp,name);};
 BellatrixEpochRules r{base.epoch,{uint32_t(b("MAX_EXTRA_DATA_BYTES")),uint32_t(b("MAX_BYTES_PER_TRANSACTION")),uint32_t(b("MAX_TRANSACTIONS_PER_PAYLOAD"))},4*1024*1024};
 r.operations.rewards.inactivity_penalty_quotient=b("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX");
 r.operations.slashing.minimum_penalty_quotient=b("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX");
 r.operations.slashing.proportional_slashing_multiplier=b("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX");
 const auto slots=r.operations.ffg.slots_per_epoch;
 const auto m=load(root/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==7,"Bellatrix slots corpus");size_t slots_checked=0;BellatrixSlotState example;
 for(const auto& row:m.at("cases")){const auto name=row.at("name").get<std::string>();for(const auto& [path,info]:row.at("files").items())check(hex(sha256(binary(root/path))).substr(2)==info.at("sha256").get<std::string>(),"Bellatrix slots fixture digest");BellatrixSlotState input{binary(root/name/"pre.ssz"),std::nullopt};const auto start=little(input.ssz,40,8);if(start/slots)input.last_processed_epoch=start/slots-1;const auto n=std::stoull(read(root/name/"slots.yaml"));const auto post=prepare_bellatrix_slots(input,r,l,start+n,n,100000);check(post.ssz==binary(root/name/"post.ssz"),"official Bellatrix slots mismatch: "+name);auto split=input;for(uint64_t s=start+1;s<=start+n;++s)split=prepare_bellatrix_slots(split,r,l,s,1,100000);check(split==post,"split slots differ");slots_checked+=n;if(name=="historical_accumulator")example=input;}
 auto cache_slot=[&](BellatrixSlotState state){auto& b=state.ssz;const auto root=bellatrix_beacon_state_root(b,l,r.execution,r.maximum_state_bytes);const auto slot=little(b,40,8),pos=slot%l.slots_per_historical_root;std::copy(root.begin(),root.end(),b.begin()+176+32*l.slots_per_historical_root+32*pos);if(root_at(b,112)==ConsensusRoot{})std::copy(root.begin(),root.end(),b.begin()+112);const ConsensusBeaconHeader h{little(b,64,8),little(b,72,8),root_at(b,80),root_at(b,112),root_at(b,144)};const auto hr=consensus_beacon_header_root(h);std::copy(hr.begin(),hr.end(),b.begin()+176+32*pos);return state;};
 const auto start=little(example.ssz,40,8);const auto target=(start/slots+1)*slots;auto boundary=cache_slot(prepare_bellatrix_slots(example,r,l,target-1,target-start,100000));const auto epoch=(target-1)/slots;auto processed=prepare_bellatrix_epoch(boundary,r,l,epoch,100000);check(processed.last_processed_epoch==epoch&&little(processed.ssz,40,8)==target-1,"epoch marker and unchanged slot");
 unsigned rejected=0;auto refuse=[&](const BellatrixSlotState& s,const BellatrixEpochRules& cfg,uint64_t expected){bool threw=false;try{prepare_bellatrix_epoch(s,cfg,l,expected,100000);}catch(const std::exception&){threw=true;}check(threw,"invalid epoch accepted");++rejected;};refuse(processed,r,epoch);refuse(boundary,r,epoch+1);refuse(example,r,epoch);auto cfg=r;cfg.operations.rewards.effective_balance_increment++;refuse(boundary,cfg,epoch);cfg=r;cfg.operations.inactivity.score_bias++;refuse(boundary,cfg,epoch);
 bool budget=false;try{prepare_bellatrix_slots(example,r,l,start+8,7,100000);}catch(const std::length_error&){budget=true;}check(budget,"slot budget ignored");
 const auto sync_slot=r.operations.sync.epochs_per_sync_committee_period*slots-1;
 const auto late_state=cache_slot(prepare_bellatrix_slots(example,r,l,sync_slot,sync_slot-start,100000));const auto late_original=late_state;
 bool late_failure=false;try{prepare_bellatrix_epoch(late_state,r,l,sync_slot/slots,0);}catch(const std::length_error&){late_failure=true;}check(late_failure&&late_state==late_original,"late sync failure published epoch changes");
 const auto recovered=prepare_bellatrix_epoch(late_state,r,l,sync_slot/slots,100000);check(recovered.last_processed_epoch==sync_slot/slots,"retry after late failure");
 const auto original=boundary;size_t calls=0;prepare_bellatrix_epoch(boundary,r,l,epoch,100000,[&]{++calls;});std::set<size_t> stops{1,2,calls/4,calls/2,3*calls/4,calls-1,calls};for(auto stop:stops){size_t n=0;bool caught=false;try{prepare_bellatrix_epoch(boundary,r,l,epoch,100000,[&]{if(++n==stop)throw std::runtime_error("cancel");});}catch(const std::runtime_error& e){caught=std::string(e.what())=="cancel";}check(caught&&boundary==original,"cancelled epoch mutated source");}
 // A non-empty execution header must survive list growth, historical rollover,
 // epoch processing and sync rotation byte-for-byte. Its root must be cached.
 auto tagged=example;const size_t header_field=272+64*l.slots_per_historical_root+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector+229+96*l.sync_committee_size;
 const auto header_start=little(tagged.ssz,header_field,4);
 auto execution=*decode_bellatrix_execution_header(std::span<const uint8_t>(tagged.ssz).subspan(header_start),r.execution.extra_data_bytes,r.maximum_state_bytes);
 execution.block_hash[0]=42;execution.extra_data=Bytes(17,7);
 const auto header=encode_bellatrix_execution_header(execution,r.execution.extra_data_bytes,r.maximum_state_bytes);
 tagged.ssz.resize(header_start);tagged.ssz.insert(tagged.ssz.end(),header.begin(),header.end());
 const auto tagged_root=bellatrix_beacon_state_root(tagged.ssz,l,r.execution,r.maximum_state_bytes);
 const auto tagged_post=prepare_bellatrix_slots(tagged,r,l,start+l.slots_per_historical_root,l.slots_per_historical_root,100000);
 const auto new_start=little(tagged_post.ssz,header_field,4);
 check(Bytes(tagged_post.ssz.begin()+new_start,tagged_post.ssz.end())==header,"execution header changed during slots/epochs");
 check(root_at(tagged_post.ssz,176+32*l.slots_per_historical_root+32*(start%l.slots_per_historical_root))==tagged_root,"slot cache omitted Bellatrix header");
 auto smaller=r;smaller.maximum_state_bytes=tagged.ssz.size();bool growth_denied=false;
 try{prepare_bellatrix_slots(tagged,smaller,l,start+l.slots_per_historical_root,l.slots_per_historical_root,100000);}catch(const std::length_error&){growth_denied=true;}
 check(growth_denied&&bellatrix_beacon_state_root(tagged.ssz,l,r.execution,r.maximum_state_bytes)==tagged_root,"epoch growth budget ignored or parent mutated");
 std::cout<<Json{{"scenario","official-bellatrix-full-slots"},{"cases",7},{"slots",slots_checked},{"splitComparisons",7},{"epochRejections",rejected},{"cancellations",stops.size()},{"fullEpoch",true},{"fullBlockTransition",false},{"networkActive",false}}.dump()<<'\n';
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
