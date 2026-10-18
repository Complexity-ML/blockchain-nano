#pragma once
#include "rlp.hpp"
#include <functional>
namespace nano {
// RPC work policies only. Consensus trie_root(), State and persisted roots are
// untouched. Limits bound logical material, not allocator overhead or process RSS.
struct MptProofLimits {
 size_t maximum_entries=65536,maximum_input_bytes=32*1024*1024;
 size_t maximum_materialized_bytes=32*1024*1024,maximum_nodes=196609;
 size_t maximum_proof_bytes=4*1024*1024,maximum_proof_steps=130;
};
class MptProofLimitExceeded:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// A request-owned proof trie. Keys are already hashed for secure account/storage
// tries; short raw keys are supported for the embedded-node encoding tests.
class MptProofTrie {
 using Entries=std::vector<std::pair<Bytes,Bytes>>;
 MptProofLimits limits_;std::function<void()> checkpoint_;
 std::map<Bytes,Bytes> nodes_;Bytes root_;size_t materialized_=0,node_count_=0;
 void check() const {if(checkpoint_)checkpoint_();}
 static Bytes nibbles(const Bytes& key){Bytes out;out.reserve(key.size()*2);for(auto b:key){out.push_back(b>>4);out.push_back(b&15);}return out;}
 static Bytes compact(const Bytes& key,size_t off,size_t count,bool leaf){Bytes out;const bool odd=count%2;out.push_back(uint8_t((leaf?32:0)+(odd?16+key[off++]:0)));for(size_t i=odd?1:0;i<count;i+=2){out.push_back(uint8_t(key[off]*16+key[off+1]));off+=2;}return out;}
 Bytes remember(const Bytes& raw){
  check();const auto hash=keccak(raw);auto found=nodes_.find(hash);
  if(found!=nodes_.end()){if(found->second!=raw)throw std::logic_error("MPT node hash collision");return hash;}
  if(raw.size()>limits_.maximum_materialized_bytes-materialized_)throw MptProofLimitExceeded("MPT materialized byte limit exceeded");
  materialized_+=raw.size();nodes_.emplace(hash,raw);return hash;
 }
 Rlp reference(const Rlp& node){check();auto raw=node.encode();return raw.size()<32?node:Rlp(remember(raw));}
 Rlp build(const Entries& entries,size_t first,size_t end,size_t off){
  check();if(++node_count_>limits_.maximum_nodes)throw MptProofLimitExceeded("MPT node limit exceeded");
  const auto& left=entries.at(first);const auto& right=entries.at(end-1);
  if(first+1==end)return Rlp(std::vector<Rlp>{Rlp(compact(left.first,off,left.first.size()-off,true)),Rlp(left.second)});
  size_t common=off;while(common<left.first.size()&&common<right.first.size()&&left.first[common]==right.first[common])++common;
  if(common>off)return Rlp(std::vector<Rlp>{Rlp(compact(left.first,off,common-off,false)),reference(build(entries,first,end,common))});
  std::vector<Rlp> children(17);size_t cursor=first;
  if(entries[cursor].first.size()==off){children[16]=Rlp(entries[cursor].second);++cursor;}
  while(cursor<end){check();const auto nib=entries[cursor].first.at(off);const auto start=cursor++;
   while(cursor<end&&entries[cursor].first.size()>off&&entries[cursor].first[off]==nib){check();++cursor;}
   children[nib]=reference(build(entries,start,cursor,off+1));
  }
  return Rlp(std::move(children));
 }
public:
 explicit MptProofTrie(const std::map<Bytes,Bytes>& entries,MptProofLimits limits={},std::function<void()> checkpoint={}):limits_(limits),checkpoint_(std::move(checkpoint)){
  if(!limits_.maximum_entries||limits_.maximum_entries>4194304||!limits_.maximum_input_bytes||limits_.maximum_input_bytes>256*1024*1024||!limits_.maximum_materialized_bytes||limits_.maximum_materialized_bytes>256*1024*1024||!limits_.maximum_nodes||limits_.maximum_nodes>12582913||!limits_.maximum_proof_bytes||limits_.maximum_proof_bytes>16*1024*1024||!limits_.maximum_proof_steps||limits_.maximum_proof_steps>130)throw std::invalid_argument("Invalid MPT proof policy");
  check();if(entries.size()>limits_.maximum_entries)throw MptProofLimitExceeded("MPT entry limit exceeded");
  Entries sorted;sorted.reserve(entries.size());size_t input=0;
  for(const auto& [key,value]:entries){check();if(key.size()>32)throw std::invalid_argument("MPT key exceeds 32 bytes");
   if(value.empty())continue;
   const auto cost=key.size()*2+value.size();if(cost>limits_.maximum_input_bytes-input)throw MptProofLimitExceeded("MPT input byte limit exceeded");input+=cost;
   sorted.emplace_back(nibbles(key),value);
  }
  if(sorted.empty())root_=keccak(Rlp(Bytes{}).encode());else root_=remember(build(sorted,0,sorted.size(),0).encode());check();
 }
 const Bytes& root() const {return root_;}
 size_t node_count() const {return node_count_;}
 size_t materialized_bytes() const {return materialized_;}
 std::vector<Bytes> proof(const Bytes& key) const {
  check();if(key.size()>32)throw std::invalid_argument("MPT key exceeds 32 bytes");if(nodes_.empty())return {};
  const auto path=nibbles(key);size_t offset=0,bytes_used=0;std::vector<Bytes> out;Rlp node;
  const auto append=[&](const Bytes& hash){check();const auto& raw=nodes_.at(hash);if(raw.size()>limits_.maximum_proof_bytes-bytes_used)throw MptProofLimitExceeded("MPT proof byte limit exceeded");bytes_used+=raw.size();out.push_back(raw);return Rlp::decode(raw);};
  node=append(root_);
  for(size_t steps=0;steps<limits_.maximum_proof_steps;++steps){
   check();Rlp next;
   if(!node.list)throw std::logic_error("MPT proof node must be a list");
   if(node.items.size()==17){if(offset==path.size())return out;next=node.items.at(path[offset++]);if(!next.list&&next.data.empty())return out;}
   else if(node.items.size()==2){
    const auto& encoded=node.items[0].blob();if(encoded.empty())throw std::logic_error("MPT compact path missing");const auto flag=encoded[0]>>4;
    if(flag>3||(!(flag&1)&&(encoded[0]&15)))throw std::logic_error("MPT compact path invalid");
    Bytes segment;if(flag&1)segment.push_back(encoded[0]&15);for(size_t i=1;i<encoded.size();++i){segment.push_back(encoded[i]>>4);segment.push_back(encoded[i]&15);}
    if(offset+segment.size()>path.size()||!std::equal(segment.begin(),segment.end(),path.begin()+offset))return out;
    offset+=segment.size();if(flag&2)return out;next=node.items[1];
   }else throw std::logic_error("MPT proof node arity invalid");
   if(next.list){if(next.encode().size()>=32)throw std::logic_error("MPT oversized inline node");node=std::move(next);}
   else{if(next.data.size()!=32)throw std::logic_error("MPT hash reference invalid");node=append(next.data);}
  }
  throw MptProofLimitExceeded("MPT proof traversal limit exceeded");
 }
};
} // namespace nano
