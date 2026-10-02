#pragma once
#include "json.hpp"
#include <algorithm>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
namespace nano::snapshot_detail {
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
