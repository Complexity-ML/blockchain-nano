#pragma once
#include "core.hpp"
#include "opcodes.hpp"
#include <set>
#include <map>
#include <cstddef>
#include <functional>
#include <coroutine>
#include <exception>
#include <utility>
namespace nano {
enum class EvmRevision { NanoExperimental, Berlin, Paris, Shanghai, Cancun, Prague, Osaka };
inline Int block_randomness(const Header& header,EvmRevision revision){return revision>=EvmRevision::Paris?integer(header.mix_hash):header.difficulty;}
inline bool is_precompile(const Int& target,EvmRevision revision){return (target>=1&&target<=(revision>=EvmRevision::Prague?17:revision>=EvmRevision::Cancun?10:9))||(revision>=EvmRevision::Osaka&&target==256);}
Int blob_base_fee(const Int& excess,EvmRevision revision=EvmRevision::Cancun);
Int transaction_floor_gas(const Transaction& tx,EvmRevision revision);
Int transaction_intrinsic_gas(const Transaction& tx,EvmRevision revision);
struct ExecutionCancelled : std::runtime_error {using std::runtime_error::runtime_error;};
// Host-only observer quota: never turn an incomplete access list into a VM result.
struct EvmAccessListLimitExceeded : ExecutionCancelled {using ExecutionCancelled::ExecutionCancelled;};
// One caller-owned monotone trace. Warm-set/state rollback never removes entries.
// Seed and exclusions are normalized and owned before execution can call the host.
class EvmAccessListTrace {
 friend class EVM;
 std::map<std::string,std::set<Bytes>> entries_;
 std::set<std::string> excluded_;
 size_t maximum_addresses_,maximum_storage_keys_,storage_keys_=0;
 bool limited_=false;
 [[noreturn]] void limit();
 void record_address(const std::string&);
 void record_storage(const std::string&,const Bytes&);
public:
 EvmAccessListTrace(const std::vector<Access>& seed,const std::set<std::string>& excluded,
  size_t maximum_addresses,size_t maximum_storage_keys);
 EvmAccessListTrace(const EvmAccessListTrace&)=delete;
 EvmAccessListTrace& operator=(const EvmAccessListTrace&)=delete;
 std::vector<Access> access_list() const;
 size_t address_count() const noexcept{return entries_.size();}
 size_t storage_key_count() const noexcept{return storage_keys_;}
};
struct ExecutionContext {
 EvmRevision revision=EvmRevision::NanoExperimental;
 std::string origin="0x0000000000000000000000000000000000000000",caller=origin,address=origin,coinbase=origin;
 Int gas_price=0,value=0,timestamp=0,number=0,prevrandao=0,gas_limit=30000000,chain_id=1337,base_fee=0;
 Int blob_base_fee=0;
 std::vector<Bytes> blob_hashes;
 uint64_t gas=30000000;
 Bytes data,code;
 std::vector<Access> access_list;
 std::set<std::string> authorization_warm_addresses;
 std::function<Bytes(const Int&)> block_hash;
 // Optional host-only cancellation. Consensus execution leaves this empty.
 std::function<void()> checkpoint;
 std::optional<std::string> code_address;
 bool is_static=false;
 bool contract_creation=false;
};
struct ExecutionResult {
 bool success=false;
 uint64_t gas_used=0;
 Int gas_refund=0;
 Bytes return_data;
 std::vector<Log> logs;
 std::set<std::string> deleted_accounts;
 std::string error;
 // Observed forwarding depth is an RPC probe hint, never serialized state/gas.
 unsigned max_call_depth=0;
};
// Symmetric coroutine transfer keeps nested EVM frames off the native stack.
// Tasks execute synchronously; there are no worker threads or scheduler races.
class EvmTask {
public:
 struct promise_type {
  ExecutionResult result;
  std::exception_ptr exception;
  std::coroutine_handle<> continuation=std::noop_coroutine();
  EvmTask get_return_object(){return EvmTask{std::coroutine_handle<promise_type>::from_promise(*this)};}
  std::suspend_always initial_suspend() noexcept{return {};}
  struct Completion {
   bool await_ready() noexcept{return false;}
   std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task) noexcept{return task.promise().continuation;}
   void await_resume() noexcept{}
  };
  Completion final_suspend() noexcept{return {};}
  void return_value(ExecutionResult value){result=std::move(value);}
  void unhandled_exception(){exception=std::current_exception();}
 };
private:
 std::coroutine_handle<promise_type> handle_;
public:
 explicit EvmTask(std::coroutine_handle<promise_type> handle):handle_(handle){}
 EvmTask(const EvmTask&)=delete;
 EvmTask(EvmTask&& other) noexcept:handle_(std::exchange(other.handle_,{})){}
 ~EvmTask(){if(handle_)handle_.destroy();}
 bool await_ready() const noexcept{return false;}
 std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent) noexcept{handle_.promise().continuation=parent;return handle_;}
 ExecutionResult await_resume(){if(handle_.promise().exception)std::rethrow_exception(handle_.promise().exception);return std::move(handle_.promise().result);}
 ExecutionResult execute(){handle_.resume();if(!handle_.done())throw std::logic_error("EVM task did not complete");return await_resume();}
};
class EVM {
 const State& state_;
 State* writable_state_=nullptr;
 State& mutable_state(){if(!writable_state_)throw std::logic_error("EVM state is read-only");return *writable_state_;}
 std::set<std::string> pending_deletions_;
 std::set<std::string> created_accounts_;
 std::map<std::pair<std::string,Int>,Int> transient_storage_;
 std::set<std::string> warm_addresses_;
 std::set<std::pair<std::string,Int>> warm_slots_;
 std::map<std::pair<std::string,Int>,Int> original_storage_;
 EvmTask run(const ExecutionContext& ctx,unsigned depth,EvmAccessListTrace* trace);
 ExecutionResult execute_impl(const ExecutionContext&,EvmAccessListTrace*);
public:
 explicit EVM(State& state):state_(state),writable_state_(&state){}
 // A const state accepts only static execution and cannot be written.
 explicit EVM(const State& state):state_(state){}
 ExecutionResult execute(const ExecutionContext& ctx);
 // Synchronous borrowed observer, propagated explicitly to every nested frame.
 ExecutionResult execute(const ExecutionContext& ctx,EvmAccessListTrace& trace);
};
}
