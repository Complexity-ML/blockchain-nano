#pragma once
#include "json.hpp"
#include <algorithm>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
namespace nano::snapshot_detail {
// Internal only. The caller has already compared every SSZ source byte to
// cached[2][0] using canonical_hex_matches. Compare every other row field;
// the candidate's null SSZ slot is a deferred copy, never wire output.
inline bool same_row_after_ssz_match(const nlohmann::json& cached,const nlohmann::json& candidate){
 if(!cached.is_array()||cached.size()!=6||!candidate.is_array()||candidate.size()!=6||
    !cached[2].is_array()||cached[2].size()!=2||!cached[2][0].is_string()||
    !candidate[2].is_array()||candidate[2].size()!=2||!candidate[2][0].is_null())return false;
 for(size_t i=0;i<6;++i){
  if(i==2){if(cached[2][1]!=candidate[2][1])return false;}
  else if(cached[i]!=candidate[i])return false;
 }
 return true;
}
// Internal only: rows are dumps of the complete, validated block records in
// document["blocks"]. Never embed peer-supplied strings through this helper.
inline std::string canonical_snapshot_rows(const nlohmann::json& document,
 std::span<const std::string_view> rows,size_t maximum_bytes,const std::function<void()>& cancel={}){
 auto require=[](bool ok){if(!ok)throw std::invalid_argument("Invalid canonical snapshot rows");};
 require(document.is_object()&&document.contains("blocks")&&document.at("blocks").is_array()&&document.at("blocks").size()==rows.size());
 std::string result;
 auto append=[&](std::string_view bytes){
  require(result.size()<=maximum_bytes&&bytes.size()<=maximum_bytes-result.size());
  for(size_t offset=0;offset<bytes.size();){if(cancel)cancel();const auto count=std::min<size_t>(4096,bytes.size()-offset);result.append(bytes.substr(offset,count));offset+=count;}
 };
 append("{");bool first=true;
 for(const auto& item:document.items()){
  if(!first)append(",");first=false;append(nlohmann::json(item.key()).dump());append(":");
  if(item.key()!="blocks")append(item.value().dump());
  else {append("[");bool first_row=true;for(const auto row:rows){require(!row.empty());if(!first_row)append(",");first_row=false;append(row);}append("]");}
 }
 append("}");if(cancel)cancel();return result;
}
}
