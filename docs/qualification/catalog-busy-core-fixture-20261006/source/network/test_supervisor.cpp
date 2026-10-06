#include "supervisor.hpp"
#include <iostream>
#include <string>
int main(){try{
 auto require=[](bool ok){if(!ok)throw std::runtime_error("supervisor validation failed");};
 require(managed_supervisor("1",1,true)==1);require(managed_supervisor("123",123,false)==123);
 for(const auto text:{"","0","-1","+1"," 1","1x","18446744073709551616"}){bool rejected=false;try{managed_supervisor(text,1,true);}catch(const std::exception&){rejected=true;}require(rejected);}
 for(const auto parent:{0,1,124}){bool rejected=false;try{managed_supervisor("123",parent,true);}catch(const std::exception&){rejected=true;}require(rejected);}
 bool rejected=false;try{managed_supervisor("1",1,false);}catch(const std::exception&){rejected=true;}require(rejected);
 rejected=false;try{managed_supervisor(std::to_string(static_cast<unsigned long long>(std::numeric_limits<pid_t>::max())+1),1,true);}catch(const std::exception&){rejected=true;}require(rejected);
 std::cout<<"Supervisor PID 1 policy, malformed/overflow PIDs and reparenting checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
