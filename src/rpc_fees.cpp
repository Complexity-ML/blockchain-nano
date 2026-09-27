#include "nano/rpc.hpp"
#include <algorithm>
#include <cmath>

namespace nano {
RpcFeePolicy RpcFeePolicy::from_json(const Json& config){
 if(!config.is_object())throw std::invalid_argument("fee_policy must be an object");
 for(auto it=config.begin();it!=config.end();++it)
  if(it.key()!="history_blocks"&&it.key()!="samples_per_block"&&it.key()!="percentile"&&it.key()!="fallback_tip"&&it.key()!="maximum_tip")throw std::invalid_argument("Unknown fee_policy setting: "+it.key());
 RpcFeePolicy result;
 auto bounded=[&](const char* key,size_t fallback,size_t low,size_t high){
  if(!config.contains(key))return fallback;
  if(!config[key].is_number_integer())throw std::invalid_argument(std::string("fee_policy ")+key+" must be an integer");
  const Int value(config[key].dump());
  if(value<Int(low)||value>Int(high))throw std::invalid_argument(std::string("fee_policy ")+key+" out of range");
  return size_t(u64(value));
 };
 result.history_blocks=bounded("history_blocks",result.history_blocks,1,1024);
 result.samples_per_block=bounded("samples_per_block",result.samples_per_block,1,100);
 result.percentile=unsigned(bounded("percentile",result.percentile,0,100));
 const Int maximum=(Int(1)<<256)-1;
 auto price=[&](const char* key){Int value=quantity(config.at(key));if(value<0||value>maximum)throw std::invalid_argument(std::string("fee_policy ")+key+" must fit uint256");return value;};
 if(config.contains("fallback_tip"))result.fallback_tip=price("fallback_tip");
 if(config.contains("maximum_tip"))result.maximum_tip=price("maximum_tip");
 if(result.maximum_tip&&result.fallback_tip>*result.maximum_tip)throw std::invalid_argument("fee_policy fallback_tip exceeds maximum_tip");
 return result;
}

Int Rpc::suggested_tip(const Header& head){
 const auto hash=head.hash();
 if(fee_head_&&*fee_head_==hash)return fee_tip_;
 std::vector<Int> samples;
 Block current;current.header=head;
 if(head.number>0){
  auto saved=chain_.store().block_by_hash(hash);
  if(!saved)throw RpcError(-32000,"Fee estimation head unavailable; retry");
  current=std::move(*saved);
 }
 for(size_t height=0;height<fee_policy_.history_blocks&&current.header.number>0;++height){
  const Int base=current.header.base_fee.value_or(Int(0));
  std::vector<std::pair<Int,size_t>> ordered;
  ordered.reserve(current.transactions.size());
  for(size_t i=0;i<current.transactions.size();++i)
   ordered.emplace_back(std::max(Int(0),Int(current.transactions[i].tx.effective_gas_price(base)-base)),i);
  std::sort(ordered.begin(),ordered.end());
  size_t added=0;
  for(const auto& [tip,index]:ordered){
   // A producer's own transfers are not independent evidence of fee demand.
   if(current.transactions[index].sender()==address(current.header.coinbase))continue;
   samples.push_back(tip);
   if(++added==fee_policy_.samples_per_block)break;
  }
  if(height+1==fee_policy_.history_blocks||current.header.number==1)break;
  auto parent=chain_.store().block_by_hash(current.header.parent_hash);
  if(!parent)throw RpcError(-32000,"Fee estimation ancestor unavailable; retry");
  current=std::move(*parent);
 }
 Int tip=fee_policy_.fallback_tip;
 if(!samples.empty()){
  std::sort(samples.begin(),samples.end());
  tip=samples[(samples.size()-1)*fee_policy_.percentile/100];
 }
 if(fee_policy_.maximum_tip)tip=std::min(tip,*fee_policy_.maximum_tip);
 if(chain_.tip_header().hash()!=hash)throw RpcError(-32000,"Chain head changed during fee estimation; retry");
 fee_head_=hash;fee_tip_=tip;return tip;
}

Json Rpc::fee_history(const Json& params) const {
 const Int count=quantity(params[0]);
 if(count<=0||count>1024)throw RpcError(-32602,"Block count must be between 1 and 1024");
 const Json percentiles=params.size()==3?params[2]:Json::array();
 if(!percentiles.is_array()||percentiles.size()>100)throw RpcError(-32602,"Expected at most 100 reward percentiles");
 double previous=-1;
 for(const auto& value:percentiles){
  if(!value.is_number())throw RpcError(-32602,"Percentiles must be numbers");
  double percentile=value.get<double>();
  if(!std::isfinite(percentile)||percentile<0||percentile>100||percentile<previous)throw RpcError(-32602,"Percentiles must be ordered between 0 and 100");
  previous=percentile;
 }
 auto newest=block(params[1]);
 if(!newest)throw RpcError(-32000,"Unknown block");
 Int oldest=std::max(Int(0),Int(newest->header.number-count+1));
 Json fees=Json::array(),ratios=Json::array(),rewards=Json::array();
 for(Int number=oldest;number<=newest->header.number;++number){
  auto current=chain_.store().block_by_number(number);
  if(!current)throw RpcError(-32000,"Fee history block unavailable");
  const auto& header=current->header;
  Int base=header.base_fee.value_or(Int(0));
  fees.push_back(quantity(base));
  ratios.push_back(header.gas_limit==0?0.0:header.gas_used.get_d()/header.gas_limit.get_d());
  if(percentiles.empty())continue;
  std::vector<std::pair<Int,Int>> samples;
  Int total=0;
  for(const auto& transaction:current->transactions){
   auto receipt=chain_.store().receipt(transaction.hash());
   if(!receipt)throw RpcError(-32000,"Fee history receipt unavailable");
   if(receipt->gas_used==0)continue;
   Int tip=std::max(Int(0),Int(transaction.tx.effective_gas_price(base)-base));
   samples.emplace_back(tip,receipt->gas_used);total+=receipt->gas_used;
  }
  std::sort(samples.begin(),samples.end());
  Json row=Json::array();
  for(const auto& percentile:percentiles){
   Int reward=0;
   if(!samples.empty()){
    // Preserve the input double exactly while calculating the gas threshold.
    mpq_class fraction(percentile.get<double>());fraction/=100;
    Int threshold=(total*fraction.get_num())/fraction.get_den();
    Int cumulative=0;reward=samples.back().first;
    for(const auto& [tip,gas]:samples){cumulative+=gas;if(cumulative>=threshold){reward=tip;break;}}
   }
   row.push_back(quantity(reward));
  }
  rewards.push_back(std::move(row));
 }
 fees.push_back(quantity(next_base_fee(newest->header)));
 Json result={{"oldestBlock",quantity(oldest)},{"baseFeePerGas",fees},{"gasUsedRatio",ratios}};
 if(!percentiles.empty())result["reward"]=std::move(rewards);
 return result;
}
}
