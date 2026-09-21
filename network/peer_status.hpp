#pragma once
#include "json.hpp"
#include <algorithm>
#include <string>

inline bool valid_peer_status(const nlohmann::json& remote,const nlohmann::json& local){
 auto shaped=[](const nlohmann::json& status){
  if(!status.is_object()||!status.contains("chain_id")||!status.at("chain_id").is_number_unsigned()||status.at("chain_id")==0||
     !status.contains("best_block")||!status.at("best_block").is_number_unsigned()||
     !status.contains("genesis_hash")||!status.at("genesis_hash").is_string())return false;
  const auto& hash=status.at("genesis_hash").get_ref<const std::string&>();
  return hash.size()==66&&hash.starts_with("0x")&&std::all_of(hash.begin()+2,hash.end(),[](char ch){return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');});
 };
 return shaped(remote)&&shaped(local)&&remote.at("chain_id")==local.at("chain_id")&&remote.at("genesis_hash")==local.at("genesis_hash");
}
