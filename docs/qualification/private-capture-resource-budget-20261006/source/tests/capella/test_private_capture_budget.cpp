#include "private_capture_budget.hpp"
#include <iostream>
using namespace nano::private_capture;
void need(bool b){if(!b)throw std::runtime_error("capture budget test failed");}
int main(){try{
 const Json legacy={{"blocks","0x400"},{"envelopeBytes","0x8000000"}};
 need(resource_budget(legacy,Json::object())==legacy);
 auto larger=legacy;larger["blocks"]="0x401";bool failed=false;try{resource_budget(larger,Json::object());}catch(const std::exception&){failed=true;}need(failed);
 const Json policy={{"blocks","0x1000"},{"envelopeBytes","0x8000000"}};need(resource_budget(larger,Json{{"captureBudget",policy}})==policy);
 auto million=policy;million["blocks"]="0xf4240";need(resource_budget(million,Json{{"captureBudget",million}})==million);
 unsigned refused=0;
 for(const Json& bad:Json::array({0,"0x0","0x0400","0X400","0xA","0x-1","0x10000000000000000","0x1 ",nullptr,"0x"})){
  for(const auto* field:{"blocks","envelopeBytes"}){auto p=policy;p[field]=bad;failed=false;try{resource_budget(legacy,Json{{"captureBudget",p}});}catch(const std::exception&){failed=true;}need(failed);++refused;}
 }
 for(const Json& p:Json::array({Json{{"blocks","0x400"}},Json{{"blocks","0x400"},{"envelopeBytes","0x8000000"},{"consensus",true}},Json{{"blocks","0x8000001"},{"envelopeBytes","0x8000000"}},Json{{"blocks","0x400"},{"envelopeBytes","0xffffffffffffffff"}},Json{{"blocks","0x3ff"},{"envelopeBytes","0x8000000"}},Json{{"blocks","0x400"},{"envelopeBytes","0x7ffffff"}}})){
  failed=false;try{resource_budget(legacy,Json{{"captureBudget",p}});}catch(const std::exception&){failed=true;}need(failed);++refused;
 }
 std::cout<<Json{{"scenario","private-capture-resource-budget"},{"legacyPreserved",true},{"explicitBeyond1024Accepted",true},{"millionBudgetParsedWithoutAllocation",true},{"refusals",refused},{"networkQualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
