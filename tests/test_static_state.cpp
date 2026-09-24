#include "support/process.hpp"
#include "nano/evm.hpp"
using namespace test;
int main(){try{
 const std::string parent="0x0000000000000000000000000000000000001234",child="0x0000000000000000000000000000000000005678",absent="0x0000000000000000000000000000000000009876";
 State state;state.get(parent).balance=100;state.set_storage(parent,0,42);state.get(child).code=unhex("0x602a60005260206000f3");
 const State& readonly=state;ExecutionContext ctx;ctx.revision=EvmRevision::Cancun;ctx.address=parent;ctx.gas=1000000;ctx.is_static=true;
 unsigned checked=0;
 auto execute=[&](const Bytes& code){auto before=state.to_json();ctx.code=code;auto result=EVM(readonly).execute(ctx);check(state.to_json()==before,"static call changed account map");++checked;return result;};
 check(integer(execute(unhex("0x60005460005260206000f3")).return_data)==42,"read storage");
 for(uint8_t opcode:{0x31,0x3b,0x3f}){auto code=concat(Bytes{0x73},unhex(absent));code.push_back(opcode);check(execute(code).success,"absent account read");}
 auto call=[&](uint8_t opcode,unsigned value){Bytes code=unhex("0x6020600060006000");if(opcode==0xf1||opcode==0xf2)code=concat(code,Bytes{0x60,uint8_t(value)});code=concat(code,concat(Bytes{0x73},unhex(child)));code=concat(code,unhex("0x620186a0"));code.push_back(opcode);return code;};
 for(uint8_t opcode:{0xf1,0xf2,0xf4,0xfa})check(execute(concat(call(opcode,0),unhex("0x5060206000f3"))).return_data==bytes(Int(42),32),"nested static output");
 for(const char* code:{"0x6001600055","0x600160005d","0x60006000a0","0x600060006000f0","0x6000600060006000f5","0x6000ff"})check(!execute(unhex(code)).success,"static write opcode rejected");
 check(!execute(call(0xf1,1)).success,"value transfer rejected");
 state.get(child).code=unhex("0x600960005560006000f3");for(uint8_t opcode:{0xf1,0xf2,0xf4,0xfa}){auto result=execute(concat(call(opcode,0),unhex("0x60005260206000f3")));check(result.success&&integer(result.return_data)==0,"nested write failure isolated");}
 state.get(child).code=unhex("0x60006000fd");check(integer(execute(concat(call(0xfa,0),unhex("0x60005260206000f3"))).return_data)==0,"nested revert");
 auto before=state.to_json();ctx.is_static=false;bool refused=false;try{EVM(readonly).execute(ctx);}catch(const std::invalid_argument&){refused=true;}check(refused&&state.to_json()==before,"read-only EVM rejects mutable context");ctx.is_static=true;
 unsigned checkpoints=0;ctx.checkpoint=[&]{if(++checkpoints==4)throw ExecutionCancelled("test cancellation");};ctx.code=unhex("0x5b600056");bool cancelled=false;try{EVM(readonly).execute(ctx);}catch(const ExecutionCancelled&){cancelled=true;}check(cancelled&&state.to_json()==before,"cancellation preserves exact state");ctx.checkpoint={};
 // A mutable outer transaction keeps rollback even when its child is static.
 ctx.is_static=false;ctx.code=concat(unhex("0x6007600055"),concat(call(0xfa,0),unhex("0x5060006000fd")));auto result=EVM(state).execute(ctx);check(!result.success&&state.to_json()==before,"mutable parent rollback after static child");
 std::cout<<Json{{"scenario","evm-readonly-state"},{"staticCases",checked},{"cancellationIsolated",true},{"mutableParentRollback",true},{"mutableContextRejected",true}}.dump()<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
