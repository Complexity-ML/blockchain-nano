#pragma once
#include "json.hpp"
#include <charconv>
#include <cstdint>
#include <limits>
#include <stdexcept>
namespace nano::private_capture {
using Json=nlohmann::json;
inline uint64_t positive_quantity(const Json& j){
 if(!j.is_string())throw std::invalid_argument("capture budget requires canonical hex quantities");
 const auto& s=j.get_ref<const std::string&>();
 if(s.size()<3||s[0]!='0'||s[1]!='x'||s[2]=='0')throw std::invalid_argument("capture budget requires positive canonical hex");
 for(size_t n=2;n<s.size();++n)if(!((s[n]>='0'&&s[n]<='9')||(s[n]>='a'&&s[n]<='f')))throw std::invalid_argument("capture budget hex alphabet");
 uint64_t result=0;const auto p=std::from_chars(s.data()+2,s.data()+s.size(),result,16);
 if(p.ec!=std::errc{}||p.ptr!=s.data()+s.size()||!result||result>std::numeric_limits<size_t>::max())throw std::invalid_argument("capture budget size overflow");
 return result;
}
// Local read-only fixture tooling, never a validator or consensus limit.
// Old requests retain their historical policy; explicit budgets replace it.
inline Json resource_budget(const Json& limits,const Json& request){
 const Json policy=request.contains("captureBudget")?request.at("captureBudget"):Json{{"blocks","0x400"},{"envelopeBytes","0x8000000"}};
 if(!policy.is_object()||policy.size()!=2||!policy.contains("blocks")||!policy.contains("envelopeBytes"))throw std::invalid_argument("capture budget fields");
 const auto blocks=positive_quantity(policy.at("blocks")),bytes=positive_quantity(policy.at("envelopeBytes"));
 if(bytes>(std::numeric_limits<size_t>::max()-2048)/8||blocks>bytes)throw std::invalid_argument("capture witness/resource budget overflow");
 if(positive_quantity(limits.at("blocks"))>blocks||positive_quantity(limits.at("envelopeBytes"))>bytes)throw std::invalid_argument("private capture budget exceeded");
 return policy;
}
}
