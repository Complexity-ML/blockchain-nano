#pragma once
#include "chain.hpp"
#include "wallet.hpp"
#include "bft_reactor.hpp"
namespace nano {
class RpcError : public std::runtime_error {
public:
 int code;Json data;
 RpcError(int code,std::string message,Json data=nullptr):std::runtime_error(std::move(message)),code(code),data(std::move(data)){}
};
struct RpcStream { std::string id,type;Json criteria;uint64_t cursor; };
class Rpc {
 Blockchain& chain_;
 std::unique_ptr<BftReactor> bft_;
 KeyStore wallets_;
 bool dev_mode_=false;
 bool validating_=false;
 uint64_t block_time_=12;
 std::chrono::steady_clock::time_point validator_due_{};
 std::string validator_error_;

 Int time_offset_=0;
 std::optional<Int> next_timestamp_;
 std::string coinbase_="0x0000000000000000000000000000000000000000";
 Int development_timestamp() const;
 struct DevSnapshot {Int number;Bytes hash;Int time_offset;std::optional<Int> next_timestamp;};
 uint64_t next_snapshot_=0;
 std::map<uint64_t,DevSnapshot> snapshots_;
 Json account_request(const std::string& method,const Json& params);
 std::string network_token_;
 Json network_report_=Json::object();
 std::chrono::steady_clock::time_point network_seen_{};
 Json network_status() const;
 Json network_block(const Block& block) const;
 Json fee_history(const Json& params) const;
 std::recursive_mutex mutex_;
 uint64_t next_filter_=0;
 std::map<std::string,RpcStream> filters_;
 Json logs(const Json& criteria,const std::optional<Int>& only=std::nullopt) const;
 std::optional<Block> block(const Json& tag) const;
 Json dispatch(const std::string& method,const Json& params);
 Json one(const Json& request);
public:
 explicit Rpc(Blockchain& chain,std::string network_token={}):chain_(chain),network_token_(std::move(network_token)){}
 bool local_only(){std::lock_guard lock(mutex_);return dev_mode_||!wallets_.addresses().empty();}
 bool has_wallets(){std::lock_guard lock(mutex_);return !wallets_.addresses().empty();}
 void validator_tick();
 void load_wallets(const Json& config,const std::string& bind_address);
 RpcStream stream(const std::string& type,const Json& criteria=Json::object());
 Json poll(RpcStream& stream);
 // Null means notification: no JSON-RPC response should be sent.
 Json handle(const Json& request);
 Json parse(const std::string& request);
};
}
