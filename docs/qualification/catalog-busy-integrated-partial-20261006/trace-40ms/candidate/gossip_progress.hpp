#pragma once
#include "json.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <memory>
#include <chrono>

struct GossipProgress {
 using Clock=std::chrono::steady_clock;
 Clock::time_point inventory_due=Clock::now()+std::chrono::seconds(60);
 uint64_t cursor=0;
 bool busy=false;
 std::string epoch,after;bool snapshot=false,periodic=false;
 std::optional<uint64_t> failed_cursor;
 bool refresh_inventory(Clock::time_point now=Clock::now()){
  if(busy||snapshot||failed_cursor||now<inventory_due)return false;
  cursor=0;epoch.clear();after.clear();snapshot=true;periodic=true;
  inventory_due=now+std::chrono::seconds(60);return true;
 }
 void inventory_completed(Clock::time_point now=Clock::now()){inventory_due=now+std::chrono::seconds(60);}
 bool acknowledge(const nlohmann::json& reply,uint64_t next,size_t count,bool inventory=false){
  bool valid=(inventory||next>=cursor)&&reply.is_object()&&reply.value("cursor",nlohmann::json())==nlohmann::json(next)&&reply.value("processed",nlohmann::json())==nlohmann::json(count)&&reply.contains("accepted")&&reply["accepted"].is_array()&&reply["accepted"].size()==count&&std::all_of(reply["accepted"].begin(),reply["accepted"].end(),[](const nlohmann::json& v){return v.is_boolean();});
  if(!valid){if(!failed_cursor)failed_cursor=next;return false;}
  cursor=next;failed_cursor.reset();return true;
 }
};

// Replace the state object so callbacks from a preceding remote session cannot
// advance the new session's inventory. The caller validates status first.
inline void observe_gossip_session(std::shared_ptr<GossipProgress>& progress,
                                  const nlohmann::json& old_status,
                                  const nlohmann::json& new_status){
 if(!progress||old_status.value("gossip_epoch",nlohmann::json())!=new_status.value("gossip_epoch",nlohmann::json()))
  progress=std::make_shared<GossipProgress>();
}
