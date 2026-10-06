#pragma once
#include "rpc.hpp"
namespace nano {
class RpcClient {
 std::vector<std::string> nodes_;
 size_t current_=0;
 uint64_t request_id_=0;
 std::chrono::milliseconds timeout_;
 unsigned retries_;
 std::optional<Int> pinned_chain_;
 Bytes pinned_genesis_;
 mutable std::mutex mutex_;
public:
 RpcClient(std::vector<std::string> nodes,std::chrono::milliseconds timeout=std::chrono::seconds(10),unsigned retries=3);
 void pin_chain(const Int& chain_id,const Bytes& genesis_hash);
 Json call(const std::string& method,Json params=Json::array());
 // Braced arguments must stay positional, including a single JSON object.
 Json call(const std::string& method,std::initializer_list<Json> params){auto array=Json::array();for(const auto& value:params)array.push_back(value);return call(method,std::move(array));}
 std::string current_node() const {std::lock_guard lock(mutex_);return nodes_.at(current_);}
};
class ValidatorStats {
 std::string address_;
 uint64_t blocks_=0;
 Int rewards_=0,fees_=0;
 std::chrono::steady_clock::time_point started_=std::chrono::steady_clock::now();
 std::optional<int64_t> last_block_;
public:
 explicit ValidatorStats(const std::string& validator):address_(address(validator)){}
 void record(const Json& result);
 Json to_json(const std::string& current_node,bool running) const;
};
Int nano_amount(const std::string& input);
Json submit_staking(RpcClient& client,const Wallet& wallet,const std::string& target,const std::string& operation,const Int& value);
int rpc_command(int argc,char** argv);
Json validate_once(RpcClient& client,const std::string& validator,const Wallet* signer=nullptr,const Json& signing_config=Json::object());
Json validator_environment(const Json& config);
int run_validator(const Json& config,bool once);
}
