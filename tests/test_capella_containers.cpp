#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_state.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures";
 const auto folder=fixtures/"consensus-capella-containers-official";
 const auto manifest=load(folder/"manifest.json");check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f"&&manifest.at("cases").size()==16,"official corpus pin");
 const auto presets=fixtures/"consensus-bellatrix-containers-official";
 const auto phase0=read(presets/"minimal-phase0.yaml"),altair=read(presets/"minimal-altair.yaml"),bellatrix=read(presets/"minimal-bellatrix.yaml");
 auto p=[&](const char* k){return parameter(phase0,k);};
 const AltairStateLimits limits{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(altair,"SYNC_COMMITTEE_SIZE")};
 const ExecutionPayloadLimits execution{uint32_t(parameter(bellatrix,"MAX_EXTRA_DATA_BYTES")),uint32_t(parameter(bellatrix,"MAX_BYTES_PER_TRANSACTION")),uint32_t(parameter(bellatrix,"MAX_TRANSACTIONS_PER_PAYLOAD"))};
 constexpr size_t budget=4*1024*1024;unsigned headers=0,states=0,mutations=0,rejections=0,cancellations=0,historical=0;
 auto refuse=[&](auto f){bool denied=false;try{f();}catch(const std::invalid_argument&){denied=true;}check(denied,"malformed Capella accepted");++rejections;};
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();const auto path=folder/name;
  for(const auto& [file,info]:row.at("files").items()){const auto b=binary(path/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest mismatch");}
  const auto data=binary(path/"serialized.ssz");const auto expected=read(path/"roots.yaml");ConsensusRoot actual;
  if(name.starts_with("ExecutionPayloadHeader")){
   auto header=decode_capella_execution_header(data,execution.extra_data_bytes,budget);check(header.has_value(),"official Capella header rejected");
   check(encode_capella_execution_header(*header,execution.extra_data_bytes,budget)==data,"Capella header roundtrip");
   actual=capella_execution_header_root(*header,execution.extra_data_bytes);
   auto changed=*header;changed.withdrawals_root[0]^=1;check(capella_execution_header_root(changed,execution.extra_data_bytes)!=actual,"withdrawals root not committed");++mutations;
   check(!decode_bellatrix_execution_header(data,execution.extra_data_bytes,budget),"Capella header admitted as Bellatrix");++rejections;
   check(!decode_capella_execution_header(data,execution.extra_data_bytes,data.size()-1),"header byte budget ignored");++rejections;
   auto bad=data;bad[436]=0;check(!decode_capella_execution_header(bad,execution.extra_data_bytes,budget),"header offset accepted");++rejections;
   refuse([&]{encode_capella_execution_header(*header,execution.extra_data_bytes,data.size()-1);});
   ++headers;
  }else{
   actual=capella_beacon_state_root(data,limits,execution,budget);++states;
   const size_t randao=272+64*limits.slots_per_historical_root;
   const size_t previous=randao+32*limits.epochs_per_historical_vector+8*limits.epochs_per_slashings_vector;
   const size_t header_field=previous+229+96*limits.sync_committee_size;
   const size_t header_at=little(data,header_field,4),history_at=little(data,header_field+20,4);
   const auto unchanged=transform_capella_beacon_state(data,actual,limits,execution,budget,[](auto owned)->std::optional<Bytes>{return Bytes(owned.begin(),owned.end());});
   check(unchanged&&unchanged->state==data&&unchanged->root==actual,"unchanged transform root mismatch");
   const size_t historical_field=randao-96;
   const std::vector<size_t> variable_fields{historical_field,historical_field+76,historical_field+88,historical_field+92,previous,previous+4,previous+129,header_field,header_field+20};
   for(size_t f=0;f<variable_fields.size();++f){const auto begin=little(data,variable_fields[f],4),end=f+1<variable_fields.size()?little(data,variable_fields[f+1],4):data.size();if(begin==end)continue;
    auto changed=data;changed.at(begin)^=1;const auto expected_changed=capella_beacon_state_root(changed,limits,execution,budget);
    const auto transformed=transform_capella_beacon_state(data,actual,limits,execution,budget,[&](auto)->std::optional<Bytes>{return changed;});
    check(transformed&&transformed->state==changed&&transformed->root==expected_changed&&transformed->root!=actual,"changed variable field reused stale root");++mutations;
   }
   bool invoked=false;auto wrong=actual;wrong[0]^=1;refuse([&]{transform_capella_beacon_state(data,wrong,limits,execution,budget,[&](auto)->std::optional<Bytes>{invoked=true;return data;});});check(!invoked,"transform invoked before authentication");
   check(!transform_capella_beacon_state(data,actual,limits,execution,budget,[](auto)->std::optional<Bytes>{return std::nullopt;}),"rejected transformation published state");
   refuse([&]{transform_capella_beacon_state(data,actual,limits,execution,budget,[&](auto)->std::optional<Bytes>{auto bad=data;for(size_t k=0;k<4;++k)bad.at(header_field+20+k)=0;return bad;});});
   auto external=data;const auto isolated=transform_capella_beacon_state(external,actual,limits,execution,budget,[&](auto owned)->std::optional<Bytes>{external.at(0)^=1;return Bytes(owned.begin(),owned.end());});check(isolated&&isolated->state==data&&isolated->root==actual,"transform snapshot aliased mutable input");

   for(size_t at:{header_field+4,header_field+12,header_at+536}){auto changed=data;changed.at(at)^=1;check(capella_beacon_state_root(changed,limits,execution,budget)!=actual,"Capella field omitted from root");++mutations;}
   if(history_at<data.size()){auto changed=data;changed.at(history_at)^=1;check(capella_beacon_state_root(changed,limits,execution,budget)!=actual,"historical summary omitted");++historical;++mutations;}
   refuse([&]{bellatrix_beacon_state_root(data,limits,execution,budget);});
   refuse([&]{capella_beacon_state_root(data,limits,execution,data.size()-1);});
   auto bad=data;bad.push_back(0);refuse([&]{capella_beacon_state_root(bad,limits,execution,budget);});
   bad=data;for(size_t k=0;k<4;++k)bad.at(header_field+20+k)=0;refuse([&]{capella_beacon_state_root(bad,limits,execution,budget);});
   bad=data;bad.at(header_at+436)=0;refuse([&]{capella_beacon_state_root(bad,limits,execution,budget);});
   for(unsigned stop:{1U,30U}){unsigned calls=0;bool cancelled=false;try{capella_beacon_state_root(data,limits,execution,budget,[&]{if(++calls==stop)throw std::runtime_error("cancel Capella");});}catch(const std::runtime_error& e){cancelled=std::string(e.what())=="cancel Capella";}check(cancelled,"cancellation swallowed");++cancellations;}
  }
  check(hex(Bytes(actual.begin(),actual.end()))==expected.substr(expected.find("0x"),66),"official Capella root mismatch: "+name);
 }
 check(headers==8&&states==8&&historical>0,"missing official coverage");
 std::cout<<Json{{"officialHeaders",headers},{"officialStates",states},{"commitmentMutations",mutations},{"historicalSummaryMutations",historical},{"malformedRejections",rejections},{"cancellations",cancellations},{"networkActivated",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
