#include "support/altair_transition_rules.hpp"
#include "nano/consensus_capella_fork.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",folder=fixtures/"consensus-capella-fork-official",refs=fixtures/"consensus-bellatrix-containers-official";
 const auto base=altair_transition_rules(fixtures);const auto l=base.state;const auto preset=read(refs/"minimal-bellatrix.yaml");auto b=[&](const char* key){return parameter(preset,key);};
 check(hex(sha256(binary(refs/"minimal-bellatrix.yaml"))).substr(2)=="96b8fbfa1a4af173be05f0dc5563af3b9df9e8327277d8f1957a4fdfa49b7cd9","Bellatrix preset digest");
 BellatrixEpochRules rules{base.epoch,{uint32_t(b("MAX_EXTRA_DATA_BYTES")),uint32_t(b("MAX_BYTES_PER_TRANSACTION")),uint32_t(b("MAX_TRANSACTIONS_PER_PAYLOAD"))},4*1024*1024};
 rules.operations.rewards.inactivity_penalty_quotient=b("INACTIVITY_PENALTY_QUOTIENT_BELLATRIX");rules.operations.slashing.minimum_penalty_quotient=b("MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX");rules.operations.slashing.proportional_slashing_multiplier=b("PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX");const auto slots=rules.operations.ffg.slots_per_epoch;
 const auto config=read(fixtures/"consensus-slashing-official/minimal-config.yaml");auto version=[&](std::string name){std::istringstream lines(config);std::string line;while(std::getline(lines,line))if(line.starts_with(name+":")){std::string value;std::istringstream(line.substr(line.find(':')+1))>>value;const auto raw=unhex(value);check(raw.size()==4,"version width");ConsensusBytes4 v;std::copy(raw.begin(),raw.end(),v.begin());return v;}throw std::runtime_error("missing fork version");};
 const auto capella=version("CAPELLA_FORK_VERSION"),bellatrix=version("BELLATRIX_FORK_VERSION");
 const auto manifest=load(folder/"manifest.json");check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f"&&manifest.at("cases").size()==14,"fork corpus pin");unsigned count=0,rejected=0;
 for(const auto& row:manifest.at("cases")){
  const auto path=folder/row.at("name").get<std::string>();for(const auto& [file,info]:row.at("files").items()){const auto bytes=binary(path/file);check(bytes.size()==info.at("bytes")&&hex(sha256(bytes)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const BellatrixSlotState input{binary(path/"pre.ssz"),std::nullopt};const auto root=bellatrix_beacon_state_root(input.ssz,l,rules.execution,rules.maximum_state_bytes);
  const auto result=upgrade_capella_state(input,root,l,rules.execution,rules.maximum_state_bytes,slots,capella);check(result.ssz==binary(path/"post.ssz"),"official fork state differs");++count;
 }
 const BellatrixSlotState input{binary(fixtures/"consensus-bellatrix-slots-official/slots_1/pre.ssz"),std::nullopt};const auto saved=input;
 const auto root=bellatrix_beacon_state_root(input.ssz,l,rules.execution,rules.maximum_state_bytes);const CapellaForkSchedule schedule{1,bellatrix,capella};
 const auto post=prepare_capella_fork_slots(input,root,rules,l,schedule,slots+3,slots+3,100000);
 const auto boundary=prepare_bellatrix_slots(input,rules,l,slots,slots,100000);
 const auto boundary_root=bellatrix_beacon_state_root(boundary.ssz,l,rules.execution,rules.maximum_state_bytes);
 const auto upgraded=upgrade_capella_state(boundary,boundary_root,l,rules.execution,rules.maximum_state_bytes,slots,capella);
 const auto expected=prepare_capella_slots(upgraded,{rules.operations,rules.execution,rules.maximum_state_bytes},l,slots+3,3,100000);
 check(post==expected&&post.last_processed_epoch==0&&input==saved,"skipped-slot fork differs");
 const auto upgraded_root=capella_beacon_state_root(upgraded.ssz,l,rules.execution,rules.maximum_state_bytes);
 check(prepare_capella_fork_slots(boundary,boundary_root,rules,l,schedule,slots,0,100000)==upgraded,"exact-boundary upgrade differs");
 check(root_at(post.ssz,176+32*l.slots_per_historical_root+32*slots)==upgraded_root,"wrong fork root cached at boundary");
 auto deny=[&](auto f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad&&input==saved,"invalid migration accepted");++rejected;};
 deny([&]{prepare_capella_fork_slots(input,root,rules,l,schedule,slots-1,slots,100000);});
 deny([&]{prepare_capella_fork_slots(input,root,rules,l,schedule,slots+3,slots+2,100000);});
 auto wrong=root;wrong[0]^=1;deny([&]{prepare_capella_fork_slots(input,wrong,rules,l,schedule,slots,slots,100000);});
 auto altered=schedule;altered.bellatrix_version[0]^=1;deny([&]{prepare_capella_fork_slots(input,root,rules,l,altered,slots,slots,100000);});
 altered=schedule;altered.epoch=UINT64_MAX;deny([&]{prepare_capella_fork_slots(input,root,rules,l,altered,slots,slots,100000);});
 deny([&]{upgrade_capella_state({upgraded.ssz,upgraded.last_processed_epoch},upgraded_root,l,rules.execution,rules.maximum_state_bytes,slots,capella);});
 auto bad_boundary=boundary;for(unsigned i=0;i<8;++i)bad_boundary.ssz[64+i]=uint8_t((slots+1)>>(8*i));
 auto bad_root=bellatrix_beacon_state_root(bad_boundary.ssz,l,rules.execution,rules.maximum_state_bytes);
 deny([&]{prepare_capella_fork_slots(bad_boundary,bad_root,rules,l,schedule,slots,0,100000);});
 bad_boundary=boundary;bad_boundary.last_processed_epoch=1;
 deny([&]{prepare_capella_fork_slots(bad_boundary,boundary_root,rules,l,schedule,slots,0,100000);});
 auto bad_rules=rules;bad_rules.execution.transactions=0;
 deny([&]{prepare_capella_fork_slots(boundary,boundary_root,bad_rules,l,schedule,slots,0,100000);});
 // A schema-valid SSZ state may still have fewer balances than validators.
 bad_boundary=boundary;
 const size_t history=176+64*l.slots_per_historical_root,participation=history+96+32*l.epochs_per_historical_vector+8*l.epochs_per_slashings_vector;
 auto offset=[&](size_t at){uint32_t n=0;for(unsigned i=0;i<4;++i)n|=uint32_t(bad_boundary.ssz[at+i])<<(8*i);return n;};
 const auto balances_end=offset(participation);bad_boundary.ssz.erase(bad_boundary.ssz.begin()+balances_end-8,bad_boundary.ssz.begin()+balances_end);
 for(size_t at:std::array<size_t,4>{participation,participation+4,participation+129,participation+229+96*l.sync_committee_size}){const auto value=offset(at)-8;for(unsigned i=0;i<4;++i)bad_boundary.ssz[at+i]=uint8_t(value>>(8*i));}
 bad_root=bellatrix_beacon_state_root(bad_boundary.ssz,l,rules.execution,rules.maximum_state_bytes);
 deny([&]{prepare_capella_fork_slots(bad_boundary,bad_root,rules,l,schedule,slots,0,100000);});
 size_t calls=0;prepare_capella_fork_slots(input,root,rules,l,schedule,slots+3,slots+3,100000,[&]{++calls;});unsigned cancellations=0;
 for(size_t stop:{size_t(1),calls/2,calls}){size_t n=0;bool stopped=false;try{prepare_capella_fork_slots(input,root,rules,l,schedule,slots+3,slots+3,100000,[&]{if(++n==stop)throw ExecutionCancelled("stop");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped&&input==saved,"cancelled migration modified parent");++cancellations;}
 std::cout<<Json{{"scenario","capella-fork-upgrade"},{"officialStates",count},{"rejected",rejected},{"cancellations",cancellations},{"skippedSlots",3},{"networkActivated",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
