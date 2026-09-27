#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_capella_bls_change.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto folder=fixtures/"consensus-capella-bls-change-official";
 const auto manifest=load(folder/"manifest.json");
 check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f"&&manifest.at("cases").size()==14,"corpus pin");
 for(const auto& [file,info]:manifest.at("referenceFiles").items()){auto b=binary(fixtures/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"reference config digest");}
 const auto presets=fixtures/"consensus-bellatrix-containers-official";
 const auto phase0=read(presets/"minimal-phase0.yaml"),altair=read(presets/"minimal-altair.yaml"),bellatrix=read(presets/"minimal-bellatrix.yaml");
 auto p=[&](const char* k){return parameter(phase0,k);};
 const AltairStateLimits limits{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(altair,"SYNC_COMMITTEE_SIZE")};
 const ExecutionPayloadLimits execution{uint32_t(parameter(bellatrix,"MAX_EXTRA_DATA_BYTES")),uint32_t(parameter(bellatrix,"MAX_BYTES_PER_TRANSACTION")),uint32_t(parameter(bellatrix,"MAX_TRANSACTIONS_PER_PAYLOAD"))};
 const auto config=read(fixtures/"consensus-slashing-official/minimal-config.yaml");
 ConsensusBytes4 genesis_version{};bool found=false;std::istringstream lines(config);std::string line;
 while(std::getline(lines,line))if(line.starts_with("GENESIS_FORK_VERSION:")){std::string value;std::istringstream(line.substr(line.find(':')+1))>>value;auto raw=unhex(value);check(raw.size()==4,"genesis version width");std::copy(raw.begin(),raw.end(),genesis_version.begin());found=true;}
 check(found,"genesis fork version missing");
 const auto max_changes=parameter(read(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml"),"MAX_BLS_TO_EXECUTION_CHANGES");
 constexpr size_t budget=4*1024*1024;unsigned valid=0,invalid=0,mutations=0,replays=0,cancelled=0;std::set<std::string> names;
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();check(names.insert(name).second,"duplicate case");
  for(const auto& [file,info]:row.at("files").items()){auto b=binary(folder/name/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto pre=binary(folder/name/"pre.ssz"),wire=binary(folder/name/"address_change.ssz");const auto before=pre;
  const auto operation=decode_capella_bls_change(wire);check(operation.has_value()&&encode_capella_bls_change(*operation)==wire,"operation roundtrip");
  const auto root=capella_beacon_state_root(pre,limits,execution,budget);
  const auto unchanged=prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,{});check(unchanged&&unchanged->state==pre&&unchanged->root==root,"empty operation list changes state");
  const std::array changes{*operation};
  const auto candidate=prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,changes);
  check(pre==before,"input mutated");
  if(!row.at("valid").get<bool>()){check(!candidate,"invalid official operation accepted: "+name);++invalid;continue;}
  check(candidate.has_value(),"valid official operation rejected: "+name);++valid;
  const auto post=binary(folder/name/"post.ssz");check(candidate->state==post&&candidate->root==capella_beacon_state_root(post,limits,execution,budget),"whole SSZ state mismatch: "+name);
  check(!prepare_capella_bls_changes(post,candidate->root,limits,execution,budget,genesis_version,max_changes,changes),"replayed credentials accepted");++replays;
  std::array duplicate{*operation,*operation};check(!prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,duplicate)&&pre==before,"duplicate list partially applied");++mutations;
  for(unsigned field=0;field<4;++field){auto changed=changes;if(field==0)changed[0].validator_index=UINT64_MAX;if(field==1)changed[0].from_bls_pubkey[0]^=1;if(field==2)changed[0].to_execution_address[0]^=1;if(field==3)changed[0].signature[0]^=1;check(!prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,changed),"mutation accepted");++mutations;}
  auto version=genesis_version;version[0]^=1;check(!prepare_capella_bls_changes(pre,root,limits,execution,budget,version,max_changes,changes),"wrong configured domain accepted");++mutations;
  auto refuse=[&](auto f){bool denied=false;try{f();}catch(const std::invalid_argument&){denied=true;}check(denied,"invalid authenticated state accepted");++mutations;};
  auto wrong_root=root;wrong_root[0]^=1;refuse([&]{prepare_capella_bls_changes(pre,wrong_root,limits,execution,budget,genesis_version,max_changes,changes);});
  refuse([&]{prepare_capella_bls_changes(pre,root,limits,execution,pre.size()-1,genesis_version,max_changes,changes);});
  std::vector<SignedCapellaBlsChange> too_many(max_changes+1,*operation);check(!prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,too_many),"operation bound ignored");++mutations;
  unsigned steps=0;prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,changes,[&]{++steps;});
  for(unsigned stop:{1U,steps}){unsigned seen=0;bool stopped=false;try{prepare_capella_bls_changes(pre,root,limits,execution,budget,genesis_version,max_changes,changes,[&]{if(++seen==stop)throw ExecutionCancelled("test cancellation");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped&&pre==before,"cancellation leaked change");++cancelled;}
 }
 for(size_t size:{0U,171U,173U})check(!decode_capella_bls_change(Bytes(size)),"bad wire length accepted");
 check(valid==7&&invalid==7,"official case accounting");
 std::cout<<Json{{"scenario","capella-bls-to-execution-change"},{"officialValid",valid},{"officialInvalid",invalid},{"localRejections",mutations},{"replays",replays},{"cancellations",cancelled},{"wholeStateCompared",true},{"networkActivated",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
