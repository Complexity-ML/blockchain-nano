// Actual frozen validator.cpp, rebuilt against explicit well-constructed memory
// doubles. This exercises its control flow, NOT a CapellaNodeRuntime integration.
#include "nano/consensus_capella_validator.hpp"
#include <iostream>
#include <sys/resource.h>
namespace nano {
// Small source-compatible noncryptographic formatting doubles. No core linked.
std::string address(std::string s){if(s!="0x0000000000000000000000000000000000000000")throw std::invalid_argument("mock address");return s;}
std::string hex(const Bytes& b){static const char h[]="0123456789abcdef";std::string s="0x";for(auto v:b){s+=h[v>>4];s+=h[v&15];}return s;}
std::string quantity(const Int& n){return "0x"+n.get_str(16);}
}
using namespace nano;
int main(){try{
 auto need=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 CapellaDurableReactor reactor;CapellaLocalClock clock;uint64_t signer_calls=0;
 auto journal=std::make_shared<BeaconSigningJournal>();
 CapellaValidatorKey key{0,journal,[&](const ConsensusRoot&){++signer_calls;throw std::logic_error("unexpected provider invocation");return Bytes{};},"0x0000000000000000000000000000000000000000",{}};
 CapellaValidatorLoop loop(reactor,clock,6000,8,1,1,{key},[]{return uint64_t(384000);});
 need(loop.healthy(),"loop must start healthy");std::string first,second;
 try{loop.poll();}catch(const std::invalid_argument& e){first=e.what();}
 need(first=="empty Altair sync active set"&&!loop.healthy(),"current scheduler must latch prep refusal");
 need(reactor.prepare_calls==1&&reactor.enumerations==0&&reactor.proposer_calls==0&&reactor.signing_paths==0&&signer_calls==0,"refusal must precede keys and providers");
 reactor.replace_with_ready_head();
 try{loop.poll();}catch(const std::runtime_error& e){second=e.what();}
 need(second=="Capella validator loop unavailable"&&reactor.prepare_calls==1&&clock.polls==1,"ready head cannot unblock latched scheduler");
 rusage usage{};need(getrusage(RUSAGE_SELF,&usage)==0&&uint64_t(usage.ru_maxrss)<=134217728,"128-MiB RSS budget");
 std::cout<<Json{{"format","nano-validator-flow-counterexample-v1"},{"result","EXPECTED_CURRENT_BEHAVIOR_FAILURE_REPRODUCED"},{"actualValidatorSourceCompiled",true},{"memoryDoubles",true},{"runtimeIntegrationQualified",false},{"wellConstructedObjects",true},{"healthyBefore",true},{"healthyAfterPreparationRefusal",loop.healthy()},{"readyHeadGeneration",reactor.admitted_head_generation},{"firstException",first},{"secondException",second},{"preparationCalls",reactor.prepare_calls},{"clockPolls",clock.polls},{"keyEnumerations",reactor.enumerations},{"proposerCalls",reactor.proposer_calls},{"signingPaths",reactor.signing_paths},{"providerCalls",signer_calls},{"status",loop.status()},{"maxRssKiB",uint64_t(usage.ru_maxrss)/1024},{"databaseOpened",false},{"runtimeStarted",false},{"networkStarted",false},{"productionQualified",false}}.dump()<<std::endl;
 return 1; // preserved expected-failure program exit, collector succeeds separately.
 }catch(const std::exception& e){std::cout<<Json{{"result","HARNESS_DIAGNOSTIC_FAILURE"},{"exception",e.what()}}.dump()<<std::endl;return 2;}}
