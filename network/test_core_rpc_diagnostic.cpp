#include "core_rpc_diagnostic.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using J=nlohmann::json;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
 unsigned calls=0;J seen;CoreRpcTrace trace=[&](const J& x){++calls;seen=x;};
 J error={{"error",{{"code",-32602},{"message","Capella inventory request retired"},{"data","PRIVATE_PAYLOAD"}}}};
 core_rpc_trace(trace,"nano_capellaCatalogAcquire","rpc-error",200,error);
 check(calls==1&&seen["cause"]=="inventory-request-retired"&&seen["rpcCode"]==-32602,"retirement cause lost");
 check(seen.dump().find("PRIVATE_PAYLOAD")==std::string::npos,"error data leaked");
 error["error"]["message"]="Capella inventory capture busy";error["error"]["code"]=-32005;core_rpc_trace(trace,"nano_capellaCatalogAcquire","rpc-error",200,error);check(seen["cause"]=="inventory-capture-busy"&&seen["rpcCode"]==-32005&&seen.dump().find("PRIVATE_PAYLOAD")==std::string::npos,"busy cause lost or leaked data");
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
 for(const char* kind:{"block","attestation","attester_slashing","voluntary_exit","bls_to_execution_change","sync_committee_message","proposer_slashing"}){
  const auto label=core_rpc_inventory_kind("nano_capellaCatalogAcquire",J::array({"PRIVATE_TOKEN",kind}));check(label==kind,"safe inventory label lost");
  core_rpc_trace(trace,"nano_capellaCatalogAcquire","rpc-error",200,error,{},label);check(seen["inventoryKind"]==kind&&seen.dump().find("PRIVATE_")==std::string::npos,"inventory diagnostic leaked parameters");
 }
 check(core_rpc_inventory_kind("nano_capellaCatalogPage",J::array({"PRIVATE_TOKEN","block"})).empty(),"catalog handle treated as category");
 for(const auto& params:{J::array({"PRIVATE_TOKEN","PRIVATE_KIND"}),J::array({"PRIVATE_TOKEN",J::object()}),J::object(),J::array()})check(core_rpc_inventory_kind("nano_capellaCatalogAcquire",params).empty(),"arbitrary parameter escaped inventory allowlist");
 core_rpc_trace(trace,"nano_capellaCatalogAcquire","rpc-error",200,error,{},"PRIVATE_KIND");check(seen["inventoryKind"].is_null()&&seen.dump().find("PRIVATE_")==std::string::npos,"direct invalid category leaked");
 unsigned bounded_calls=0;auto bounded=bounded_core_rpc_trace([&](const J&){++bounded_calls;},64);
 // Each request receives a fresh callback copy, matching Core's connection pool.
 for(unsigned i=0;i<1000;++i){auto connection=bounded;core_rpc_trace(connection,"nano_capellaStatus","read-error");}
 check(bounded_calls==64,"copied connections reset process trace allowance");
 auto retained=bounded;bounded={};for(unsigned i=0;i<1000;++i)retained(J::object());
 check(bounded_calls==64,"saturated retained copy emitted more diagnostics");
 std::atomic<unsigned> concurrent_calls=0;auto concurrent=bounded_core_rpc_trace([&](const J&){++concurrent_calls;},64);
 std::vector<std::thread> workers;for(unsigned n=0;n<8;++n)workers.emplace_back([observer=concurrent]{for(unsigned i=0;i<1000;++i){auto copy=observer;copy(J::object());}});
 for(auto& worker:workers)worker.join();check(concurrent_calls==64,"concurrent copies exceeded allowance");
 unsigned throws=0;auto broken=bounded_core_rpc_trace([&](const J&){++throws;throw std::runtime_error("observer failure");},2);
 for(unsigned n=0;n<100;++n){auto copy=broken;copy(J::object());}check(throws==2,"observer failure reset allowance or escaped");
 check(!bounded_core_rpc_trace({},64)&&!bounded_core_rpc_trace(trace,0),"disabled diagnostics allocated an active observer");
 std::cout<<"Diagnostic classification/redaction and shared bounded observer cases passed; no transport or WAN qualification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
