#include "nano/consensus_deposit_history.hpp"
#include "nano/consensus_canonical_deposit_context.hpp"
#include <algorithm>
#include <exception>
#include <optional>
namespace nano {
namespace {
ConsensusRoot scalar(uint64_t n){ConsensusRoot out{};for(unsigned i=0;i<8;++i)out[i]=uint8_t(n>>(8*i));return out;}
const std::array<ConsensusRoot,33>& zeros(){static const auto value=[] {std::array<ConsensusRoot,33> out{};for(unsigned i=1;i<=32;++i)out[i]=consensus_signing_root(out[i-1],out[i-1]);return out;}();return value;}
Bytes view(const State& state,const Header& header,const char* name,const std::function<void()>& check){
 State isolated=state;ExecutionContext ctx;ctx.revision=EvmRevision::Paris;ctx.address=ethereum_deposit_contract;ctx.code=state.get(ctx.address).code;
 const std::string signature=name;ctx.data=keccak(Bytes(signature.begin(),signature.end()));ctx.data.resize(4);ctx.is_static=true;ctx.number=header.number;ctx.timestamp=header.timestamp;
 // The EVM preserves ExecutionCancelled but treats other exceptions as execution
 // failures. A caller's typed duty expiry must reach its scheduler unchanged.
 std::exception_ptr interrupted;
 if(check)ctx.checkpoint=[&]{try{check();}catch(...){interrupted=std::current_exception();throw ExecutionCancelled("deposit view checkpoint interrupted");}};
 auto result=[&]{try{return EVM(isolated).execute(ctx);}catch(...){if(interrupted)std::rethrow_exception(interrupted);throw;}}();
 if(interrupted)std::rethrow_exception(interrupted);
 if(!result.success)throw std::invalid_argument("deposit history contract view failed");return result.return_data;
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
 struct Cold {unsigned depth;CanonicalDepositBlobRef ref;};
 std::optional<Cold> cold;
 using Ptr=std::shared_ptr<const Node>;
 using Context=std::shared_ptr<CanonicalDepositContext>;
 static constexpr size_t maximum_wire=256;
 static constexpr std::array<uint8_t,8> magic={'N','A','N','O','D','P',1,0};
 static void checkpoint(const std::function<void()>& check){if(check)check();}
 static ConsensusRoot digest(const Bytes& wire){
  const auto bytes=sha256(wire);if(bytes.size()!=32)throw std::logic_error("deposit node SHA256 width");
  ConsensusRoot out{};std::copy(bytes.begin(),bytes.end(),out.begin());return out;
 }
 static uint64_t read64(const Bytes& wire,size_t& at){
  if(at>wire.size()||wire.size()-at<8)throw std::invalid_argument("short deposit node integer");
  uint64_t out=0;for(unsigned i=0;i<8;++i)out|=uint64_t(wire[at++])<<(8*i);return out;
 }
 static void write64(Bytes& wire,uint64_t value){for(unsigned i=0;i<8;++i)wire.push_back(uint8_t(value>>(8*i)));}
 static ConsensusRoot read_root(const Bytes& wire,size_t& at){
  if(at>wire.size()||wire.size()-at<32)throw std::invalid_argument("short deposit node root");
  ConsensusRoot out{};std::copy_n(wire.begin()+at,32,out.begin());at+=32;return out;
 }
 static void write_root(Bytes& wire,const ConsensusRoot& root){wire.insert(wire.end(),root.begin(),root.end());}
 static Ptr stub(const ConsensusRoot& hash,unsigned depth,const CanonicalDepositBlobRef& ref,const std::function<void()>& check){
  checkpoint(check);if(depth>32||ref.bytes<10||ref.bytes>maximum_wire)throw std::invalid_argument("deposit cold reference bounds");
  return std::make_shared<const Node>(Node{hash,{},{},{},Cold{depth,ref}});
 }
 static Ptr child(const Bytes& wire,size_t& at,unsigned depth,const std::function<void()>& check){
  checkpoint(check);if(at>=wire.size())throw std::invalid_argument("short deposit child flag");
  const auto flag=wire[at++];if(!flag)return {};
  if(flag!=1||at>=wire.size()||wire[at++]!=depth)throw std::invalid_argument("deposit child type/depth");
  const auto hash=read_root(wire,at),content=read_root(wire,at);const auto bytes=read64(wire,at);
  return stub(hash,depth,CanonicalDepositBlobRef{content,bytes},check);
 }
 static void encode_child(Bytes& wire,const Ptr& value,unsigned depth){
  if(!value){wire.push_back(0);return;}
  if(!value->cold||value->cold->depth!=depth||value->left||value->right||value->data)
   throw std::logic_error("unfolded deposit child");
  wire.push_back(1);wire.push_back(uint8_t(depth));write_root(wire,value->hash);write_root(wire,value->cold->ref.digest);write64(wire,value->cold->ref.bytes);
 }
 // The returned hot node owns only its two stubs, never the raw lease. Release
 // the lease before descending another level so pinning is O(1) raw blobs.
 static Node expand(const Ptr& node,unsigned depth,const Context& context,const std::function<void()>& check){
  checkpoint(check);if(!node||depth>32)throw std::logic_error("missing deposit node");
  if(!node->cold){
   if((!depth&&(!node->data||node->left||node->right))||(depth&&(node->data||(!node->left&&!node->right))))
    throw std::logic_error("invalid hot deposit node shape");
   return *node;
  }
  if(!context||node->cold->depth!=depth||node->left||node->right||node->data)
   throw std::logic_error("deposit cold node context/type");
  context->ensure_open(check);const auto& ref=node->cold->ref;
  if(ref.bytes<10||ref.bytes>maximum_wire)throw std::invalid_argument("deposit cold node width");
  const auto wire=context->lease_node_blob(ref,check);
  checkpoint(check);if(!wire||wire->size()!=ref.bytes||digest(*wire)!=ref.digest||!std::equal(magic.begin(),magic.end(),wire->begin())||(*wire)[9]!=depth)
   throw std::invalid_argument("deposit cold node content binding");
  Node out{node->hash,{},{},{},{}};size_t at=10;
  if(!depth){
   if((*wire)[8]!=0||wire->size()!=194)throw std::invalid_argument("deposit cold leaf encoding");
   Phase0Deposit deposit{};std::copy_n(wire->begin()+at,48,deposit.message.public_key.begin());at+=48;
   std::copy_n(wire->begin()+at,32,deposit.message.withdrawal_credentials.begin());at+=32;
   deposit.message.amount_gwei=read64(*wire,at);std::copy_n(wire->begin()+at,96,deposit.signature.begin());at+=96;
   if(consensus_deposit_data_root(deposit)!=node->hash)throw std::invalid_argument("deposit cold leaf SSZ root");
   checkpoint(check);out.data=std::make_shared<const Data>(Data{deposit.message,deposit.signature});
  }else{
   if((*wire)[8]!=1)throw std::invalid_argument("deposit cold branch encoding");
   out.left=child(*wire,at,depth-1,check);out.right=child(*wire,at,depth-1,check);
   if((!out.left&&!out.right)||consensus_signing_root(out.left?out.left->hash:zeros()[depth-1],out.right?out.right->hash:zeros()[depth-1])!=node->hash)
    throw std::invalid_argument("deposit cold branch SSZ root");
  }
  if(at!=wire->size())throw std::invalid_argument("deposit cold node trailing bytes");
  context->ensure_open(check);checkpoint(check);return out;
 }
 // Copy a closure only when moving to another context; in the same context,
 // immutable stubs are reused. New paths are folded bottom-up immediately.
 static Ptr persist(const Ptr& node,unsigned depth,const Context& source,const Context& target,const std::function<void()>& check){
  checkpoint(check);if(!target)throw std::invalid_argument("missing deposit cold target");target->ensure_open(check);
  if(!node)return {};
  if(node->cold&&source==target){
   if(node->cold->depth!=depth||node->left||node->right||node->data)throw std::logic_error("deposit retained stub shape");
   return node;
  }
  auto expanded=expand(node,depth,source,check);
  Bytes wire(magic.begin(),magic.end());wire.push_back(depth?1:0);wire.push_back(uint8_t(depth));
  if(!depth){
   const auto& d=*expanded.data;wire.insert(wire.end(),d.message.public_key.begin(),d.message.public_key.end());
   wire.insert(wire.end(),d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end());write64(wire,d.message.amount_gwei);wire.insert(wire.end(),d.signature.begin(),d.signature.end());
  }else{
   const auto left=persist(expanded.left,depth-1,source,target,check),right=persist(expanded.right,depth-1,source,target,check);
   encode_child(wire,left,depth-1);encode_child(wire,right,depth-1);
  }
  if(wire.size()>maximum_wire)throw std::logic_error("deposit node encoding capacity");
  checkpoint(check);target->ensure_open(check);const auto ref=target->put_node_blob(wire,check);
  checkpoint(check);if(ref.bytes!=wire.size()||ref.digest!=digest(wire))throw std::invalid_argument("deposit cold write content binding");
  target->ensure_open(check);return stub(node->hash,depth,ref,check);
 }
};
ExecutionDepositHistory::ExecutionDepositHistory(Bytes anchor):execution_hash_(std::move(anchor)){
 if(execution_hash_.size()!=32)throw std::invalid_argument("deposit history anchor width");
}
void ExecutionDepositHistory::check_storage(const std::function<void()>& check)const{
 if(check)check();if(cold_storage_)cold_storage_->ensure_open(check);
}
ConsensusRoot ExecutionDepositHistory::root()const{check_storage({});return consensus_signing_root(tree_?tree_->hash:zeros()[32],scalar(count_));}
ExecutionDepositHistory ExecutionDepositHistory::with_cold_storage(std::shared_ptr<CanonicalDepositContext> context,const std::function<void()>& check)const{
 check_storage(check);if(!context)throw std::invalid_argument("missing deposit cold storage");context->ensure_open(check);
 auto next=*this;next.tree_=Node::persist(tree_,32,cold_storage_,context,check);next.cold_storage_=std::move(context);
 next.check_storage(check);if(next.root()!=root())throw std::logic_error("deposit cold migration root mismatch");next.check_storage(check);return next;
}
std::shared_ptr<const ExecutionDepositHistory::Node> ExecutionDepositHistory::insert(const std::shared_ptr<const Node>& old,unsigned depth,uint64_t index,const Phase0Deposit& deposit,const std::function<void()>& check,const std::shared_ptr<CanonicalDepositContext>& context){
 if(check)check();if(!depth){if(old)throw std::logic_error("deposit leaf already occupied");return std::make_shared<const Node>(Node{consensus_deposit_data_root(deposit),{},{},std::make_shared<const Node::Data>(Node::Data{deposit.message,deposit.signature}),{}});}
 std::shared_ptr<const Node> left,right;if(old){auto expanded=Node::expand(old,depth,context,check);left=std::move(expanded.left);right=std::move(expanded.right);}
 if((index>>(depth-1))&1)right=insert(right,depth-1,index,deposit,check,context);else left=insert(left,depth-1,index,deposit,check,context);
 if(check)check();return std::make_shared<const Node>(Node{consensus_signing_root(left?left->hash:zeros()[depth-1],right?right->hash:zeros()[depth-1]),std::move(left),std::move(right),{},{}});
}
std::shared_ptr<const ExecutionDepositHistory::Node> ExecutionDepositHistory::prefix_tree(const std::shared_ptr<const Node>& node,unsigned depth,uint64_t count,const std::function<void()>& check,const std::shared_ptr<CanonicalDepositContext>& context){
 if(check)check();if(!count)return {};const uint64_t capacity=uint64_t(1)<<depth;
 if(count==capacity)return node;
 if(!node||!depth||count>capacity)throw std::logic_error("invalid deposit prefix path");
 const auto expanded=Node::expand(node,depth,context,check);const auto half=capacity/2;auto left=expanded.left;std::shared_ptr<const Node> right;
 if(count<=half)left=prefix_tree(left,depth-1,count,check,context);
 else right=prefix_tree(expanded.right,depth-1,count-half,check,context);
 if(check)check();return std::make_shared<const Node>(Node{consensus_signing_root(left?left->hash:zeros()[depth-1],right?right->hash:zeros()[depth-1]),std::move(left),std::move(right),{},{}});
}
ExecutionDepositHistory ExecutionDepositHistory::authenticated_prefix(uint64_t count,const Bytes& hash,const ConsensusRoot& expected,const std::function<void()>& check)const{
 check_storage(check);if(count>count_)throw std::invalid_argument("deposit prefix count exceeds history");
 ExecutionDepositHistory result(hash);result.count_=count;result.cold_storage_=cold_storage_;result.tree_=prefix_tree(tree_,32,count,check,cold_storage_);
 if(result.root()!=expected)throw std::invalid_argument("deposit prefix commitment mismatch");
 result.check_storage(check);return result;
}
Phase0Deposit ExecutionDepositHistory::proof(uint64_t index,const std::function<void()>& check)const{
 check_storage(check);if(index>=count_)throw std::out_of_range("deposit proof index");Phase0Deposit out{};auto node=tree_;
 for(unsigned depth=32;depth;--depth){if(check)check();const auto expanded=Node::expand(node,depth,cold_storage_,check);const bool right=(index>>(depth-1))&1;const auto sibling=right?expanded.left:expanded.right;out.proof[depth-1]=sibling?sibling->hash:zeros()[depth-1];node=right?expanded.right:expanded.left;}
 const auto leaf=Node::expand(node,0,cold_storage_,check);out.message=leaf.data->message;out.signature=leaf.data->signature;out.proof[32]=scalar(count_);check_storage(check);return out;
}
std::pair<ExecutionDepositHistory,ExecutionDepositBatch> ExecutionDepositHistory::extend(const ExecutionEngine& engine,const State& parent_state,const Header& parent,const Block& block,const Bytes& expected_hash,const Bytes& code_hash,size_t maximum,const std::function<void()>& check)const{
 check_storage(check);if(parent.hash()!=execution_hash_)throw std::invalid_argument("deposit history execution parent mismatch");
 auto batch=replay_execution_deposits(engine,parent_state,parent,block,expected_hash,code_hash,count_,maximum,check);
 validate_view(*this,parent_state,parent,check);
 auto next=append_verified_execution(block.header,batch.state,batch.receipts,code_hash,maximum,check);
 if(next.count_!=batch.next_deposit_index)throw std::logic_error("deposit history count mismatch");
 next.check_storage(check);return {std::move(next),std::move(batch)};
}
ExecutionDepositHistory ExecutionDepositHistory::append_verified_execution(const Header& header,const State& state,const std::vector<Receipt>& receipts,const Bytes& code_hash,size_t maximum,const std::function<void()>& check)const{
 check_storage(check);if(header.parent_hash!=execution_hash_||header.receipts_root!=receipts_root(receipts)||header.state_root!=state.root())throw std::invalid_argument("deposit execution commitments mismatch");
 auto next=*this;size_t added=0;
 for(const auto& receipt:receipts){if(check)check();for(const auto& log:receipt.logs){
  if(check)check();const auto encoded=deposit_requests({log});if(encoded.size()==1)continue;
  if(!receipt.success||encoded.size()!=193)throw std::invalid_argument("invalid executed deposit event");
  auto number=[&](size_t at){uint64_t n=0;for(unsigned i=0;i<8;++i)n|=uint64_t(encoded[at+i])<<(8*i);return n;};
  if(number(185)!=next.count_||next.count_>=((uint64_t(1)<<32)-1))throw std::invalid_argument("nonconsecutive execution deposit index");
  if(added>=maximum)throw std::length_error("execution deposit batch capacity");
  Phase0Deposit d{};std::copy_n(encoded.begin()+1,48,d.message.public_key.begin());std::copy_n(encoded.begin()+49,32,d.message.withdrawal_credentials.begin());d.message.amount_gwei=number(81);std::copy_n(encoded.begin()+89,96,d.signature.begin());
  auto path=insert(next.tree_,32,next.count_,d,check,next.cold_storage_);
  // Fold each path before the next event; a large execution block cannot keep
  // a hot path for every deposit alive. Failed validation leaves only counted
  // orphan blobs in scratch, never a published candidate History.
  next.tree_=next.cold_storage_?Node::persist(path,32,next.cold_storage_,next.cold_storage_,check):std::move(path);++next.count_;++added;
 }}
 next.execution_hash_=header.hash();next.validate_execution(header,state,code_hash,check);next.check_storage(check);return next;
}

void ExecutionDepositHistory::validate_execution(const Header& header,const State& state,const Bytes& code_hash,const std::function<void()>& check)const{
 check_storage(check);if(header.hash()!=execution_hash_||state.root()!=header.state_root||code_hash.size()!=32||state.get(ethereum_deposit_contract).code.empty()||keccak(state.get(ethereum_deposit_contract).code)!=code_hash)throw std::invalid_argument("deposit history execution binding mismatch");
 validate_view(*this,state,header,check);
}

Json ExecutionDepositHistory::snapshot(const std::function<void()>& check)const{
 check_storage(check);Json entries=Json::array();
 for(uint64_t i=0;i<count_;++i){const auto d=proof(i,check);Bytes data(d.message.public_key.begin(),d.message.public_key.end());data.insert(data.end(),d.message.withdrawal_credentials.begin(),d.message.withdrawal_credentials.end());for(unsigned j=0;j<8;++j)data.push_back(uint8_t(d.message.amount_gwei>>(8*j)));data.insert(data.end(),d.signature.begin(),d.signature.end());entries.push_back(hex(data));}
 return {{"format","nano-execution-deposit-history-v1"},{"executionHash",hex(execution_hash_)},{"deposits",std::move(entries)}};
}
ExecutionDepositHistory ExecutionDepositHistory::from_snapshot(const Json& document,const Header& header,const State& state,const Bytes& expected_hash,const Bytes& code_hash,size_t maximum,const std::function<void()>& check){
 return read_snapshot(document,header,state,expected_hash,code_hash,maximum,{},check);
}
ExecutionDepositHistory ExecutionDepositHistory::from_snapshot_cold(const Json& document,const Header& header,const State& state,const Bytes& expected_hash,const Bytes& code_hash,size_t maximum,std::shared_ptr<CanonicalDepositContext> context,const std::function<void()>& check){
 if(!context)throw std::invalid_argument("missing deposit cold restore storage");context->ensure_open(check);
 return read_snapshot(document,header,state,expected_hash,code_hash,maximum,std::move(context),check);
}
ExecutionDepositHistory ExecutionDepositHistory::read_snapshot(const Json& document,const Header& header,const State& state,const Bytes& expected_hash,const Bytes& code_hash,size_t maximum,std::shared_ptr<CanonicalDepositContext> context,const std::function<void()>& check){
 if(check)check();
 if(expected_hash.size()!=32||header.hash()!=expected_hash||state.root()!=header.state_root||code_hash.size()!=32||state.get(ethereum_deposit_contract).code.empty()||keccak(state.get(ethereum_deposit_contract).code)!=code_hash)
  throw std::invalid_argument("unauthenticated deposit history restore");
 if(!document.is_object()||document.size()!=3||document.value("format",Json())!="nano-execution-deposit-history-v1"||document.value("executionHash",Json())!=hex(expected_hash)||!document.contains("deposits")||!document.at("deposits").is_array()||document.at("deposits").size()>maximum||document.at("deposits").size()>((uint64_t(1)<<32)-1))
  throw std::invalid_argument("invalid deposit history snapshot");
 ExecutionDepositHistory result(expected_hash);result.cold_storage_=std::move(context);result.check_storage(check);
 for(const auto& entry:document.at("deposits")){
  if(check)check();if(!entry.is_string()||entry.get_ref<const std::string&>().size()!=370)throw std::invalid_argument("invalid deposit history data width");
  const auto data=unhex(entry.get_ref<const std::string&>());if(data.size()!=184||hex(data)!=entry.get_ref<const std::string&>())throw std::invalid_argument("noncanonical deposit history data");
  Phase0Deposit deposit{};std::copy_n(data.begin(),48,deposit.message.public_key.begin());std::copy_n(data.begin()+48,32,deposit.message.withdrawal_credentials.begin());for(unsigned i=0;i<8;++i)deposit.message.amount_gwei|=uint64_t(data[80+i])<<(8*i);std::copy_n(data.begin()+88,96,deposit.signature.begin());
  auto path=insert(result.tree_,32,result.count_,deposit,check,result.cold_storage_);
  result.tree_=result.cold_storage_?Node::persist(path,32,result.cold_storage_,result.cold_storage_,check):std::move(path);++result.count_;
 }
 validate_view(result,state,header,check);result.check_storage(check);return result;
}

}
