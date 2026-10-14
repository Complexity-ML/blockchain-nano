#pragma once
#include "json.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <string>
#include <array>
#include <string_view>
using CoreRpcTrace=std::function<void(const nlohmann::json&)>;
// Copies handed to pooled/concurrent HTTP connections share one process-local
// allowance. Saturation avoids wraparound and observer failures cannot reset it.
inline CoreRpcTrace bounded_core_rpc_trace(CoreRpcTrace observer,size_t maximum){
 if(!observer||!maximum)return {};
 auto used=std::make_shared<std::atomic<size_t>>(0);
 return [used,maximum,observer=std::move(observer)](const nlohmann::json& row){
  auto count=used->load(std::memory_order_relaxed);
  while(count<maximum){
   if(used->compare_exchange_weak(count,count+1,std::memory_order_relaxed)){
    try{observer(row);}catch(...){/* Diagnostics never affect RPC completion. */}
    return;
   }
  }
 };
}
// Only fixed category labels for these local inventory methods are observable.
// Authentication tokens, roots, handles and arbitrary parameters never escape.
inline std::string core_rpc_inventory_kind(const std::string& method,const nlohmann::json& params){
 if(method!="nano_capellaCatalogAcquire"&&method!="nano_capellaCatalogGeneration"&&method!="nano_capellaExport")return {};
 if(!params.is_array()||params.size()<2||!params[1].is_string())return {};
 const auto& kind=params[1].get_ref<const std::string&>();
 for(const char* allowed:{"block","attestation","attester_slashing","voluntary_exit","bls_to_execution_change","sync_committee_message","proposer_slashing"})if(kind==allowed)return kind;
 return {};
}
// Observability only: never emit parameters, payloads, tokens, error data or raw text.
inline void core_rpc_trace(const CoreRpcTrace& trace,const std::string& method,const char* phase,unsigned status=0,const nlohmann::json& response=nullptr,std::string_view body={},std::string_view inventory_kind={})noexcept{
 if(!trace)return;
 try{
  using J=nlohmann::json;
  static constexpr std::array safe_methods={"nano_capellaCatalogGeneration","nano_capellaCatalogAcquire","nano_capellaCatalogRelease","nano_capellaCatalogPage","nano_capellaCatalogSeek","nano_capellaCatalogClose","nano_capellaExport","nano_capellaStatus","nano_capellaSubmit","nano_capellaJob","nano_capellaCancel","nano_capellaForget","nano_capellaRotateSession","nano_capellaJobs","nano_altairStatus","nano_altairRotateSession","nano_altairJobs","nano_altairCancel","nano_altairForget"};
  bool known=false;for(const auto* allowed:safe_methods)if(method==allowed){known=true;break;}
  J record={{"method",known?method:"other"},{"phase",phase},{"httpStatus",status},{"rpcCode",nullptr},{"cause","unknown"}};
  record["inventoryKind"]=nullptr;
  for(const char* allowed:{"block","attestation","attester_slashing","voluntary_exit","bls_to_execution_change","sync_committee_message","proposer_slashing"})if(inventory_kind==allowed){record["inventoryKind"]=allowed;break;}
  if(response.is_object()&&response.contains("error")&&response["error"].is_object()){
   const auto& error=response["error"];
   if(error.contains("code")&&error["code"].is_number_integer())record["rpcCode"]=error["code"];
   if(error.contains("message")&&error["message"].is_string()){
    const auto& message=error["message"].get_ref<const std::string&>();
    if(message=="Capella inventory capture busy")record["cause"]="inventory-capture-busy";
    else if(message=="Capella inventory request retired")record["cause"]="inventory-request-retired";
    else if(message=="Capella inventory handle unavailable")record["cause"]="inventory-handle-unavailable";
    else if(message=="Invalid params")record["cause"]="invalid-params";
    else if(message=="Internal error")record["cause"]="internal-error";
   }
  }
  if(status==503){
   if(body=="RPC simulation capacity exceeded")record["cause"]="rpc-capacity-exceeded";
   else if(body=="RPC simulation queue wait exceeded")record["cause"]="rpc-queue-expired";
  }
  trace(record);
 }catch(...){/* A broken observer must not alter completion or cause retries. */}
}
