#pragma once
#include "core.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
namespace nano {
struct MiningResult { Bytes nonce,mix_hash;double hash_rate=0;uint64_t attempts=0; };
class ProofOfWork {
 std::atomic<bool> stopped_{false};
 std::atomic<double> hashrate_{0};
public:
 static Bytes hash(const Header& header);
 static bool verify(const Header& header);
 static Int adjust_difficulty(const Header& parent,const Int& timestamp);
 std::optional<MiningResult> mine(Header header,std::chrono::milliseconds timeout=std::chrono::milliseconds(0),uint64_t start_nonce=0,std::stop_token cancel={});
 double hashrate() const {return hashrate_.load();}
 void stop(){stopped_=true;}
};
// start/stop are controlled by one owner thread; callbacks run on the worker.
class Miner {
 ProofOfWork pow_;
 std::atomic<bool> running_{false};
 std::jthread worker_;
 mutable std::mutex error_mutex_;
 std::string error_;
public:
 ~Miner(){stop();}
 void start(std::function<std::optional<Block>()> get_block,std::function<void(const Block&)> submit_block);
 void stop();
 bool is_mining() const {return running_.load();}
 double hashrate() const {return pow_.hashrate();}
 std::string error() const {std::lock_guard lock(error_mutex_);return error_;}
};
struct HeaderValidation { bool valid;std::string error; };
HeaderValidation validate_pow_header(const Header& header,const std::optional<Header>& parent,const Int& now);

struct Validator {
 std::string address;
 Int stake,registered_at,locked_until,last_block=0,blocks_validated=0;
 bool active=true;
};
class ProofOfStake {
 std::vector<Validator> validators_;
 Validator* find(const std::string& addr);
public:
 static const Int minimum_stake,block_reward;
 static constexpr const char* bootstrap_validator="0x311384E7e4803AdBDB1D8ab46b973eeE0f9cF55B";
 bool bootstrap=true;
 bool register_validator(const std::string& addr,const Int& stake,const Int& block);
 bool add_stake(const std::string& addr,const Int& amount,const Int& block);
 std::optional<Int> withdraw(const std::string& addr,const Int& block);
 std::optional<std::string> select(const Int& block,const Bytes& hash) const;
 bool validate(const Header& h,const std::string& addr) const;
 void reward(const std::string& addr,const Int& block);
 void slash(const std::string& addr);
 Int total_stake() const;
 Int stake(const std::string& addr) const;
 Json validators() const;
 Json to_json() const;
 static ProofOfStake from_json(const Json& data);
};
}
