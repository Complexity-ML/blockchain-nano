#include "nano/consensus_deposit_history.hpp"
namespace nano {
namespace {
ConsensusRoot scalar(uint64_t n){ConsensusRoot out{};for(unsigned i=0;i<8;++i)out[i]=uint8_t(n>>(8*i));return out;}
const std::array<ConsensusRoot,33>& zeros(){static const auto value=[] {std::array<ConsensusRoot,33> out{};for(unsigned i=1;i<=32;++i)out[i]=consensus_signing_root(out[i-1],out[i-1]);return out;}();return value;}
Bytes view(const State& state,const Header& header,const char* name,const std::function<void()>& check){
 State isolated=state;ExecutionContext ctx;ctx.revision=EvmRevision::Paris;ctx.address=ethereum_deposit_contract;ctx.code=state.get(ctx.address).code;
 const std::string signature=name;ctx.data=keccak(Bytes(signature.begin(),signature.end()));ctx.data.resize(4);ctx.is_static=true;ctx.checkpoint=check;ctx.number=header.number;ctx.timestamp=header.timestamp;
 auto result=EVM(isolated).execute(ctx);if(!result.success)throw std::invalid_argument("deposit history contract view failed");return result.return_data;
}
void validate_view(const ExecutionDepositHistory& history,const State& state,const Header& header,const std::function<void()>& check){
 if(check)check();const auto count=view(state,header,"get_deposit_count()",check);
 Bytes expected(96);expected[31]=32;expected[63]=8;for(unsigned i=0;i<8;++i)expected[64+i]=uint8_t(history.count()>>(8*i));
 const auto root=history.root();
 if(count!=expected||view(state,header,"get_deposit_root()",check)!=Bytes(root.begin(),root.end()))throw std::invalid_argument("deposit history differs from execution contract");
}
}
struct ExecutionDepositHistory::Node {
 ConsensusRoot hash;
 std::shared_ptr<const Node> left,right;
 struct Data {ConsensusDepositMessage message;std::array<uint8_t,96> signature;};
 std::shared_ptr<const Data> data;
};
ExecutionDepositHistory::ExecutionDepositHistory(Bytes anchor):execution_hash_(std::move(anchor)){
 if(execution_hash_.size()!=32)throw std::invalid_argument("deposit history anchor width");
}
ConsensusRoot ExecutionDepositHistory::root()const{return consensus_signing_root(tree_?tree_->hash:zeros()[32],scalar(count_));}
std::shared_ptr<const ExecutionDepositHistory::Node> ExecutionDepositHistory::insert(const std::shared_ptr<const Node>& old,unsigned depth,uint64_t index,const Phase0Deposit& deposit,const std::function<void()>& check){
 if(check)check();if(!depth){if(old)throw std::logic_error("deposit leaf already occupied");return std::make_shared<const Node>(Node{consensus_deposit_data_root(deposit),{},{},std::make_shared<const Node::Data>(Node::Data{deposit.message,deposit.signature})});}
 auto left=old?old->left:nullptr,right=old?old->right:nullptr;
 if((index>>(depth-1))&1)right=insert(right,depth-1,index,deposit,check);else left=insert(left,depth-1,index,deposit,check);
 return std::make_shared<const Node>(Node{consensus_signing_root(left?left->hash:zeros()[depth-1],right?right->hash:zeros()[depth-1]),std::move(left),std::move(right),{}});
}
Phase0Deposit ExecutionDepositHistory::proof(uint64_t index,const std::function<void()>& check)const{
 if(index>=count_)throw std::out_of_range("deposit proof index");Phase0Deposit out{};auto node=tree_;
 for(unsigned depth=32;depth;--depth){if(check)check();if(!node)throw std::logic_error("missing deposit path");const bool right=(index>>(depth-1))&1;const auto sibling=right?node->left:node->right;out.proof[depth-1]=sibling?sibling->hash:zeros()[depth-1];node=right?node->right:node->left;}
 if(!node||!node->data)throw std::logic_error("missing deposit data");out.message=node->data->message;out.signature=node->data->signature;out.proof[32]=scalar(count_);return out;
}
std::pair<ExecutionDepositHistory,ExecutionDepositBatch> ExecutionDepositHistory::extend(const ExecutionEngine& engine,const State& parent_state,const Header& parent,const Block& block,const Bytes& expected_hash,const Bytes& code_hash,size_t maximum,const std::function<void()>& check)const{
 if(check)check();if(parent.hash()!=execution_hash_)throw std::invalid_argument("deposit history execution parent mismatch");
 auto batch=replay_execution_deposits(engine,parent_state,parent,block,expected_hash,code_hash,count_,maximum,check);
 validate_view(*this,parent_state,parent,check);
 auto next=append_verified_execution(block.header,batch.state,batch.receipts,code_hash,maximum,check);
 if(next.count_!=batch.next_deposit_index)throw std::logic_error("deposit history count mismatch");
 if(check)check();return {std::move(next),std::move(batch)};
}
ExecutionDepositHistory ExecutionDepositHistory::append_verified_execution(const Header& header,const State& state,const std::vector<Receipt>& receipts,const Bytes& code_hash,size_t maximum,const std::function<void()>& check)const{
 if(check)check();if(header.parent_hash!=execution_hash_||header.receipts_root!=receipts_root(receipts)||header.state_root!=state.root())throw std::invalid_argument("deposit execution commitments mismatch");
 auto next=*this;size_t added=0;
 for(const auto& receipt:receipts){if(check)check();for(const auto& log:receipt.logs){
  if(check)check();const auto encoded=deposit_requests({log});if(encoded.size()==1)continue;
  if(!receipt.success||encoded.size()!=193)throw std::invalid_argument("invalid executed deposit event");
  auto number=[&](size_t at){uint64_t n=0;for(unsigned i=0;i<8;++i)n|=uint64_t(encoded[at+i])<<(8*i);return n;};
  if(number(185)!=next.count_||next.count_>=((uint64_t(1)<<32)-1))throw std::invalid_argument("nonconsecutive execution deposit index");
  if(added>=maximum)throw std::length_error("execution deposit batch capacity");
  Phase0Deposit d{};std::copy_n(encoded.begin()+1,48,d.message.public_key.begin());std::copy_n(encoded.begin()+49,32,d.message.withdrawal_credentials.begin());d.message.amount_gwei=number(81);std::copy_n(encoded.begin()+89,96,d.signature.begin());
  next.tree_=insert(next.tree_,32,next.count_,d,check);++next.count_;++added;
 }}
 next.execution_hash_=header.hash();next.validate_execution(header,state,code_hash,check);if(check)check();return next;
}

void ExecutionDepositHistory::validate_execution(const Header& header,const State& state,const Bytes& code_hash,const std::function<void()>& check)const{
 if(check)check();if(header.hash()!=execution_hash_||state.root()!=header.state_root||code_hash.size()!=32||state.get(ethereum_deposit_contract).code.empty()||keccak(state.get(ethereum_deposit_contract).code)!=code_hash)throw std::invalid_argument("deposit history execution binding mismatch");
 validate_view(*this,state,header,check);
}

Json ExecutionDepositHistory::snapshot(const std::function<void()>& check)const{
 if(check)check();Json entries=Json::array();
 for(uint64_t i=0;i<count_;++i){const auto d=proof(i,check);Bytes data(d.message.public_key.begin(),d.message.public_key.end());data.insert(data.end(),d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end());for(unsigned j=0;j<8;++j)data.push_back(uint8_t(d.message.amount_gwei>>(8*j)));data.insert(data.end(),d.signature.begin(),d.signature.end());entries.push_back(hex(data));}
 return {{"format","nano-execution-deposit-history-v1"},{"executionHash",hex(execution_hash_)},{"deposits",std::move(entries)}};
}
ExecutionDepositHistory ExecutionDepositHistory::from_snapshot(const Json& document,const Header& header,const State& state,const Bytes& expected_hash,const Bytes& code_hash,size_t maximum,const std::function<void()>& check){
 if(check)check();
 if(expected_hash.size()!=32||header.hash()!=expected_hash||state.root()!=header.state_root||code_hash.size()!=32||state.get(ethereum_deposit_contract).code.empty()||keccak(state.get(ethereum_deposit_contract).code)!=code_hash)
  throw std::invalid_argument("unauthenticated deposit history restore");
 if(!document.is_object()||document.size()!=3||document.value("format",Json())!="nano-execution-deposit-history-v1"||document.value("executionHash",Json())!=hex(expected_hash)||!document.contains("deposits")||!document.at("deposits").is_array()||document.at("deposits").size()>maximum||document.at("deposits").size()>((uint64_t(1)<<32)-1))
  throw std::invalid_argument("invalid deposit history snapshot");
 ExecutionDepositHistory result(expected_hash);
 for(const auto& entry:document.at("deposits")){
  if(check)check();if(!entry.is_string()||entry.get_ref<const std::string&>().size()!=370)throw std::invalid_argument("invalid deposit history data width");
  const auto data=unhex(entry.get_ref<const std::string&>());if(data.size()!=184||hex(data)!=entry.get_ref<const std::string&>())throw std::invalid_argument("noncanonical deposit history data");
  Phase0Deposit deposit{};std::copy_n(data.begin(),48,deposit.message.public_key.begin());std::copy_n(data.begin()+48,32,deposit.message.withdrawal_credentials.begin());for(unsigned i=0;i<8;++i)deposit.message.amount_gwei|=uint64_t(data[80+i])<<(8*i);std::copy_n(data.begin()+88,96,deposit.signature.begin());
  result.tree_=insert(result.tree_,32,result.count_,deposit,check);++result.count_;
 }
 validate_view(result,state,header,check);if(check)check();return result;
}

}
