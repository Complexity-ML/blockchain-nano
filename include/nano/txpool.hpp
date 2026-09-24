#pragma once
#include "core.hpp"
#include <mutex>
#include <set>
#include <deque>
namespace nano {
class TxPool {
 struct Entry {SignedTransaction tx;std::string sender;uint64_t arrival;size_t raw_bytes;};
 mutable std::mutex mutex_;
 std::map<std::string,Entry> entries_;
 // Active hashes live in entries_; only recent mined hashes need a cache.
 std::set<std::string> mined_;
 std::deque<std::string> mined_order_;
 size_t max_size_,max_per_account_,max_bytes_,bytes_=0;
 Int minimum_price_;
 uint64_t arrival_=0;
 static Int price(const Transaction& tx);
public:
 explicit TxPool(size_t max_size=10000,size_t max_per_account=100,Int minimum_price=1,size_t max_bytes=64*1024*1024);
 bool add(const SignedTransaction& tx);
 std::optional<SignedTransaction> get(const std::string& hash) const;
 bool remove(const std::string& hash);
 void remove_stale(const State& state);
 void remove_mined(const std::vector<SignedTransaction>& txs);
 // Sender is authenticated at admission; execution still validates each signature.
 struct Candidate {SignedTransaction transaction;std::string sender;};
 std::vector<Candidate> candidates_for_block(const Int& gas_limit) const;
 std::vector<SignedTransaction> pending_for_block(const Int& gas_limit) const;
 std::vector<SignedTransaction> by_sender(const std::string& addr) const;
 Int reserved_cost(const std::string& sender,const Int& replacing_nonce,const Int& canonical_nonce) const;
 Int pending_nonce(const std::string& addr) const;
 size_t size() const;
 size_t serialized_bytes() const;
 Json content() const;
 Json classified(bool content,const State& state) const;
 Json page(const std::string& after) const;
 Json status() const;
 void clear();
};
}
