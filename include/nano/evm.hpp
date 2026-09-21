#pragma once
#include "core.hpp"
#include "opcodes.hpp"
#include <set>
#include <functional>
namespace nano {
enum class EvmRevision { NanoExperimental, Berlin, Paris };
struct ExecutionContext {
 EvmRevision revision=EvmRevision::NanoExperimental;
 std::string origin="0x0000000000000000000000000000000000000000",caller=origin,address=origin,coinbase=origin;
 Int gas_price=0,value=0,timestamp=0,number=0,prevrandao=0,gas_limit=30000000,chain_id=1337,base_fee=0;
 uint64_t gas=30000000;
 Bytes data,code;
 std::vector<Access> access_list;
 std::function<Bytes(const Int&)> block_hash;
 std::optional<std::string> code_address;
 bool is_static=false;
};
struct ExecutionResult {
 bool success=false;
 uint64_t gas_used=0;
 Int gas_refund=0;
 Bytes return_data;
 std::vector<Log> logs;
 std::set<std::string> deleted_accounts;
 std::string error;
};
class EVM {
 State& state_;
 std::set<std::string> pending_deletions_;
 std::set<std::string> warm_addresses_;
 std::set<std::pair<std::string,Int>> warm_slots_;
 std::map<std::pair<std::string,Int>,Int> original_storage_;
 ExecutionResult run(const ExecutionContext& ctx, unsigned depth=0);
public:
 explicit EVM(State& state):state_(state){}
 ExecutionResult execute(const ExecutionContext& ctx);
};
}
