#pragma once
#include "json.hpp"
#include <openssl/sha.h>
#include <array>
#include <chrono>
#include <map>
#include <set>
#include <string>
// Transport knowledge only: an exact vote received from a peer need not be
// echoed back immediately. This never grants voting weight or skips core checks.
class BftVoteKnowledge {
 using Json=nlohmann::json;
 using Digest=std::array<unsigned char,SHA256_DIGEST_LENGTH>;
 struct Context {std::set<Digest> seen;std::chrono::steady_clock::time_point refresh{};uint64_t touched=0;};
 std::map<std::string,Context> contexts_;uint64_t sequence_=0;
 static Digest digest(const std::string& text){Digest out;SHA256(reinterpret_cast<const unsigned char*>(text.data()),text.size(),out.data());return out;}
 static std::string key(const Json& page){
  if(!page.is_object()||page.value("version",Json())!=1||!page.contains("votes")||!page["votes"].is_array()||page["votes"].size()>2000)return {};
  std::string out;for(const char* field:{"height","round"}){if(!page.contains(field)||!page[field].is_string())return {};auto s=page[field].get<std::string>();if(s.size()<3||s.size()>18||s.substr(0,2)!="0x"||s.find_first_not_of("0123456789abcdef",2)!=std::string::npos||(s.size()>3&&s[2]=='0'))return {};out+=s+":";}
  for(const auto& v:page["votes"])if(!v.is_string()||v.get_ref<const std::string&>().size()>2048)return {};
  return out;
 }
 Context& context(const std::string& key){
  if(!contexts_.contains(key)&&contexts_.size()==2){auto old=contexts_.begin();for(auto it=contexts_.begin();it!=contexts_.end();++it)if(it->second.touched<old->second.touched)old=it;contexts_.erase(old);}
  auto& value=contexts_[key];value.touched=++sequence_;return value;
 }
public:
 using Clock=std::chrono::steady_clock;
 struct Receipt {std::string context;std::vector<Digest> votes;};
 Receipt receipt(const Json& sent)const{Receipt r;r.context=key(sent);if(!r.context.empty())for(const auto& v:sent["votes"])r.votes.push_back(digest(v.get_ref<const std::string&>()));return r;}
 bool acknowledge(const Receipt& sent,const Json& reply){
  if(sent.context.empty()||key(reply)!=sent.context)return false;
  auto& c=context(sent.context);for(const auto& vote:sent.votes){if(c.seen.size()>=2000)break;c.seen.insert(vote);}return true;
 }

 void observe(const Json& remote){auto k=key(remote);if(k.empty())return;auto& c=context(k);for(const auto& v:remote["votes"]){if(c.seen.size()>=2000)break;c.seen.insert(digest(v.get_ref<const std::string&>()));}}
 Json filter(Json local,Clock::time_point now=Clock::now()){
  auto k=key(local);if(k.empty())return local;auto& c=context(k);
  // Never rely indefinitely on remembered remote state: core restart/session
  // detection can lag, and an older peer can silently lose volatile votes.
  if(c.refresh==Clock::time_point{}||now>=c.refresh){c.refresh=now+std::chrono::seconds(5);return local;}
  Json votes=Json::array();for(const auto& v:local["votes"])if(!c.seen.contains(digest(v.get_ref<const std::string&>())))votes.push_back(v);local["votes"]=std::move(votes);return local;
 }
 size_t retained()const{size_t n=0;for(const auto& [_,c]:contexts_)n+=c.seen.size();return n;}
};
