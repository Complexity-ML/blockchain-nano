#include "nano/rpc.hpp"
namespace nano {
namespace {
void validate_filter(const Json& filter){
 if(!filter.is_object())throw RpcError(-32602,"Expected log filter object");
 if(filter.contains("blockHash")){
  if(filter.contains("fromBlock")||filter.contains("toBlock"))throw RpcError(-32602,"blockHash excludes fromBlock/toBlock");
  if(unhex(filter["blockHash"].get<std::string>()).size()!=32)throw RpcError(-32602,"Invalid block hash");
 }
 for(auto name:{"fromBlock","toBlock"})if(filter.contains(name)){
  auto tag=filter[name].get<std::string>();if(tag!="earliest"&&tag!="latest")quantity(filter[name]);
 }
 if(filter.contains("address")){const auto& a=filter["address"];if(a.is_array()){for(const auto& addr:a)address(addr.get<std::string>());}else address(a.get<std::string>());}
 if(filter.contains("topics")){
  const auto& topics=filter["topics"];if(!topics.is_array()||topics.size()>4)throw RpcError(-32602,"Invalid topic filter");
  auto check=[](const Json& t){if(!t.is_null()&&unhex(t.get<std::string>()).size()!=32)throw RpcError(-32602,"Topic must be 32 bytes");};
  for(const auto& topic:topics){if(topic.is_array())for(const auto& alternative:topic)check(alternative);else check(topic);}
 }
}
bool matches(const Json& filter,const Log& log){
 if(filter.contains("address")){auto target=address(log.address);const auto& a=filter["address"];bool match=false;if(a.is_array()){for(const auto& addr:a)if(address(addr.get<std::string>())==target)match=true;}else match=address(a.get<std::string>())==target;if(!match)return false;}
 if(filter.contains("topics")){const auto& topics=filter["topics"];if(topics.size()>log.topics.size())return false;
  for(size_t i=0;i<topics.size();++i){auto equal=[&](const Json& value){return value.is_null()||unhex(value.get<std::string>())==log.topics[i];};bool match=false;const auto& t=topics[i];if(t.is_array()){for(const auto& value:t)if(equal(value))match=true;}else match=equal(t);if(!match)return false;}
 }
 return true;
}
}
Json Rpc::logs(const Json& criteria,const std::optional<Int>& only)const{
 validate_filter(criteria);Json result=Json::array();auto tip=chain_.tip();Int first=tip.header.number,last=tip.header.number;
 if(criteria.contains("blockHash")){auto b=chain_.store().block_by_hash(unhex(criteria["blockHash"].get<std::string>()));if(!b)return result;first=last=b->header.number;}
 else{
  auto number=[&](const Json& tag)->Int{if(tag=="latest")return tip.header.number;if(tag=="earliest")return 0;return quantity(tag);};
  first=number(criteria.value("fromBlock",Json("latest")));last=number(criteria.value("toBlock",Json("latest")));last=std::min(last,tip.header.number);
 }
 if(only){if(*only<first||*only>last)return result;first=last=*only;}
 if(last<first)return result;if(last-first>=10000)throw RpcError(-32005,"Log range exceeds 10000 blocks");
 for(Int number=first;number<=last;++number){auto b=chain_.store().block_by_number(number);if(!b)throw RpcError(-32000,"Missing block in log range");size_t index=0;
  for(size_t txindex=0;txindex<b->transactions.size();++txindex){auto txhash=b->transactions[txindex].hash();auto receipt=chain_.store().receipt(txhash);if(!receipt)throw RpcError(-32000,"Receipt unavailable");
   for(const auto& log:receipt->logs){if(matches(criteria,log)){
    Json item=log.to_json();item["blockHash"]=hex(b->header.hash());item["blockNumber"]=quantity(number);item["transactionHash"]=hex(txhash);item["transactionIndex"]=quantity(Int(txindex));item["logIndex"]=quantity(Int(index));item["removed"]=false;result.push_back(std::move(item));if(result.size()>10000)throw RpcError(-32005,"Log results exceed 10000 entries");
   }++index;}
  }
 }
 return result;
}
RpcStream Rpc::stream(const std::string& type,const Json& criteria){
 std::lock_guard lock(mutex_);if(type!="newHeads"&&type!="newPendingTransactions"&&type!="logs")throw RpcError(-32602,"Unknown subscription type");
 if(type=="logs")validate_filter(criteria);
 auto cursor=chain_.event_cursor();Json filter=criteria;
 if(type=="logs"&&!filter.contains("blockHash")&&(!filter.contains("fromBlock")||filter["fromBlock"]=="latest"))filter["fromBlock"]=quantity(chain_.tip().header.number);
 return {quantity(Int(std::to_string(++next_filter_))),type,std::move(filter),cursor};
}
Json Rpc::poll(RpcStream& stream){
 std::lock_guard lock(mutex_);std::vector<ChainEvent> events;
 try{events=chain_.events_since(stream.cursor);}catch(const std::exception&){throw RpcError(-32000,"Filter event history expired; create a new filter");}
 Json result=Json::array();
 for(const auto& event:events){
  if(event.type=="reset")throw RpcError(-32000,"Chain restored; create a new subscription");
  if(event.type=="pending"&&stream.type=="newPendingTransactions")result.push_back(event.value);
  if(event.type=="block"){
   if(stream.type=="newHeads")result.push_back(event.value);
   else if(stream.type=="logs"){for(auto& log:logs(stream.criteria,quantity(event.value.at("number"))))result.push_back(std::move(log));}
  }
  if(result.size()>10000)throw RpcError(-32005,"Filter results exceed 10000 entries");
 }
 if(!events.empty())stream.cursor=events.back().sequence;return result;
}
}
