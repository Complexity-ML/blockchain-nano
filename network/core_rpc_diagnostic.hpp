#pragma once
#include "json.hpp"
#include <functional>
#include <string>
#include <array>
#include <string_view>
using CoreRpcTrace=std::function<void(const nlohmann::json&)>;
// Observability only: never emit parameters, payloads, tokens, error data or raw text.
inline void core_rpc_trace(const CoreRpcTrace& trace,const std::string& method,const char* phase,unsigned status=0,const nlohmann::json& response=nullptr,std::string_view body={})noexcept{
 if(!trace)return;
 try{
  using J=nlohmann::json;
  static constexpr std::array safe_methods={"nano_capellaCatalogGeneration","nano_capellaCatalogAcquire","nano_capellaCatalogRelease","nano_capellaCatalogPage","nano_capellaCatalogSeek","nano_capellaCatalogClose","nano_capellaExport","nano_capellaStatus","nano_capellaSubmit","nano_capellaJob","nano_capellaCancel","nano_capellaForget","nano_capellaRotateSession","nano_capellaJobs","nano_altairStatus","nano_altairRotateSession","nano_altairJobs","nano_altairCancel","nano_altairForget"};
  bool known=false;for(const auto* allowed:safe_methods)if(method==allowed){known=true;break;}
  J record={{"method",known?method:"other"},{"phase",phase},{"httpStatus",status},{"rpcCode",nullptr},{"cause","unknown"}};
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
