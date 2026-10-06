#pragma once
#include "types.hpp"
#include <chrono>
namespace nano {
class BlockImportLimit:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// Serialized by Rpc's mutex. Limits apply to retained payload, not process RSS.
class BlockImports {
public:
 using Clock=std::chrono::steady_clock;
 struct Limits {size_t bytes=64*1024*1024;size_t sessions=4;Clock::duration lifetime=std::chrono::seconds(60);};
private:
 struct Assembly {Json block,envelope=nullptr;std::vector<Bytes> completed;Bytes current;uint64_t total=0;size_t charged=0;Clock::time_point expires;};
 Limits limits_;size_t used_=0;std::map<std::string,Assembly> assemblies_;
 void erase(std::map<std::string,Assembly>::iterator it);
 Json progress(const std::string& id,const Assembly& value)const;
 Json begin_impl(const Json& manifest,Clock::time_point now,Json envelope);
 Json finish_impl(const std::string& id,Clock::time_point now,bool proposal);
public:
 BlockImports():BlockImports(Limits{}){}
 explicit BlockImports(Limits limits):limits_(limits){}
 void expire(Clock::time_point now);
 Json begin(const Json& manifest,Clock::time_point now);
 Json begin_proposal(const Json& envelope,Clock::time_point now);
 Json append(const std::string& id,size_t index,const Json& fragment,Clock::time_point now);
 Json finish(const std::string& id,Clock::time_point now);
 Json finish_proposal(const std::string& id,Clock::time_point now);
 bool abort(const std::string& id,Clock::time_point now);
 size_t used_bytes()const{return used_;}
 size_t sessions()const{return assemblies_.size();}
};
}
