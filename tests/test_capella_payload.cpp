#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_capella_withdrawals.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",folder=fixtures/"consensus-capella-payload-official";
 const auto manifest=load(folder/"manifest.json");check(manifest.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f"&&manifest.at("cases").size()==30,"payload corpus pin");
 for(const auto& [file,info]:manifest.at("referenceFiles").items())check(hex(sha256(binary(fixtures/file))).substr(2)==info.at("sha256").get<std::string>(),"preset digest");
 const auto bellatrix=read(fixtures/"consensus-bellatrix-containers-official/minimal-bellatrix.yaml"),capella=read(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml");
 const ExecutionPayloadLimits limits{uint32_t(parameter(bellatrix,"MAX_EXTRA_DATA_BYTES")),uint32_t(parameter(bellatrix,"MAX_BYTES_PER_TRANSACTION")),uint32_t(parameter(bellatrix,"MAX_TRANSACTIONS_PER_PAYLOAD"))};const auto max_withdrawals=parameter(capella,"MAX_WITHDRAWALS_PER_PAYLOAD");constexpr size_t budget=4*1024*1024;
 const auto phase0=read(fixtures/"consensus-bellatrix-containers-official/minimal-phase0.yaml"),altair=read(fixtures/"consensus-bellatrix-containers-official/minimal-altair.yaml");auto p=[&](const char* k){return parameter(phase0,k);};
 const AltairStateLimits state_limits{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(altair,"SYNC_COMMITTEE_SIZE")};
 const CapellaWithdrawalRules rules{p("MAX_EFFECTIVE_BALANCE"),max_withdrawals,parameter(capella,"MAX_VALIDATORS_PER_WITHDRAWALS_SWEEP")};
 unsigned official=0,rejections=0,mutations=0,cancellations=0,withdrawal_valid=0,withdrawal_invalid=0;
 auto refuse=[&](auto f){bool denied=false;try{f();}catch(const std::invalid_argument&){denied=true;}check(denied,"invalid Capella payload accepted");++rejections;};
 for(const auto& row:manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();for(const auto& [file,info]:row.at("files").items()){auto b=binary(folder/name/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto wire=binary(folder/name/"serialized.ssz");const auto decoded=decode_capella_execution_payload(wire,limits,max_withdrawals,budget);check(decoded.has_value(),"official payload rejected");
  check(encode_capella_execution_payload(*decoded,limits,max_withdrawals,budget)==wire,"official wire roundtrip");const auto root=capella_execution_payload_root(*decoded,limits,max_withdrawals);const auto expected=read(folder/name/"roots.yaml");check(hex(Bytes(root.begin(),root.end()))==expected.substr(expected.find("0x"),66),"official payload root differs");++official;
  const auto header=capella_execution_payload_header(*decoded,limits,max_withdrawals);check(capella_execution_header_root(header,limits.extra_data_bytes)==root,"header/payload commitment differs");
  check(!decode_bellatrix_execution_payload(wire,limits,budget),"cross-fork payload admitted");++rejections;
  check(!decode_capella_execution_payload(wire,limits,max_withdrawals,wire.size()-1),"decode budget ignored");++rejections;
  refuse([&]{encode_capella_execution_payload(*decoded,limits,max_withdrawals,wire.size()-1);});
  for(size_t at:{436U,504U,508U}){auto bad=wire;std::fill_n(bad.begin()+at,4,0);check(!decode_capella_execution_payload(bad,limits,max_withdrawals,budget),"bad offset accepted");++rejections;}
  auto bad=wire;bad.push_back(0);check(!decode_capella_execution_payload(bad,limits,max_withdrawals,budget),"partial withdrawal accepted");++rejections;
  auto many=*decoded;many.withdrawals.resize(max_withdrawals+1);refuse([&]{encode_capella_execution_payload(many,limits,max_withdrawals,budget);});refuse([&]{capella_execution_payload_root(many,limits,max_withdrawals);});
  auto changed=*decoded;if(changed.withdrawals.empty())changed.withdrawals.push_back({0,0,{},0});else changed.withdrawals[0].amount_gwei^=1;check(capella_execution_payload_root(changed,limits,max_withdrawals)!=root,"withdrawals omitted from commitment");++mutations;
  unsigned calls=0;encode_capella_execution_payload(*decoded,limits,max_withdrawals,budget,[&]{++calls;});for(unsigned stop:{1U,calls}){unsigned n=0;bool stopped=false;try{encode_capella_execution_payload(*decoded,limits,max_withdrawals,budget,[&]{if(++n==stop)throw ExecutionCancelled("stop");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped,"cancellation ignored");++cancellations;}
 }
 const auto withdrawal_folder=fixtures/"consensus-capella-withdrawals-official";const auto withdrawal_manifest=load(withdrawal_folder/"manifest.json");
 check(withdrawal_manifest.at("commit")==manifest.at("commit")&&withdrawal_manifest.at("cases").size()==54,"withdrawal corpus pin");
 for(const auto& row:withdrawal_manifest.at("cases")){
  const auto name=row.at("name").get<std::string>();for(const auto& [file,info]:row.at("files").items()){auto b=binary(withdrawal_folder/name/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"withdrawal fixture digest");}
  const auto pre=binary(withdrawal_folder/name/"pre.ssz"),wire=binary(withdrawal_folder/name/"execution_payload.ssz");
  const auto payload=decode_capella_execution_payload(wire,limits,max_withdrawals,budget);check(payload.has_value(),"withdrawal operation payload encoding rejected: "+name);
  const auto root=capella_beacon_state_root(pre,state_limits,limits,budget);
  const auto post=prepare_capella_withdrawal_state(pre,root,state_limits,limits,budget,p("SLOTS_PER_EPOCH"),rules,payload->withdrawals);
  if(row.at("valid").get<bool>()){check(post&&post->state==binary(withdrawal_folder/name/"post.ssz"),"whole withdrawal state differs: "+name);++withdrawal_valid;}
  else{check(!post,"invalid official withdrawal accepted: "+name);++withdrawal_invalid;}
 }
 check(withdrawal_valid==35&&withdrawal_invalid==19,"withdrawal case accounting");
 // Empty lists and opaque transaction bytes must not overlap withdrawal bytes.
 CapellaExecutionPayload empty;const auto empty_wire=encode_capella_execution_payload(empty,limits,max_withdrawals,budget);check(empty_wire.size()==512&&decode_capella_execution_payload(empty_wire,limits,max_withdrawals,budget)==empty,"empty payload layout");
 empty.transactions={{},{1,2,3}};empty.extra_data={4,5};empty.withdrawals={{UINT64_MAX,UINT64_MAX,{},UINT64_MAX}};const auto mixed=encode_capella_execution_payload(empty,limits,max_withdrawals,budget);check(decode_capella_execution_payload(mixed,limits,max_withdrawals,budget)==empty,"mixed variable lists overlap");
 refuse([&]{decode_capella_execution_payload(mixed,limits,0,budget);});
 std::cout<<Json{{"scenario","capella-execution-payload"},{"officialRoots",official},{"withdrawalStatesMatched",withdrawal_valid},{"withdrawalInvalidRejected",withdrawal_invalid},{"rejections",rejections},{"withdrawalMutations",mutations},{"cancellations",cancellations},{"networkActivated",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
