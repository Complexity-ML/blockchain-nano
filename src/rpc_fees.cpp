#include "nano/rpc.hpp"
#include <algorithm>
#include <cmath>

namespace nano {
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
