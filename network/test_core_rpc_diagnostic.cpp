#include "core_rpc_diagnostic.hpp"
#include <iostream>
#include <stdexcept>
using J=nlohmann::json;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
 unsigned calls=0;J seen;CoreRpcTrace trace=[&](const J& x){++calls;seen=x;};
 J error={{"error",{{"code",-32602},{"message","Capella inventory request retired"},{"data","PRIVATE_PAYLOAD"}}}};
 core_rpc_trace(trace,"nano_capellaCatalogAcquire","rpc-error",200,error);
 check(calls==1&&seen["cause"]=="inventory-request-retired"&&seen["rpcCode"]==-32602,"retirement cause lost");
 check(seen.dump().find("PRIVATE_PAYLOAD")==std::string::npos,"error data leaked");
 error["error"]["message"]="PRIVATE_TOKEN";core_rpc_trace(trace,"PRIVATE_METHOD","rpc-error",200,error);
 check(seen["method"]=="other"&&seen["cause"]=="unknown"&&seen.dump().find("PRIVATE_")==std::string::npos,"unknown text leaked");
 core_rpc_trace(trace,"nano_capellaCatalogPage","malformed-response",200,J::array());check(seen["rpcCode"].is_null(),"malformed response acquired code");
 error["error"]["code"]="PRIVATE_TOKEN";core_rpc_trace(trace,"nano_capellaCatalogPage","rpc-error",200,error);check(seen["rpcCode"].is_null(),"non-numeric error code leaked");
 core_rpc_trace([](const J&){throw std::runtime_error("observer failed");},"nano_capellaStatus","read-error");
 core_rpc_trace({},"nano_capellaStatus","read-error");
 core_rpc_trace(trace,"nano_capellaStatus","http-error",503,nullptr,"RPC simulation capacity exceeded");check(seen["cause"]=="rpc-capacity-exceeded","capacity refusal lost");
 core_rpc_trace(trace,"nano_capellaStatus","http-error",503,nullptr,"RPC simulation queue wait exceeded");check(seen["cause"]=="rpc-queue-expired","queue expiry lost");
 core_rpc_trace(trace,"nano_capellaStatus","http-error",503,nullptr,"PRIVATE_HTTP_PAYLOAD");check(seen["cause"]=="unknown"&&seen.dump().find("PRIVATE_")==std::string::npos,"HTTP body leaked");
 core_rpc_trace(trace,"nano_capellaStatus","rpc-error",200,nullptr,"RPC simulation capacity exceeded");check(seen["cause"]=="unknown","unrelated status forged capacity");
 std::cout<<"10 diagnostic classification/redaction/observer cases passed; no transport or WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
