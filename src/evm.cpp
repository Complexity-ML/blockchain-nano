#include "nano/evm.hpp"
#include "nano/precompiles.hpp"
namespace nano {
namespace {
struct Revert { Bytes data; };
struct Frame {
 uint64_t gas;
 std::vector<Int> stack;
 Bytes memory;
 explicit Frame(uint64_t budget):gas(budget){}
 void charge(uint64_t cost){if(cost>gas)throw std::runtime_error("Out of gas");gas-=cost;}
 void push(const Int& value){if(stack.size()>=1024)throw std::runtime_error("Stack overflow");stack.push_back(word(value));}
 Int pop(){if(stack.empty())throw std::runtime_error("Stack underflow");Int value=stack.back();stack.pop_back();return value;}
 size_t offset(const Int& value){if(value>1048576)throw std::runtime_error("Out of gas");return value.get_ui();}
 std::pair<size_t,size_t> region(const Int& start,const Int& size){if(size==0)return {0,0};size_t o=offset(start),n=offset(size);if(n>1048576-o)throw std::runtime_error("Out of gas");size_t end=o+n;if(end>memory.size()){auto old=(memory.size()+31)/32,next=(end+31)/32;charge(next*3+next*next/512-old*3-old*old/512);memory.resize(next*32);}return {o,n};}
 Bytes read(const Int& start,const Int& size){auto [o,n]=region(start,size);return Bytes(memory.begin()+o,memory.begin()+o+n);}
 void write(const Int& start,const Bytes& b){auto [o,n]=region(start,Int(b.size()));std::copy(b.begin(),b.end(),memory.begin()+o);}
};
Bytes slice(const Bytes& input,const Int& start,size_t size){Bytes out(size);if(start>=Int(input.size()))return out;size_t off=start.get_ui();std::copy_n(input.begin()+off,std::min(size,input.size()-off),out.begin());return out;}
std::string evm_address(const Int& value){return hex(bytes(Int(value&((Int(1)<<160)-1)),20));}
}
ExecutionResult EVM::execute(const ExecutionContext& ctx){warm_addresses_={address(ctx.origin),address(ctx.caller),address(ctx.address)};for(unsigned i=1;i<=9;++i)warm_addresses_.insert(evm_address(Int(i)));if(ctx.revision==EvmRevision::Shanghai)warm_addresses_.insert(address(ctx.coinbase));warm_slots_.clear();original_storage_.clear();pending_deletions_.clear();for(const auto& access:ctx.access_list){auto account=address(access.address);warm_addresses_.insert(account);for(const auto& key:access.keys){if(key.size()!=32)throw std::invalid_argument("access-list storage key must be 32 bytes");warm_slots_.insert({account,integer(key)});}}auto result=run(ctx);if(result.success){result.deleted_accounts=pending_deletions_;for(const auto& account:pending_deletions_)state_.erase(account);}return result;}
ExecutionResult EVM::run(const ExecutionContext& ctx, unsigned depth){
 Int target=integer(unhex(ctx.code_address.value_or(ctx.address)));if(target>=1&&target<=(ctx.revision==EvmRevision::NanoExperimental?10:9))return precompile(unsigned(target.get_ui()),ctx.data,ctx.gas);
 Frame f(ctx.gas);ExecutionResult result;Bytes last_return;size_t snap=state_.snapshot();
 try {
 std::set<size_t> jumpdests;for(size_t i=0;i<ctx.code.size();++i){auto op=ctx.code[i];if(op==0x5b)jumpdests.insert(i);if(op>=0x60&&op<=0x7f)i+=op-0x5f;}
 auto warm=[&](const std::string& a){if(warm_addresses_.insert(a).second)f.charge(2500);};
 bool done=false;
 for(size_t pc=0;pc<ctx.code.size()&&!done;++pc){uint8_t op=ctx.code[pc];
 if(((ctx.revision==EvmRevision::Berlin||ctx.revision==EvmRevision::Paris)&&op==0x5f)||(ctx.revision==EvmRevision::Berlin&&op==0x48))throw std::runtime_error("Opcode unavailable in selected EVM revision");
 f.charge(opcode_gas(op));
 if(op>=0x60&&op<=0x7f){size_t n=op-0x5f;f.push(integer(slice(ctx.code,Int(pc+1),n)));pc+=n;continue;}
 if(op>=0x80&&op<=0x8f){size_t depth=op-0x7f;if(f.stack.size()<depth)throw std::runtime_error("Stack underflow");Int value=f.stack[f.stack.size()-depth];f.push(value);continue;}
 if(op>=0x90&&op<=0x9f){size_t depth=op-0x8f;if(f.stack.size()<=depth)throw std::runtime_error("Stack underflow");std::swap(f.stack.back(),f.stack[f.stack.size()-depth-1]);continue;}
 if(op>=0xa0&&op<=0xa4){if(ctx.is_static)throw std::runtime_error("LOG in static context");Int off=f.pop(),size=f.pop();Log log{ctx.address,{}, {}};for(unsigned i=0;i<unsigned(op-0xa0);++i)log.topics.push_back(bytes(f.pop(),32));log.data=f.read(off,size);f.charge(log.data.size()*8);result.logs.push_back(std::move(log));continue;}
 switch(static_cast<NanoOpCodes>(op)){
 case NanoOpCodes::STOP:done=true;break;
 case NanoOpCodes::ADD:{Int a=f.pop(),b=f.pop();f.push(a+b);break;}
 case NanoOpCodes::MUL:{Int a=f.pop(),b=f.pop();f.push(a*b);break;}
 case NanoOpCodes::SUB:{Int a=f.pop(),b=f.pop();f.push(a-b);break;}
 case NanoOpCodes::DIV:{Int a=f.pop(),b=f.pop();f.push(b==0?Int(0):Int(a/b));break;}
 case NanoOpCodes::SDIV:{Int a=signed_word(f.pop()),b=signed_word(f.pop());f.push(b==0?Int(0):Int(a/b));break;}
 case NanoOpCodes::MOD:{Int a=f.pop(),b=f.pop();f.push(b==0?Int(0):Int(a%b));break;}
 case NanoOpCodes::SMOD:{Int a=signed_word(f.pop()),b=signed_word(f.pop());f.push(b==0?Int(0):Int(a%b));break;}
 case NanoOpCodes::ADDMOD:{Int a=f.pop(),b=f.pop(),n=f.pop();f.push(n==0?Int(0):Int((a+b)%n));break;}
 case NanoOpCodes::MULMOD:{Int a=f.pop(),b=f.pop(),n=f.pop();f.push(n==0?Int(0):Int((a*b)%n));break;}
 case NanoOpCodes::EXP:{Int a=f.pop(),b=f.pop(),out,mod=Int(1)<<256;f.charge(bytes(b).size()*50);mpz_powm(out.get_mpz_t(),a.get_mpz_t(),b.get_mpz_t(),mod.get_mpz_t());f.push(out);break;}
 case NanoOpCodes::SIGNEXTEND:{Int b=f.pop(),x=f.pop();if(b<31){unsigned bit=b.get_ui()*8+7;Int mask=(Int(1)<<bit)-1;f.push(mpz_tstbit(x.get_mpz_t(),bit)?Int(x|~mask):Int(x&mask));}else f.push(x);break;}
 case NanoOpCodes::LT:{Int a=f.pop(),b=f.pop();f.push(Int(a<b));break;}
 case NanoOpCodes::GT:{Int a=f.pop(),b=f.pop();f.push(Int(a>b));break;}
 case NanoOpCodes::SLT:{Int a=signed_word(f.pop()),b=signed_word(f.pop());f.push(Int(a<b));break;}
 case NanoOpCodes::SGT:{Int a=signed_word(f.pop()),b=signed_word(f.pop());f.push(Int(a>b));break;}
 case NanoOpCodes::EQ:{Int a=f.pop(),b=f.pop();f.push(Int(a==b));break;}
 case NanoOpCodes::ISZERO:f.push(Int(f.pop()==0));break;
 case NanoOpCodes::AND:{Int a=f.pop(),b=f.pop();f.push(a&b);break;}
 case NanoOpCodes::OR:{Int a=f.pop(),b=f.pop();f.push(a|b);break;}
 case NanoOpCodes::XOR:{Int a=f.pop(),b=f.pop();f.push(a^b);break;}
 case NanoOpCodes::NOT:f.push(~f.pop());break;
 case NanoOpCodes::BYTE:{Int i=f.pop(),x=f.pop();f.push(i>=32?Int(0):Int((x>>(248-i.get_ui()*8))&255));break;}
 case NanoOpCodes::SHL:{Int shift=f.pop(),x=f.pop();f.push(shift>=256?Int(0):Int(x<<shift.get_ui()));break;}
 case NanoOpCodes::SHR:{Int shift=f.pop(),x=f.pop();f.push(shift>=256?Int(0):Int(x>>shift.get_ui()));break;}
 case NanoOpCodes::SAR:{Int shift=f.pop(),x=signed_word(f.pop());f.push(shift>=256?Int(x<0?-1:0):Int(x>>shift.get_ui()));break;}
 case NanoOpCodes::KECCAK256:{Int off=f.pop(),size=f.pop();auto data=f.read(off,size);f.charge(6*((data.size()+31)/32));f.push(integer(keccak(data)));break;}
 case NanoOpCodes::ADDRESS:f.push(integer(unhex(ctx.address)));break;
 case NanoOpCodes::ORIGIN:f.push(integer(unhex(ctx.origin)));break;
 case NanoOpCodes::CALLER:f.push(integer(unhex(ctx.caller)));break;
 case NanoOpCodes::CALLVALUE:f.push(ctx.value);break;
 case NanoOpCodes::CALLDATALOAD:f.push(integer(slice(ctx.data,f.pop(),32)));break;
 case NanoOpCodes::CALLDATASIZE:f.push(Int(ctx.data.size()));break;
 case NanoOpCodes::CODESIZE:f.push(Int(ctx.code.size()));break;
 case NanoOpCodes::GASPRICE:f.push(ctx.gas_price);break;
 case NanoOpCodes::CALLDATACOPY:
 case NanoOpCodes::CODECOPY:{Int dest=f.pop(),off=f.pop(),size=f.pop();auto [o,n]=f.region(dest,size);f.charge(3*((n+31)/32));f.write(Int(o),slice(op==0x37?ctx.data:ctx.code,off,n));break;}
 case NanoOpCodes::BALANCE:{auto a=evm_address(f.pop());warm(a);f.push(state_.get(a).balance);break;}
 case NanoOpCodes::EXTCODESIZE:{auto a=evm_address(f.pop());warm(a);f.push(Int(state_.get(a).code.size()));break;}
 case NanoOpCodes::EXTCODEHASH:{auto a=evm_address(f.pop());warm(a);f.push(state_.exists(a)?integer(keccak(state_.get(a).code)):Int(0));break;}
 case NanoOpCodes::EXTCODECOPY:{auto a=evm_address(f.pop());Int dest=f.pop(),off=f.pop(),size=f.pop();warm(a);auto [o,n]=f.region(dest,size);f.charge(3*((n+31)/32));f.write(Int(o),slice(state_.get(a).code,off,n));break;}
 case NanoOpCodes::RETURNDATASIZE:f.push(Int(last_return.size()));break;
 case NanoOpCodes::RETURNDATACOPY:{Int dest=f.pop(),off=f.pop(),size=f.pop();if(off+size>Int(last_return.size()))throw std::runtime_error("Return data out of bounds");auto [o,n]=f.region(dest,size);f.charge(3*((n+31)/32));f.write(Int(o),slice(last_return,off,n));break;}
 case NanoOpCodes::BLOCKHASH:{
  Int number=f.pop();
  if(number>=ctx.number||ctx.number-number>256||!ctx.block_hash)f.push(Int(0));
  else{auto hash=ctx.block_hash(number);if(hash.size()!=32)throw std::runtime_error("invalid ancestor hash");f.push(integer(hash));}
  break;
 }
 case NanoOpCodes::COINBASE:f.push(integer(unhex(ctx.coinbase)));break;
 case NanoOpCodes::TIMESTAMP:f.push(ctx.timestamp);break;
 case NanoOpCodes::NUMBER:f.push(ctx.number);break;
 case NanoOpCodes::PREVRANDAO:f.push(ctx.prevrandao);break;
 case NanoOpCodes::GASLIMIT:f.push(ctx.gas_limit);break;
 case NanoOpCodes::CHAINID:f.push(ctx.chain_id);break;
 case NanoOpCodes::SELFBALANCE:f.push(state_.get(ctx.address).balance);break;
 case NanoOpCodes::BASEFEE:f.push(ctx.base_fee);break;
 case NanoOpCodes::POP:f.pop();break;
 case NanoOpCodes::MLOAD:{auto off=f.pop();f.push(integer(f.read(off,Int(32))));break;}
 case NanoOpCodes::MSTORE:{Int off=f.pop(),value=f.pop();f.write(off,bytes(value,32));break;}
 case NanoOpCodes::MSTORE8:{Int off=f.pop(),value=f.pop();f.write(off,Bytes{uint8_t(value.get_ui()&255)});break;}
 case NanoOpCodes::SLOAD:{Int key=f.pop();if(warm_slots_.insert({address(ctx.address),key}).second)f.charge(2000);f.push(state_.get_storage(ctx.address,key));break;}
 case NanoOpCodes::SSTORE:{
  if(ctx.is_static)throw std::runtime_error("SSTORE in static context");
  // EIP-2200's stipend guard precedes the cold-slot surcharge.
  if(f.gas<=2300)throw std::runtime_error("Out of gas");
  Int clear_refund=ctx.revision==EvmRevision::Berlin?15000:4800;
  Int key=f.pop(),value=f.pop();auto slot=std::make_pair(address(ctx.address),key);
  Int current=state_.get_storage(ctx.address,key);
  const Int& original=original_storage_.try_emplace(slot,current).first->second;
  if(warm_slots_.insert(slot).second)f.charge(2100);
  if(current==value)f.charge(100);
  else if(original==current){
   f.charge(original==0?20000:2900);
   if(original!=0&&value==0)result.gas_refund+=clear_refund;
  }else{
   f.charge(100);
   if(original!=0){if(current==0)result.gas_refund-=clear_refund;if(value==0)result.gas_refund+=clear_refund;}
   if(original==value)result.gas_refund+=original==0?19900:2800;
  }
  state_.set_storage(ctx.address,key,value);break;
 }
 case NanoOpCodes::JUMP:
 case NanoOpCodes::JUMPI:{Int dest=f.pop(),condition=op==0x57?f.pop():Int(1);if(condition!=0){if(dest>=Int(ctx.code.size())||!jumpdests.contains(dest.get_ui()))throw std::runtime_error("Invalid jump destination");pc=dest.get_ui()-1;}break;}
 case NanoOpCodes::PC:f.push(Int(pc));break;
 case NanoOpCodes::MSIZE:f.push(Int(f.memory.size()));break;
 case NanoOpCodes::GAS:f.push(Int(std::to_string(f.gas)));break;
 case NanoOpCodes::JUMPDEST:break;
 case NanoOpCodes::PUSH0:f.push(Int(0));break;
 case NanoOpCodes::CALL:
 case NanoOpCodes::CALLCODE:
 case NanoOpCodes::DELEGATECALL:
 case NanoOpCodes::STATICCALL:{
  Int requested=f.pop();auto target=evm_address(f.pop());bool delegate=op==0xf4,static_call=op==0xfa,callcode=op==0xf2;
  Int value=(delegate||static_call)?Int(0):f.pop();
  Int args_off=f.pop(),args_size=f.pop(),out_off=f.pop(),out_size=f.pop();
  if(ctx.is_static&&value!=0)throw std::runtime_error("Value transfer in static context");
  auto input=f.read(args_off,args_size);auto [output_offset,output_size]=f.region(out_off,out_size);
  bool cold=warm_addresses_.insert(target).second;
  if(cold)f.charge(2500);
  if(value!=0){f.charge(9000);if(!callcode&&state_.get(target).empty())f.charge(25000);}
  last_return.clear();
  if(depth>=1023||state_.get(ctx.address).balance<value){if(value!=0)f.gas+=2300;f.push(Int(0));break;}
  uint64_t cap=f.gas-f.gas/64;uint64_t budget=requested>Int(std::to_string(cap))?cap:u64(requested);
  uint64_t stipend=value!=0?2300:0;
  ExecutionContext child=ctx;child.address=delegate||callcode?ctx.address:target;child.caller=delegate?ctx.caller:ctx.address;
  child.value=delegate?ctx.value:value;child.data=std::move(input);child.code=state_.get(target).code;child.code_address=target;child.gas=budget+stipend;child.is_static=ctx.is_static||static_call;
  size_t transfer_snap=state_.snapshot();auto previous_addresses=warm_addresses_;auto previous_slots=warm_slots_;auto previous_deletions=pending_deletions_;
  if(!delegate&&!callcode&&!static_call&&value!=0)state_.transfer(ctx.address,target,value);
  f.charge(budget);warm_addresses_.insert(child.address);auto output=run(child,depth+1);
  f.gas+=child.gas-output.gas_used;last_return=output.return_data;
  if(output.success){state_.commit(transfer_snap);result.logs.insert(result.logs.end(),output.logs.begin(),output.logs.end());result.gas_refund+=output.gas_refund;}
  else{state_.revert(transfer_snap);warm_addresses_=std::move(previous_addresses);warm_slots_=std::move(previous_slots);pending_deletions_=std::move(previous_deletions);}
  size_t count=std::min(output_size,last_return.size());std::copy_n(last_return.begin(),count,f.memory.begin()+output_offset);
  f.push(Int(output.success));break;
 }
 case NanoOpCodes::CREATE:
 case NanoOpCodes::CREATE2:{
  if(ctx.is_static)throw std::runtime_error("CREATE in static context");
  Int value=f.pop(),off=f.pop(),size=f.pop();Int salt=op==0xf5?f.pop():Int(0);if(ctx.revision==EvmRevision::Shanghai&&size>49152)throw std::runtime_error("initcode size exceeds limit");auto init=f.read(off,size);last_return.clear();
  if(ctx.revision==EvmRevision::Shanghai)f.charge(2*((init.size()+31)/32));
  if(op==0xf5)f.charge(6*((init.size()+31)/32));
  if(depth>=1023||state_.get(ctx.address).balance<value||state_.get(ctx.address).nonce>=Int("18446744073709551615")){f.push(Int(0));break;}
  auto nonce=state_.get(ctx.address).nonce;
  auto target=op==0xf5?create2_address(ctx.address,bytes(salt,32),keccak(init)):create_address(ctx.address,nonce);
  warm_addresses_.insert(target);
  state_.get(ctx.address).nonce+=1;
  if(state_.get(target).nonce!=0||!state_.get(target).code.empty()||(ctx.revision!=EvmRevision::NanoExperimental&&!state_.get(target).storage.empty())){f.charge(f.gas-f.gas/64);f.push(Int(0));break;}
  size_t create_snap=state_.snapshot();auto previous_addresses=warm_addresses_;auto previous_slots=warm_slots_;auto previous_deletions=pending_deletions_;
  state_.get(target).nonce=1;state_.transfer(ctx.address,target,value);
  ExecutionContext child=ctx;child.address=target;child.caller=ctx.address;child.value=value;child.data.clear();child.code=init;child.code_address=target;child.gas=f.gas-f.gas/64;
  warm_addresses_.insert(target);auto output=run(child,depth+1);f.charge(output.gas_used);last_return=output.return_data;
  uint64_t deposit=output.return_data.size()*200;
  if(output.success&&(output.return_data.size()>24576||deposit>child.gas-output.gas_used||((ctx.revision==EvmRevision::Paris||ctx.revision==EvmRevision::Shanghai)&&!output.return_data.empty()&&output.return_data[0]==0xef))){
   // Code deposit belongs to the child budget; preserve the parent's 1/64 reserve.
   f.charge(child.gas-output.gas_used);output.success=false;last_return.clear();
  }
  if(output.success){f.charge(deposit);state_.get(target).code=output.return_data;state_.commit(create_snap);result.logs.insert(result.logs.end(),output.logs.begin(),output.logs.end());result.gas_refund+=output.gas_refund;f.push(integer(unhex(target)));last_return.clear();}
  else{state_.revert(create_snap);warm_addresses_=std::move(previous_addresses);warm_slots_=std::move(previous_slots);pending_deletions_=std::move(previous_deletions);f.push(Int(0));}
  break;
 }
 case NanoOpCodes::RETURN:{Int off=f.pop(),size=f.pop();result.return_data=f.read(off,size);done=true;break;}
 case NanoOpCodes::REVERT:{Int off=f.pop(),size=f.pop();throw Revert{f.read(off,size)};}
 case NanoOpCodes::SELFDESTRUCT:{
  if(ctx.is_static)throw std::runtime_error("SELFDESTRUCT in static context");
  auto target=evm_address(f.pop());Int balance=state_.get(ctx.address).balance;
  if(ctx.revision==EvmRevision::NanoExperimental){state_.transfer(ctx.address,target,balance);state_.erase(ctx.address);}
  else{
   if(warm_addresses_.insert(target).second)f.charge(2600);
   if(balance!=0&&state_.get(target).empty())f.charge(25000);
   if(pending_deletions_.insert(address(ctx.address)).second&&ctx.revision==EvmRevision::Berlin)result.gas_refund+=24000;
   state_.get(target).balance+=balance;state_.get(ctx.address).balance=0;
  }
  // Code and storage remain visible until the transaction completes.
  done=true;break;
 }

 default:throw std::runtime_error("Unsupported opcode "+hex(Bytes{op}));
 }
 }
 state_.commit(snap);result.success=true;result.gas_used=ctx.gas-f.gas;return result;
 }catch(const Revert& r){state_.revert(snap);result.return_data=r.data;result.error="Execution reverted";result.gas_used=ctx.gas-f.gas;}
 catch(const std::exception& e){state_.revert(snap);result.return_data.clear();result.error=e.what();result.gas_used=ctx.gas;}
 result.success=false;result.logs.clear();result.gas_refund=0;return result;
}
}
