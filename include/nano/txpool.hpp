#pragma once
#include "core.hpp"
#include <mutex>
#include <set>
namespace nano {
class TxPool {
 struct Entry {SignedTransaction tx;std::string sender;uint64_t arrival;};
 mutable std::mutex mutex_;
 std::map<std::string,Entry> entries_;
 std::set<std::string> known_;
 size_t max_size_,max_per_account_;
 Int minimum_price_;
 uint64_t arrival_=0;
 static Int price(const Transaction& tx);
public:
 explicit TxPool(size_t max_size=10000,size_t max_per_account=100,Int minimum_price=1);
 bool add(const SignedTransaction& tx);
 std::optional<SignedTransaction> get(const std::string& hash) const;
 bool remove(const std::string& hash);
 void remove_mined(const std::vector<SignedTransaction>& txs);
 std::vector<SignedTransaction> pending_for_block(const Int& gas_limit) const;
 std::vector<SignedTransaction> by_sender(const std::string& addr) const;
 Int pending_nonce(const std::string& addr) const;
 size_t size() const;
 Json content() const;
 Json status() const;
 void clear();
};
}
