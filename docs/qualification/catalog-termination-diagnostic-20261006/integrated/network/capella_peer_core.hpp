#pragma once
#include "altair_core_dispatch.hpp"

// Peer/session validity gates new work, not cleanup of local snapshots already
// owned by an attempt. Cleanup RPCs remain authenticated by the core's token.
inline AltairCoreDispatch::Call capella_peer_core(AltairCoreDispatch::Call core,std::function<bool()> current){
 if(!core||!current)throw std::invalid_argument("Invalid Capella peer core gate");
 return [core=std::move(core),current=std::move(current)](const std::string& method,Json params,Reply reply){
  if(!current()&&method!="nano_capellaCatalogClose"&&method!="nano_capellaCatalogRelease"){reply(nullptr);return;}
  core(method,std::move(params),std::move(reply));
 };
}
