#include "nano/consensus_execution_payload.hpp"
#include <algorithm>
#include <stdexcept>
namespace nano {namespace {
using View=std::span<const uint8_t>;using Root=ConsensusRoot;
void config(const ExecutionPayloadLimits& l){if(!l.extra_data_bytes||!l.transaction_bytes||!l.transactions||l.transactions>UINT32_MAX/4)throw std::invalid_argument("Invalid execution payload SSZ limits");}
uint64_t number(View b,size_t at,size_t width){uint64_t n=0;for(size_t i=0;i<width;++i)n|=uint64_t(b[at+i])<<(8*i);return n;}
template<size_t N>std::array<uint8_t,N> array(View b,size_t at){std::array<uint8_t,N> out{};std::copy_n(b.begin()+at,N,out.begin());return out;}
Root scalar(View b){Root r{};std::copy(b.begin(),b.end(),r.begin());return r;}
Root integer_root(uint64_t n){Root r{};for(unsigned i=0;i<8;++i)r[i]=uint8_t(n>>(8*i));return r;}
Root merkle(std::vector<Root> nodes,uint64_t limit,const std::function<void()>& check){
 if(nodes.size()>limit||!limit)throw std::invalid_argument("Execution SSZ tree limit");
 Root zero{};uint64_t width=1;if(nodes.empty())nodes.push_back(zero);
 while(nodes.size()>1){if(nodes.size()%2)nodes.push_back(zero);for(size_t i=0;i<nodes.size()/2;++i){if(check)check();nodes[i]=consensus_signing_root(nodes[2*i],nodes[2*i+1]);}nodes.resize(nodes.size()/2);zero=consensus_signing_root(zero,zero);width*=2;}
 while(width<limit){if(check)check();nodes[0]=consensus_signing_root(nodes[0],zero);zero=consensus_signing_root(zero,zero);width*=2;}return nodes[0];
}
Root packed(View b,uint64_t limit,const std::function<void()>& check){std::vector<Root> chunks;for(size_t i=0;i<b.size();i+=32){if(check)check();chunks.push_back(scalar(b.subspan(i,std::min(size_t(32),b.size()-i))));}return merkle(std::move(chunks),(limit+31)/32,check);}
size_t size(const BellatrixExecutionPayload& p,const ExecutionPayloadLimits& l,const std::function<void()>& check){
 config(l);if(p.extra_data.size()>l.extra_data_bytes||p.transactions.size()>l.transactions)throw std::invalid_argument("Execution payload list limit");
 uint64_t n=508+uint64_t(p.extra_data.size())+4*uint64_t(p.transactions.size());
 for(const auto& tx:p.transactions){if(check)check();if(tx.size()>l.transaction_bytes)throw std::invalid_argument("Execution transaction byte limit");n+=tx.size();if(n>UINT32_MAX)throw std::invalid_argument("Execution payload SSZ offset overflow");}
 if(n>UINT32_MAX)throw std::invalid_argument("Execution payload SSZ offset overflow");return size_t(n);
}
}
std::optional<BellatrixExecutionPayload> decode_bellatrix_execution_payload(View b,const ExecutionPayloadLimits& l,size_t maximum_bytes,const std::function<void()>& check){
 config(l);if(check)check();if(b.size()<508||b.size()>maximum_bytes||b.size()>UINT32_MAX||number(b,436,4)!=508)return std::nullopt;
 const size_t transactions=number(b,504,4);if(transactions<508||transactions>b.size()||transactions-508>l.extra_data_bytes)return std::nullopt;
 const auto list=b.subspan(transactions);size_t count=0;
 if(!list.empty()){if(list.size()<4)return std::nullopt;const auto first=number(list,0,4);if(first<4||first%4||first>list.size()||first/4>l.transactions)return std::nullopt;count=first/4;
  size_t start=first;for(size_t i=0;i<count;++i){if(check)check();const size_t end=i+1==count?list.size():number(list,4*(i+1),4);if(start>end||end>list.size()||end-start>l.transaction_bytes)return std::nullopt;start=end;}}
 BellatrixExecutionPayload p;p.parent_hash=array<32>(b,0);p.fee_recipient=array<20>(b,32);p.state_root=array<32>(b,52);p.receipts_root=array<32>(b,84);p.logs_bloom=array<256>(b,116);p.prev_randao=array<32>(b,372);
 p.block_number=number(b,404,8);p.gas_limit=number(b,412,8);p.gas_used=number(b,420,8);p.timestamp=number(b,428,8);p.base_fee_per_gas=array<32>(b,440);p.block_hash=array<32>(b,472);p.extra_data.assign(b.begin()+508,b.begin()+transactions);
 p.transactions.reserve(count);for(size_t i=0;i<count;++i){if(check)check();const size_t start=number(list,4*i,4),end=i+1==count?list.size():number(list,4*(i+1),4);p.transactions.emplace_back(list.begin()+start,list.begin()+end);}return p;
}
Bytes encode_bellatrix_execution_payload(const BellatrixExecutionPayload& p,const ExecutionPayloadLimits& l,size_t maximum_bytes,const std::function<void()>& check){
 if(check)check();const auto n=size(p,l,check);if(n>maximum_bytes)throw std::invalid_argument("Execution payload transport budget");Bytes out;out.reserve(n);
 auto append=[&](const auto& bytes){out.insert(out.end(),bytes.begin(),bytes.end());};auto num=[&](uint64_t value,unsigned width){for(unsigned i=0;i<width;++i)out.push_back(uint8_t(value>>(8*i)));};
 append(p.parent_hash);append(p.fee_recipient);append(p.state_root);append(p.receipts_root);append(p.logs_bloom);append(p.prev_randao);num(p.block_number,8);num(p.gas_limit,8);num(p.gas_used,8);num(p.timestamp,8);num(508,4);append(p.base_fee_per_gas);append(p.block_hash);num(508+p.extra_data.size(),4);append(p.extra_data);
 size_t offset=4*p.transactions.size();for(const auto& tx:p.transactions){if(check)check();num(offset,4);offset+=tx.size();}for(const auto& tx:p.transactions){if(check)check();append(tx);}return out;
}
BellatrixExecutionPayloadHeader bellatrix_execution_payload_header(const BellatrixExecutionPayload& p,const ExecutionPayloadLimits& l,const std::function<void()>& check){
 size(p,l,check);if(check)check();std::vector<Root> transactions;transactions.reserve(p.transactions.size());
 for(const auto& tx:p.transactions){if(check)check();transactions.push_back(consensus_signing_root(packed(tx,l.transaction_bytes,check),integer_root(tx.size())));}
 BellatrixExecutionPayloadHeader h;static_cast<BellatrixExecutionFields&>(h)=p;
 h.transactions_root=consensus_signing_root(merkle(std::move(transactions),l.transactions,check),integer_root(p.transactions.size()));return h;
}
static ConsensusRoot execution_header_root(const BellatrixExecutionPayloadHeader& p,uint32_t extra_limit,const std::function<void()>& check,const ConsensusRoot* withdrawals=nullptr){
 if(!extra_limit||p.extra_data.size()>extra_limit)throw std::invalid_argument("Execution header extra data limit");
 if(check)check();
 std::vector<Root> fields{p.parent_hash,scalar(p.fee_recipient),p.state_root,p.receipts_root,packed(p.logs_bloom,256,check),p.prev_randao,integer_root(p.block_number),integer_root(p.gas_limit),integer_root(p.gas_used),integer_root(p.timestamp),consensus_signing_root(packed(p.extra_data,extra_limit,check),integer_root(p.extra_data.size())),p.base_fee_per_gas,p.block_hash,p.transactions_root};
 if(withdrawals)fields.push_back(*withdrawals);return merkle(std::move(fields),withdrawals?15:14,check);
}
ConsensusRoot bellatrix_execution_header_root(const BellatrixExecutionPayloadHeader& p,uint32_t extra_limit,const std::function<void()>& check){return execution_header_root(p,extra_limit,check);}
ConsensusRoot capella_execution_header_root(const CapellaExecutionPayloadHeader& p,uint32_t extra_limit,const std::function<void()>& check){return execution_header_root(p,extra_limit,check,&p.withdrawals_root);}
ConsensusRoot bellatrix_execution_payload_root(const BellatrixExecutionPayload& p,const ExecutionPayloadLimits& l,const std::function<void()>& check){
 return bellatrix_execution_header_root(bellatrix_execution_payload_header(p,l,check),l.extra_data_bytes,check);
}
std::optional<BellatrixExecutionPayloadHeader> decode_bellatrix_execution_header(View b,uint32_t extra_limit,size_t budget,const std::function<void()>& check){
 if(!extra_limit)throw std::invalid_argument("Execution header extra data limit");
 if(check)check();
 if(b.size()<536||b.size()>budget||b.size()>UINT32_MAX||number(b,436,4)!=536||b.size()-536>extra_limit)return std::nullopt;
 BellatrixExecutionPayloadHeader p;p.parent_hash=array<32>(b,0);p.fee_recipient=array<20>(b,32);p.state_root=array<32>(b,52);p.receipts_root=array<32>(b,84);p.logs_bloom=array<256>(b,116);p.prev_randao=array<32>(b,372);
 p.block_number=number(b,404,8);p.gas_limit=number(b,412,8);p.gas_used=number(b,420,8);p.timestamp=number(b,428,8);p.base_fee_per_gas=array<32>(b,440);p.block_hash=array<32>(b,472);p.transactions_root=array<32>(b,504);p.extra_data.assign(b.begin()+536,b.end());return p;
}
Bytes encode_bellatrix_execution_header(const BellatrixExecutionPayloadHeader& p,uint32_t extra_limit,size_t budget,const std::function<void()>& check){
 if(check)check();
 if(!extra_limit||p.extra_data.size()>extra_limit||p.extra_data.size()>UINT32_MAX-536||budget<536||p.extra_data.size()>budget-536)throw std::invalid_argument("Execution header size or budget");
 Bytes out;out.reserve(536+p.extra_data.size());
 auto append=[&](const auto& b){out.insert(out.end(),b.begin(),b.end());};auto num=[&](uint64_t value,unsigned width){for(unsigned i=0;i<width;++i)out.push_back(uint8_t(value>>(8*i)));};
 append(p.parent_hash);append(p.fee_recipient);append(p.state_root);append(p.receipts_root);append(p.logs_bloom);append(p.prev_randao);num(p.block_number,8);num(p.gas_limit,8);num(p.gas_used,8);num(p.timestamp,8);num(536,4);append(p.base_fee_per_gas);append(p.block_hash);append(p.transactions_root);append(p.extra_data);return out;
}
std::optional<CapellaExecutionPayloadHeader> decode_capella_execution_header(View b,uint32_t extra_limit,size_t budget,const std::function<void()>& check){
 if(!extra_limit)throw std::invalid_argument("Execution header extra data limit");
 if(check)check();
 if(b.size()<568||b.size()>budget||b.size()>UINT32_MAX||number(b,436,4)!=568||b.size()-568>extra_limit)return std::nullopt;
 // Reuse Bellatrix's common-field decoder after validating the Capella layout.
 Bytes common(b.begin(),b.begin()+536);common.insert(common.end(),b.begin()+568,b.end());
 for(unsigned i=0;i<4;++i)common[436+i]=uint8_t(536U>>(8*i));
 const auto decoded=decode_bellatrix_execution_header(common,extra_limit,budget-32,check);
 if(!decoded)return std::nullopt;
 CapellaExecutionPayloadHeader out;static_cast<BellatrixExecutionPayloadHeader&>(out)=*decoded;out.withdrawals_root=array<32>(b,536);return out;
}
Bytes encode_capella_execution_header(const CapellaExecutionPayloadHeader& p,uint32_t extra_limit,size_t budget,const std::function<void()>& check){
 if(check)check();
 if(budget<568||p.extra_data.size()>UINT32_MAX-568)throw std::invalid_argument("Capella header size or budget");
 auto out=encode_bellatrix_execution_header(p,extra_limit,budget-32,check);
 out.insert(out.begin()+536,p.withdrawals_root.begin(),p.withdrawals_root.end());
 for(unsigned i=0;i<4;++i)out[436+i]=uint8_t(568U>>(8*i));
 return out;
}
std::optional<CapellaExecutionPayload> decode_capella_execution_payload(View b,
 const ExecutionPayloadLimits& l,uint64_t maximum_withdrawals,size_t budget,const std::function<void()>& check){
 config(l);if(!maximum_withdrawals||maximum_withdrawals>UINT32_MAX/44)throw std::invalid_argument("invalid Capella withdrawal list limit");
 if(check)check();
 if(b.size()<512||b.size()>budget||b.size()>UINT32_MAX||number(b,436,4)!=512)return std::nullopt;
 const size_t transactions=number(b,504,4),withdrawals=number(b,508,4);
 if(transactions<512||transactions>withdrawals||withdrawals>b.size()||transactions-512>l.extra_data_bytes||
    (b.size()-withdrawals)%44||(b.size()-withdrawals)/44>maximum_withdrawals)return std::nullopt;
 // Withdrawals delimit the transaction list. Never expose their bytes as a transaction.
 Bytes common(b.begin(),b.begin()+508);common.insert(common.end(),b.begin()+512,b.begin()+withdrawals);
 for(unsigned i=0;i<4;++i){common[436+i]=uint8_t(508U>>(8*i));common[504+i]=uint8_t((transactions-4)>>(8*i));}
 auto decoded=decode_bellatrix_execution_payload(common,l,budget-4,check);if(!decoded)return std::nullopt;
 CapellaExecutionPayload out;static_cast<BellatrixExecutionPayload&>(out)=std::move(*decoded);
 for(size_t at=withdrawals;at<b.size();at+=44){if(check)check();out.withdrawals.push_back({number(b,at,8),number(b,at+8,8),array<20>(b,at+16),number(b,at+36,8)});}
 return out;
}
Bytes encode_capella_execution_payload(const CapellaExecutionPayload& p,
 const ExecutionPayloadLimits& l,uint64_t maximum_withdrawals,size_t budget,const std::function<void()>& check){
 if(!maximum_withdrawals||maximum_withdrawals>UINT32_MAX/44||p.withdrawals.size()>maximum_withdrawals)
  throw std::invalid_argument("invalid Capella withdrawal list limit");
 const uint64_t extra=4+44*uint64_t(p.withdrawals.size()),common_size=size(p,l,check);
 if(extra>budget||common_size>budget-extra||common_size+extra>UINT32_MAX)throw std::invalid_argument("Capella payload byte budget");
 auto out=encode_bellatrix_execution_payload(p,l,budget-extra,check);const auto withdrawal_offset=out.size()+4;
 out.insert(out.begin()+508,4,0);
 for(unsigned i=0;i<4;++i){out[436+i]=uint8_t(512U>>(8*i));out[504+i]=uint8_t((512+p.extra_data.size())>>(8*i));out[508+i]=uint8_t(withdrawal_offset>>(8*i));}
 auto num=[&](uint64_t v){for(unsigned i=0;i<8;++i)out.push_back(uint8_t(v>>(8*i)));};
 for(const auto& w:p.withdrawals){if(check)check();num(w.index);num(w.validator_index);out.insert(out.end(),w.address.begin(),w.address.end());num(w.amount_gwei);}
 return out;
}
CapellaExecutionPayloadHeader capella_execution_payload_header(const CapellaExecutionPayload& p,
 const ExecutionPayloadLimits& l,uint64_t maximum_withdrawals,const std::function<void()>& check){
 if(!maximum_withdrawals||maximum_withdrawals>UINT32_MAX/44||p.withdrawals.size()>maximum_withdrawals)
  throw std::invalid_argument("invalid Capella withdrawal list limit");
 CapellaExecutionPayloadHeader out;static_cast<BellatrixExecutionPayloadHeader&>(out)=bellatrix_execution_payload_header(p,l,check);
 std::vector<Root> roots;roots.reserve(p.withdrawals.size());
 for(const auto& w:p.withdrawals){if(check)check();roots.push_back(consensus_signing_root(
  consensus_signing_root(integer_root(w.index),integer_root(w.validator_index)),
  consensus_signing_root(scalar(w.address),integer_root(w.amount_gwei))));}
 out.withdrawals_root=consensus_signing_root(merkle(std::move(roots),maximum_withdrawals,check),integer_root(p.withdrawals.size()));return out;
}
ConsensusRoot capella_execution_payload_root(const CapellaExecutionPayload& p,
 const ExecutionPayloadLimits& l,uint64_t maximum_withdrawals,const std::function<void()>& check){
 return capella_execution_header_root(capella_execution_payload_header(p,l,maximum_withdrawals,check),l.extra_data_bytes,check);
}

}
