#pragma once
#include "peer_status.hpp"
inline const std::string altair_protocol="/nano/altair/1.0.0";
// Advertisement only filters compatibility. Every SSZ message still requires
// independent consensus/BLS validation in the core.
inline bool compatible_altair_peer(const nlohmann::json& remote,const nlohmann::json& local){
 if(!valid_peer_status(remote,local))return false;
 auto shaped=[](const nlohmann::json& status){
  if(!status.contains("altair")||!status.at("altair").is_object())return false;const auto& a=status.at("altair");
  if(a.size()!=3||a.value("protocol",nlohmann::json())!=altair_protocol)return false;
  for(const auto& [key,length]:{std::pair{"consensusIdentity",size_t(66)},std::pair{"forkDigest",size_t(10)}}){
   if(!a.contains(key)||!a.at(key).is_string())return false;const auto& s=a.at(key).get_ref<const std::string&>();
   if(s.size()!=length||!s.starts_with("0x")||!std::all_of(s.begin()+2,s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))return false;
  }return true;
 };
 return shaped(remote)&&shaped(local)&&remote.at("altair")==local.at("altair");
}
