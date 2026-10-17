#pragma once
#include "json.hpp"
#include <algorithm>
#include <string>
#include <cstddef>

// Local metadata policy; block/data frames retain their independent 16 MiB cap.
inline constexpr size_t peer_status_frame_bytes=4096;

inline bool valid_peer_status(const nlohmann::json& remote,const nlohmann::json& local){
 auto shaped=[](const nlohmann::json& status){
  if(!status.is_object()||!status.contains("chain_id")||!status.at("chain_id").is_number_unsigned()||status.at("chain_id")==0||
     !status.contains("best_block")||!status.at("best_block").is_number_unsigned()||
     !status.contains("genesis_hash")||!status.at("genesis_hash").is_string())return false;
  if(status.contains("best_hash")){
   if(!status["best_hash"].is_string())return false;
   const auto& hash=status["best_hash"].get_ref<const std::string&>();
   if(hash.size()!=66||!hash.starts_with("0x")||!std::all_of(hash.begin()+2,hash.end(),[](char ch){return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');}))return false;
  }
  if(status.contains("gossip_epoch")){
   if(!status["gossip_epoch"].is_string())return false;
   const auto& epoch=status["gossip_epoch"].get_ref<const std::string&>();
   if(epoch.size()!=34||!epoch.starts_with("0x")||!std::all_of(epoch.begin()+2,epoch.end(),[](char ch){return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');}))return false;
  }
  const auto& hash=status.at("genesis_hash").get_ref<const std::string&>();
  return hash.size()==66&&hash.starts_with("0x")&&std::all_of(hash.begin()+2,hash.end(),[](char ch){return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');});
 };
 return shaped(remote)&&shaped(local)&&remote.at("chain_id")==local.at("chain_id")&&remote.at("genesis_hash")==local.at("genesis_hash");
}

// Call only after valid_peer_status and the bounded status-frame reader.
// Unknown extensions may fit on the wire but never become retained peer state.
inline nlohmann::json project_peer_status(const nlohmann::json& status){
 auto projected=nlohmann::json::object();
 for(const auto* field:{"chain_id","network_id","best_block","best_hash","genesis_hash","cursor","gossip_epoch","version","altairIpc","bellatrixIpc","capellaIpc","altair","bellatrix","capella","capellaSession","capellaStreamProtocol","beacon_head_slot"})
  if(status.contains(field))projected[field]=status.at(field);
 return projected;
}
