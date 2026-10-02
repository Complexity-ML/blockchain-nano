#pragma once
#include "json.hpp"
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
namespace nano::checkpoint_detail {
// Internal canonical serializer: encoded_store must come from the validated
// store serializer. Never use this entry point to embed an untrusted peer value.
inline std::string durable_json(nlohmann::json document,const std::string& encoded_store,
 size_t maximum_bytes,const std::function<void()>& cancel={}){
 auto require=[](bool ok){if(!ok)throw std::invalid_argument("Invalid Bellatrix durable archive");};
 require(document.is_object()&&!encoded_store.empty());
 // The canonical object uses sorted keys. Refuse future fields that would
 // change the position of store instead of silently changing checkpoint bytes.
 for(const auto& item:document.items())require(item.key()<"store");
 auto result=document.dump();constexpr std::string_view key=",\"store\":";
 require(!document.empty()&&encoded_store.size()<=maximum_bytes&&result.size()<=maximum_bytes-encoded_store.size()&&key.size()<=maximum_bytes-encoded_store.size()-result.size());
 result.reserve(result.size()+key.size()+encoded_store.size());
 result.pop_back();result.append(key);result.append(encoded_store);result.push_back('}');
 if(cancel)cancel();return result;
}
}
