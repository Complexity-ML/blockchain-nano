#include "../src/consensus_capella_snapshot_json.hpp"
#include "../src/consensus_capella_snapshot_hex.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
using Json=nlohmann::json;
using nano::snapshot_detail::canonical_snapshot_rows;
int main(){try{
 auto check=[](bool ok){if(!ok)throw std::runtime_error("canonical snapshot rows mismatch");};
 std::mt19937_64 random(0x4e414e4f);size_t comparisons=0,bounds=0,cancellations=0;
 for(unsigned schema=1;schema<=3;++schema)for(unsigned sample=0;sample<128;++sample){
  Json blocks=Json::array();std::vector<std::string> owned;
  for(unsigned i=0;i<sample%11;++i){blocks.push_back(Json::array({random(),Json{{"ssz",std::string((sample%16)*4097,'a')},{"text","é 🐝 \"\\\n"}},Json::array({nullptr,true,false,-17,1.25})}));owned.push_back(blocks.back().dump());}
  std::vector<std::string_view> rows;for(const auto& row:owned)rows.push_back(row);
  Json document={{"format","nano-capella-store-v"+std::to_string(schema)},{"blocks",blocks},{"ancestors",Json::array()},{"clock",Json::array({"0",std::to_string(random())})},{"nested",Json{{"blocks",Json::array()},{"escape","\"blocks\":[]"}}},{"z",UINT64_MAX}};
  if(schema>=2)document["networkAnchor"]="0x0123";if(schema>=3)document["depositAncestors"]=Json::array({"0xdead"});
  const auto expected=document.dump();
  check(canonical_snapshot_rows(document,rows,expected.size())==expected);check(canonical_snapshot_rows(document,rows,SIZE_MAX)==expected);comparisons+=2;
  auto skeleton=document;skeleton["blocks"]=Json::array();for(size_t i=0;i<rows.size();++i)skeleton["blocks"].push_back(nullptr);
  check(canonical_snapshot_rows(skeleton,rows,expected.size())==expected);++comparisons;
  for(size_t bound:{size_t(0),expected.size()-1}){bool refused=false;try{canonical_snapshot_rows(document,rows,bound);}catch(const std::invalid_argument&){refused=true;}check(refused);++bounds;}
  bool stopped=false;size_t callbacks=0;try{canonical_snapshot_rows(document,rows,SIZE_MAX,[&]{if(++callbacks==2)throw std::runtime_error("cancel");});}catch(const std::runtime_error&){stopped=true;}check(stopped&&callbacks==2);++cancellations;
 }
 size_t rowComparisons=0,sszRefusals=0;
 for(unsigned sample=0;sample<256;++sample){
  std::vector<uint8_t> bytes(8+sample%97);std::string encoded="0x";constexpr char digits[]="0123456789abcdef";
  for(auto& b:bytes){b=uint8_t(random());encoded+=digits[b>>4];encoded+=digits[b&15];}
  Json full=Json::array({std::to_string(random()),Json::array({sample,"parent","state","body"}),Json::array({encoded,sample}),"realized","unrealized",Json{{"receipts",Json::array({sample})},{"state","execution"}}});
  auto deferred=full;deferred[2][0]=nullptr;
  auto matches=[&](const Json& cached,const Json& candidate){return cached.is_array()&&cached.size()==6&&cached[2].is_array()&&cached[2].size()==2&&cached[2][0].is_string()&&nano::snapshot_detail::canonical_hex_matches(cached[2][0].get_ref<const std::string&>(),bytes)&&nano::snapshot_detail::same_row_after_ssz_match(cached,candidate);};
  check(matches(full,deferred));++rowComparisons;
  for(unsigned field=0;field<6;++field){auto changed=deferred;auto materialized=full;
   if(field==2){changed[2][1]="different-epoch";materialized[2][1]="different-epoch";}
   else {changed[field]=Json::object({{"mutation",field}});materialized[field]=changed[field];}
   check(matches(full,changed)==(full==materialized));++rowComparisons;
  }
  for(auto bad:{Json::array(),Json::object(),Json::array({1,2,3,4,5,6})}){check(!matches(bad,deferred));++rowComparisons;}
  bytes[0]^=1;check(!matches(full,deferred));++sszRefusals;
 }
 for(const auto& value:{Json::array(),Json::object(),Json{{"blocks",false}},Json{{"blocks",Json::array({1})}}}){bool refused=false;try{canonical_snapshot_rows(value,{},SIZE_MAX);}catch(const std::invalid_argument&){refused=true;}check(refused);}
 Json row=Json::array({std::string(65536,'a'),Json{{"escape","é 🐝 \"\\\n"}}});const auto dump=row.dump();std::vector<std::string_view> rows(32,dump);Json document={{"blocks",Json::array()},{"format","nano-capella-store-v3"}};for(unsigned i=0;i<32;++i)document["blocks"].push_back(row);
 const auto expected=document.dump();check(canonical_snapshot_rows(document,rows,expected.size())==expected);
 std::vector<double> legacy,optimized;size_t consumed=0;
 auto measure=[&](bool reuse){const auto begin=std::chrono::steady_clock::now();for(unsigned i=0;i<12;++i){const auto text=reuse?canonical_snapshot_rows(document,rows,expected.size()):document.dump();check(text==expected);consumed+=text.size();}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();};
 for(unsigned i=0;i<5;++i){if(i%2){optimized.push_back(measure(true));legacy.push_back(measure(false));}else{legacy.push_back(measure(false));optimized.push_back(measure(true));}}
 std::cout<<Json{{"comparisons",comparisons+1},{"rowComparisonChecks",rowComparisons},{"sszMutationRefusals",sszRefusals},{"envelopeRefusals",bounds},{"cancellations",cancellations},{"legacyMilliseconds",legacy},{"cachedRowsMilliseconds",optimized},{"bytesConsumed",consumed},{"scope","canonical JSON assembly only; no node speed extrapolation"},{"passed",true}}.dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
