#pragma once
#include "nano/core.hpp"
#include "consensus_capella_snapshot_hex.hpp"
namespace nano::snapshot_detail {
// Internal comparison against a privately generated, already validated cache.
// Derived storageRoot/codeHash are deterministic from the compared primitives;
// never use this helper to authenticate account JSON supplied by a peer.
inline bool validated_accounts_match(const State& state,const Json& cached,
 const std::function<void()>& cancel={}){
 if(!cached.is_object()||cached.size()!=state.accounts_view().size())return false;
 for(const auto& [address,account]:state.accounts_view()){
  if(cancel)cancel();const auto found=cached.find(address);
  if(found==cached.end()||!found->is_object()||found->size()!=6)return false;
  const auto& row=*found;
  for(const auto* field:{"nonce","balance","code","codeHash","storageRoot"})
   if(!row.contains(field)||!row[field].is_string())return false;
  if(!row.contains("storage")||!row["storage"].is_object()||
     row["storage"].size()!=account.storage.size())return false;
  if(row["nonce"].get_ref<const std::string&>()!=quantity(account.nonce)||
     row["balance"].get_ref<const std::string&>()!=quantity(account.balance)||
     !canonical_hex_matches(row["code"].get_ref<const std::string&>(),account.code,cancel))return false;
  for(const auto& [key,value]:account.storage){
   if(cancel)cancel();const auto slot=row["storage"].find(quantity(key));
   if(slot==row["storage"].end()||!slot->is_string()||
      slot->get_ref<const std::string&>()!=quantity(value))return false;
  }
 }
 if(cancel)cancel();return true;
}
}
