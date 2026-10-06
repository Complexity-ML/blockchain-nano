#pragma once
#include "types.hpp"
namespace nano {
struct Rlp {
 bool list=false;
 Bytes data;
 std::vector<Rlp> items;
 Rlp()=default;
 Rlp(Bytes b):data(std::move(b)){}
 Rlp(const Int& n):data(bytes(n)){}
 Rlp(uint64_t n):data(bytes(Int(std::to_string(n)))){}
 Rlp(std::vector<Rlp> v):list(true),items(std::move(v)){}
 Int number() const;
 const Bytes& blob() const;
 const Rlp& operator[](size_t i) const;
 Bytes encode() const;
 static Rlp decode(const Bytes& input);
};
Bytes trie_root(const std::map<Bytes,Bytes>& entries);
}
