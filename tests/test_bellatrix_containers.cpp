#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_beacon_body.hpp"
#include "nano/consensus_beacon_state.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto folder=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-bellatrix-containers-official";
 const auto m=load(folder/"manifest.json");check(m.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&m.at("cases").size()==24,"Bellatrix corpus pin");
 for(const auto& row:m.at("presets")){const auto b=binary(folder/row.at("name").get<std::string>());check(b.size()==row.at("bytes")&&hex(sha256(b)).substr(2)==row.at("sha256").get<std::string>(),"preset digest");}
 const auto phase0=read(folder/"minimal-phase0.yaml"),altair=read(folder/"minimal-altair.yaml"),bellatrix=read(folder/"minimal-bellatrix.yaml");
 auto p=[&](const char* k){return parameter(phase0,k);};
 const AltairStateLimits s{p("SLOTS_PER_HISTORICAL_ROOT"),p("EPOCHS_PER_HISTORICAL_VECTOR"),p("EPOCHS_PER_SLASHINGS_VECTOR"),p("HISTORICAL_ROOTS_LIMIT"),p("EPOCHS_PER_ETH1_VOTING_PERIOD")*p("SLOTS_PER_EPOCH"),p("VALIDATOR_REGISTRY_LIMIT"),parameter(altair,"SYNC_COMMITTEE_SIZE")};
 const Phase0BodyLimits b{uint32_t(p("MAX_PROPOSER_SLASHINGS")),uint32_t(p("MAX_ATTESTER_SLASHINGS")),uint32_t(p("MAX_ATTESTATIONS")),uint32_t(p("MAX_DEPOSITS")),uint32_t(p("MAX_VOLUNTARY_EXITS")),uint32_t(p("MAX_VALIDATORS_PER_COMMITTEE"))};
 const ExecutionPayloadLimits e{uint32_t(parameter(bellatrix,"MAX_EXTRA_DATA_BYTES")),uint32_t(parameter(bellatrix,"MAX_BYTES_PER_TRANSACTION")),uint32_t(parameter(bellatrix,"MAX_TRANSACTIONS_PER_PAYLOAD"))};
 constexpr size_t budget=4*1024*1024;size_t total=0;std::map<std::string,unsigned> counts;
 for(const auto& row:m.at("cases")){
  const auto name=row.at("name").get<std::string>(),kind=name.substr(0,name.find('/'));const auto path=folder/name;
  for(const auto& [file,info]:row.at("files").items()){const auto bytes=binary(path/file);check(bytes.size()==info.at("bytes")&&hex(sha256(bytes)).substr(2)==info.at("sha256").get<std::string>(),"fixture digest");}
  const auto bytes=binary(path/"serialized.ssz");const auto yaml=read(path/"roots.yaml");ConsensusRoot root{};
  if(kind=="BeaconState")root=bellatrix_beacon_state_root(bytes,s,e,budget);
  else if(kind=="BeaconBlockBody")root=bellatrix_beacon_body_root(bytes,b,s.sync_committee_size,e,budget);
  else{const auto header=decode_bellatrix_execution_header(bytes,e.extra_data_bytes,budget);check(header.has_value(),"official execution header rejected");check(encode_bellatrix_execution_header(*header,e.extra_data_bytes,budget)==bytes,"official header roundtrip");root=bellatrix_execution_header_root(*header,e.extra_data_bytes);}
  check(hex(Bytes(root.begin(),root.end()))==yaml.substr(yaml.find("0x"),66),"official Bellatrix root mismatch: "+name);++counts[kind];total+=bytes.size();
 }
 check(counts==std::map<std::string,unsigned>{{"BeaconState",8},{"BeaconBlockBody",8},{"ExecutionPayloadHeader",8}},"container coverage");
 const auto state=binary(folder/"BeaconState/case_0/serialized.ssz"),body=binary(folder/"BeaconBlockBody/case_0/serialized.ssz"),header=binary(folder/"ExecutionPayloadHeader/case_0/serialized.ssz");
 size_t rejections=0,mutations=0,cancellations=0;
 auto deny=[&](const Bytes& data,unsigned kind,size_t maximum){bool refused=false;try{if(kind==0)bellatrix_beacon_state_root(data,s,e,maximum);if(kind==1)bellatrix_beacon_body_root(data,b,s.sync_committee_size,e,maximum);if(kind==2)refused=!decode_bellatrix_execution_header(data,e.extra_data_bytes,maximum);}catch(const std::invalid_argument&){refused=true;}check(refused,"malformed container accepted");++rejections;};
 for(unsigned kind=0;kind<3;++kind){const auto& bytes=kind==0?state:kind==1?body:header;deny({},kind,budget);deny(bytes,kind,bytes.size()-1);}
 const size_t state_field=272+64*s.slots_per_historical_root+32*s.epochs_per_historical_vector+8*s.epochs_per_slashings_vector+229+96*s.sync_committee_size;
 const size_t body_field=316+(s.sync_committee_size+7)/8;
 for(size_t field:{size_t(4272),size_t(7057),state_field}){auto bad=state;std::fill_n(bad.begin()+field,4,0);deny(bad,0,budget);bad=state;std::fill_n(bad.begin()+field,4,255);deny(bad,0,budget);}
 for(size_t field:{size_t(200),body_field}){auto bad=body;std::fill_n(bad.begin()+field,4,0);deny(bad,1,budget);bad=body;std::fill_n(bad.begin()+field,4,255);deny(bad,1,budget);}
 {auto bad=header;bad[436]^=1;deny(bad,2,budget);bad.resize(535);deny(bad,2,budget);}
 {auto bad=state;const auto at=little(state,state_field,4);bad[at+436]^=1;deny(bad,0,budget);bad=state;bad[at+472]^=1;check(bellatrix_beacon_state_root(bad,s,e,budget)!=bellatrix_beacon_state_root(state,s,e,budget),"execution header not committed in state");++mutations;}
 {auto bad=body;const auto at=little(body,body_field,4);bad[at+436]^=1;deny(bad,1,budget);bad=body;bad[at+472]^=1;check(bellatrix_beacon_body_root(bad,b,s.sync_committee_size,e,budget)!=bellatrix_beacon_body_root(body,b,s.sync_committee_size,e,budget),"execution payload not committed in body");++mutations;}
 for(unsigned kind=0;kind<3;++kind){auto hash=[&](const std::function<void()>& cp){if(kind==0)bellatrix_beacon_state_root(state,s,e,budget,cp);if(kind==1)bellatrix_beacon_body_root(body,b,s.sync_committee_size,e,budget,cp);if(kind==2)bellatrix_execution_header_root(*decode_bellatrix_execution_header(header,e.extra_data_bytes,budget),e.extra_data_bytes,cp);};size_t calls=0;hash([&]{++calls;});for(size_t stop:{size_t(1),calls/2,calls}){size_t count=0;bool aborted=false;try{hash([&]{if(++count==stop)throw std::runtime_error("cancel container");});}catch(const std::runtime_error& ex){aborted=std::string(ex.what())=="cancel container";}check(aborted,"container cancellation swallowed");++cancellations;}}
 const auto payload_folder=folder.parent_path()/"consensus-bellatrix-payload-official";
 for(unsigned i=0;i<8;++i){const auto payload=*decode_bellatrix_execution_payload(binary(payload_folder/("case_"+std::to_string(i))/"serialized.ssz"),e,budget);check(bellatrix_execution_header_root(bellatrix_execution_payload_header(payload,e),e.extra_data_bytes)==bellatrix_execution_payload_root(payload,e),"payload/header root differs");}
 std::cout<<Json{{"scenario","bellatrix-consensus-containers"},{"officialCases",counts},{"sszBytes",total},{"malformedRejections",rejections},{"nestedCommitmentMutations",mutations},{"cancellations",cancellations},{"networkActive",false}}.dump()<<'\n';
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
