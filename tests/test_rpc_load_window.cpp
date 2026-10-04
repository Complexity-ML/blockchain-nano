#include "support/rpc_load_window.hpp"
#include <iostream>
using namespace std::chrono_literals;
void require(bool value){if(!value)throw std::runtime_error("bounded RPC window control failed");}
int main(){
 // A fast client may finish its old three rounds before finality is observed.
 require(continue_rpc_load(3,3,true,false,1s,45s));
 require(!continue_rpc_load(3,3,true,true,2s,45s));
 // Early observer completion cannot omit the original minimum workload.
 require(continue_rpc_load(2,3,true,true,2s,45s));
 // Twenty-four fast rounds alone do not establish sustained observations.
 require(continue_rpc_load(24,24,true,false,9s,80s));
 require(!continue_rpc_load(90,24,true,true,30s,80s));
 // A stuck observer never permits infinite traffic, even on a very fast CPU.
 for(auto elapsed:{80s,81s}){bool failed=false;try{continue_rpc_load(24,24,true,false,elapsed,80s);}catch(const std::runtime_error&){failed=true;}require(failed);}
 bool capped=false;try{continue_rpc_load(1024,24,true,false,1s,80s);}catch(const std::runtime_error&){capped=true;}require(capped);
 require(!continue_rpc_load(1024,24,true,true,1s,80s));
 // Existing many-client/overload scenarios retain exact round counts.
 require(continue_rpc_load(11,12,false,true,90s,80s));
 require(!continue_rpc_load(12,12,false,false,1s,80s));
 std::cout<<"11 bounded RPC workload controls passed\n";
}
