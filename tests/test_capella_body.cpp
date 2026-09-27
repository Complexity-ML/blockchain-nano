#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_body.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto fixtures=fs::path(NANO_TEST_SOURCE)/"tests/fixtures",folder=fixtures/"consensus-capella-body-official",presets=fixtures/"consensus-bellatrix-containers-official";
 const auto m=load(folder/"manifest.json");check(m.at("commit")=="194a91fd9e6f97118156421aac4c80e6e47a9a7f"&&m.at("cases").size()==30,"body corpus pin");
 for(const auto& [file,info]:m.at("referenceFiles").items())check(hex(sha256(binary(fixtures/file))).substr(2)==info.at("sha256").get<std::string>(),"preset digest");
 const auto phase0=read(presets/"minimal-phase0.yaml"),altair=read(presets/"minimal-altair.yaml"),bellatrix=read(presets/"minimal-bellatrix.yaml"),capella=read(fixtures/"consensus-capella-withdrawals-official/minimal-capella.yaml");auto p=[&](const char* k){return uint32_t(parameter(phase0,k));};
 const Phase0BodyLimits limits{p("MAX_PROPOSER_SLASHINGS"),p("MAX_ATTESTER_SLASHINGS"),p("MAX_ATTESTATIONS"),p("MAX_DEPOSITS"),p("MAX_VOLUNTARY_EXITS"),p("MAX_VALIDATORS_PER_COMMITTEE")};
 const ExecutionPayloadLimits execution{uint32_t(parameter(bellatrix,"MAX_EXTRA_DATA_BYTES")),uint32_t(parameter(bellatrix,"MAX_BYTES_PER_TRANSACTION")),uint32_t(parameter(bellatrix,"MAX_TRANSACTIONS_PER_PAYLOAD"))};
 const auto sync=parameter(altair,"SYNC_COMMITTEE_SIZE"),withdrawals=parameter(capella,"MAX_WITHDRAWALS_PER_PAYLOAD"),changes=parameter(capella,"MAX_BLS_TO_EXECUTION_CHANGES");
 constexpr size_t budget=4*1024*1024;const size_t execution_field=316+(sync+7)/8,changes_field=execution_field+4;
 unsigned official=0,rejections=0,commitments=0,cancellations=0;
 auto root=[&](const Bytes& b,const std::function<void()>& check={}){return capella_beacon_body_root(b,limits,sync,execution,withdrawals,changes,budget,check);};
 auto refuse=[&](auto f){bool bad=false;try{f();}catch(const std::invalid_argument&){bad=true;}check(bad,"malformed body accepted");++rejections;};
 for(const auto& row:m.at("cases")){
  const auto path=folder/row.at("name").get<std::string>();for(const auto& [file,info]:row.at("files").items()){auto b=binary(path/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto b=binary(path/"serialized.ssz");const auto r=root(b);const auto expected=read(path/"roots.yaml");check(hex(Bytes(r.begin(),r.end()))==expected.substr(expected.find("0x"),66),"official body root mismatch");++official;
  refuse([&]{bellatrix_beacon_body_root(b,limits,sync,execution,budget);});
  refuse([&]{capella_beacon_body_root(b,limits,sync,execution,withdrawals,changes,b.size()-1);});
  for(size_t at:{size_t(200),size_t(204),size_t(208),size_t(212),size_t(216),execution_field,changes_field}){auto bad=b;std::fill_n(bad.begin()+at,4,0);refuse([&]{root(bad);});}
  auto bad=b;bad.push_back(0);refuse([&]{root(bad);});
  const auto start=little(b,changes_field,4),payload=little(b,execution_field,4);
  bad=b;bad.resize(start+172*(changes+1));refuse([&]{root(bad);});
  if(start<b.size())for(size_t off:{size_t(0),size_t(8),size_t(56),size_t(76)}){bad=b;bad[start+off]^=1;check(root(bad)!=r,"credential change field omitted from root");++commitments;}
  bad=b;bad[payload+472]^=1;check(root(bad)!=r,"payload block hash omitted");++commitments;
  const auto withdrawal_at=payload+little(b,payload+508,4);if(withdrawal_at<start){bad=b;bad[withdrawal_at+36]^=1;check(root(bad)!=r,"withdrawal amount omitted");++commitments;}
  unsigned calls=0;root(b,[&]{++calls;});for(unsigned stop:{1U,calls}){unsigned n=0;bool stopped=false;try{root(b,[&]{if(++n==stop)throw ExecutionCancelled("stop");});}catch(const ExecutionCancelled&){stopped=true;}check(stopped,"cancellation swallowed");++cancellations;}
 }
 check(commitments>30,"credential mutations absent");
 std::cout<<Json{{"scenario","capella-body-ssz"},{"officialRoots",official},{"rejections",rejections},{"commitmentMutations",commitments},{"cancellations",cancellations},{"networkActivated",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
