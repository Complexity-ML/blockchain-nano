#include "support/phase0_minimal_ssz.hpp"
#include "nano/consensus_execution_payload.hpp"
using namespace test;using namespace test::phase0_minimal;
int main(){try{
 const auto folder=fs::path(NANO_TEST_SOURCE)/"tests/fixtures/consensus-bellatrix-payload-official";const auto manifest=load(folder/"manifest.json");check(manifest.at("commit")=="bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908"&&manifest.at("cases").size()==8,"execution corpus pin");
 const auto preset=binary(folder/"minimal-bellatrix.yaml");check(hex(sha256(preset)).substr(2)==manifest.at("preset").at("sha256").get<std::string>(),"payload preset digest");const auto yaml=read(folder/"minimal-bellatrix.yaml");
 const ExecutionPayloadLimits limits{uint32_t(parameter(yaml,"MAX_EXTRA_DATA_BYTES")),uint32_t(parameter(yaml,"MAX_BYTES_PER_TRANSACTION")),uint32_t(parameter(yaml,"MAX_TRANSACTIONS_PER_PAYLOAD"))};size_t cases=0,bytes=0,transactions=0;
 for(const auto& row:manifest.at("cases")){const auto path=folder/row.at("name").get<std::string>();for(const auto& [file,info]:row.at("files").items()){const auto b=binary(path/file);check(b.size()==info.at("bytes")&&hex(sha256(b)).substr(2)==info.at("sha256").get<std::string>(),"payload fixture digest");}
  const auto encoded=binary(path/"serialized.ssz");const auto p=decode_bellatrix_execution_payload(encoded,limits,1024*1024);check(p.has_value(),"official payload rejected");check(encode_bellatrix_execution_payload(*p,limits,1024*1024)==encoded,"official payload roundtrip mismatch");const auto yaml=read(path/"roots.yaml");const auto root=bellatrix_execution_payload_root(*p,limits);check(hex(Bytes(root.begin(),root.end()))==yaml.substr(yaml.find("0x"),66),"official payload root mismatch");++cases;bytes+=encoded.size();transactions+=p->transactions.size();
 }
 auto encoded=binary(folder/"case_0/serialized.ssz");const auto p=*decode_bellatrix_execution_payload(encoded,limits,1024*1024);size_t refused=0;
 auto deny=[&](const Bytes& b,const ExecutionPayloadLimits& l,size_t budget){check(!decode_bellatrix_execution_payload(b,l,budget),"malformed or over-budget payload accepted");++refused;};
 for(size_t n:{size_t(0),size_t(4),size_t(435),size_t(436),size_t(503),size_t(507)})deny(Bytes(encoded.begin(),encoded.begin()+n),limits,1024*1024);
 for(size_t at:{size_t(436),size_t(437),size_t(438),size_t(439),size_t(507)}){auto b=encoded;b[at]^=0x80;deny(b,limits,1024*1024);}
 deny(encoded,limits,encoded.size()-1);auto small=limits;small.extra_data_bytes=1;if(p.extra_data.size()>1)deny(encoded,small,1024*1024);
 check(p.transactions.size()>1,"fixture must exercise transaction offsets");small=limits;small.transactions=1;deny(encoded,small,1024*1024);small=limits;small.transaction_bytes=1;deny(encoded,small,1024*1024);
 const size_t list=little(encoded,504,4);{auto b=encoded;b[list]^=1;deny(b,limits,1024*1024);} {auto b=encoded;for(unsigned i=0;i<4;++i)b[list+4+i]=0;deny(b,limits,1024*1024);}
 BellatrixExecutionPayload edge{};edge.block_number=UINT64_MAX;edge.timestamp=UINT64_MAX;edge.base_fee_per_gas.fill(0xff);edge.transactions={{},{1,2,3}};
 const auto edge_bytes=encode_bellatrix_execution_payload(edge,limits,1024);check(decode_bellatrix_execution_payload(edge_bytes,limits,1024)==edge,"uint256/uint64 or empty opaque transaction changed");
 const auto with_empty=bellatrix_execution_payload_root(edge,limits);edge.transactions.erase(edge.transactions.begin());check(bellatrix_execution_payload_root(edge,limits)!=with_empty,"transaction list length not committed");
 auto changed=p;changed.base_fee_per_gas[31]^=1;check(bellatrix_execution_payload_root(changed,limits)!=bellatrix_execution_payload_root(p,limits),"upper uint256 base fee not committed");
 size_t cancellations=0;auto abort=[&]{throw std::runtime_error("cancel payload");};
 for(unsigned op=0;op<3;++op){bool stopped=false;try{if(op==0)decode_bellatrix_execution_payload(encoded,limits,1024*1024,abort);if(op==1)encode_bellatrix_execution_payload(p,limits,1024*1024,abort);if(op==2)bellatrix_execution_payload_root(p,limits,abort);}catch(const std::runtime_error& e){stopped=std::string(e.what())=="cancel payload";}check(stopped,"payload cancellation ignored");++cancellations;}
 bool bad_limits=false;try{decode_bellatrix_execution_payload(encoded,{32,1024,UINT32_MAX},1024*1024);}catch(const std::invalid_argument&){bad_limits=true;}check(bad_limits,"impossible offset-table limit accepted");
 bool over_budget=false;try{encode_bellatrix_execution_payload(p,limits,507);}catch(const std::invalid_argument&){over_budget=true;}check(over_budget,"encode budget ignored");
 std::cout<<Json{{"scenario","bellatrix-execution-payload-ssz"},{"officialCases",cases},{"officialBytes",bytes},{"opaqueTransactions",transactions},{"malformedOrBudgetRefusals",refused},{"cancellations",cancellations},{"executionValidated",false},{"networkActivated",false}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
