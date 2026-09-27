#pragma once
#include "chain.hpp"
#include "beacon_sync.hpp"
#include "admission_budget.hpp"
#include <functional>
#include "wallet.hpp"
#include "bft_reactor.hpp"
#include "block_import.hpp"
class AltairRpcService;
class BellatrixRpcService;
namespace nano {
struct CapellaRuntime;
class RpcError : public std::runtime_error {
public:
 int code;Json data;
 RpcError(int code,std::string message,Json data=nullptr):std::runtime_error(std::move(message)),code(code),data(std::move(data)){}
};
struct RpcSimulationLimits {
 std::chrono::milliseconds timeout{5000};
 std::function<std::chrono::steady_clock::time_point()> now=std::chrono::steady_clock::now;
};
struct RpcStream { std::string id,type;Json criteria;uint64_t cursor; };
struct RpcFeePolicy {
 size_t history_blocks=20,samples_per_block=3;
 unsigned percentile=60;
 Int fallback_tip=0;
 std::optional<Int> maximum_tip;
 static RpcFeePolicy from_json(const Json& config);
};
class Rpc {
 Blockchain& chain_;
 std::shared_ptr<::AltairRpcService> altair_;
 std::shared_ptr<::BellatrixRpcService> bellatrix_;
 std::shared_ptr<const CapellaRuntime> capella_;
 RpcSimulationLimits simulation_limits_;
 std::function<AdmissionBudget::Clock::time_point()> admission_now_;
 AdmissionBudget admission_budget_;
 void charge_transactions(size_t count){if(!admission_budget_.consume(count,admission_now_()))throw RpcError(-32005,"Transaction admission rate limit exceeded");}
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
 using Estimate = std::function<Json(const Json&)>;
 Json account_request(const std::string& method,const Json& params,const Estimate& estimate={});
 BlockImports block_imports_;
 std::string network_token_;
 std::shared_ptr<BeaconSyncTracker> beacon_sync_;
 Json network_report_=Json::object();
 std::chrono::steady_clock::time_point network_seen_{};
 Json network_status() const;
 Json network_block(const Block& block,bool full=true) const;
 Json fee_history(const Json& params) const;
 RpcFeePolicy fee_policy_;
 std::optional<Bytes> fee_head_;
 Int fee_tip_=0;
 Int suggested_tip(const Header& head);
 std::recursive_mutex mutex_;
 uint64_t next_filter_=0;
 std::map<std::string,RpcStream> filters_;
 Json logs(const Json& criteria,const std::optional<Int>& only=std::nullopt) const;
 std::optional<Block> block(const Json& tag) const;
 std::function<Json()> prepare_simulation(const std::string& method,const Json& params,const std::function<void()>& cancelled={});
 Json dispatch(const std::string& method,const Json& params,const Estimate& estimate={});
 Json one(const Json& request,const std::function<void()>& cancelled);
public:
 explicit Rpc(Blockchain& chain,std::string network_token={},std::function<AdmissionBudget::Clock::time_point()> now=AdmissionBudget::Clock::now,RpcSimulationLimits simulation_limits={},std::shared_ptr<::AltairRpcService> altair={},std::shared_ptr<::BellatrixRpcService> bellatrix={},std::shared_ptr<const CapellaRuntime> capella={}):chain_(chain),altair_(std::move(altair)),bellatrix_(std::move(bellatrix)),capella_(std::move(capella)),simulation_limits_(std::move(simulation_limits)),admission_now_(std::move(now)),admission_budget_(admission_now_()),network_token_(std::move(network_token)){if(unsigned(bool(altair_))+unsigned(bool(bellatrix_))+unsigned(bool(capella_))>1)throw std::invalid_argument("Choose one consensus RPC service");chain_.require_execution_ready();}
 void set_beacon_sync_tracker(std::shared_ptr<BeaconSyncTracker> tracker){std::lock_guard lock(mutex_);beacon_sync_=std::move(tracker);}
 bool requires_private_bind()const{return bool(altair_)||bool(bellatrix_)||bool(capella_);}
 bool local_only(){std::lock_guard lock(mutex_);return bool(altair_)||bool(bellatrix_)||bool(capella_)||dev_mode_||!wallets_.addresses().empty();}
 bool has_wallets(){std::lock_guard lock(mutex_);return !wallets_.addresses().empty();}
 void validator_tick();
 void load_wallets(const Json& config,const std::string& bind_address);
 RpcStream stream(const std::string& type,const Json& criteria=Json::object());
 Json poll(RpcStream& stream);
 // Null means notification: no JSON-RPC response should be sent.
 Json handle(const Json& request,const std::function<void()>& cancelled={});
 Json parse(const std::string& request);
};
}
