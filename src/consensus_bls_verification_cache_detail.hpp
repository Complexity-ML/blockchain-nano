#pragma once
#include "nano/consensus_bls_verification_cache.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>
namespace nano::bls_verification_cache_detail {
// The same instance implementation is tested with a scheduling double. The
// production singleton is PRIVATE to consensus_bls.cpp and always uses BLST;
// no global verifier injection, certificate import, or "assume valid" API.
class PositiveVerificationCache {
 using Key=std::array<uint8_t,48+32+96>;
 static_assert(sizeof(Key)==176);
 using Buffer=std::vector<Key>;
 mutable std::mutex mutex_;
 Buffer slots_;
 ConsensusBlsVerificationCacheStats stats_;
 size_t entries_=0,next_=0;
 void add(uint64_t& target,uint64_t value=1){
  if(value>UINT64_MAX-target){target=UINT64_MAX;stats_.counters_saturated=true;}else target+=value;
 }
 void new_generation(){
  if(stats_.generation==UINT64_MAX)throw std::overflow_error("BLS verification cache generation exhausted");
  const auto generation=stats_.generation+1;const auto inflight=stats_.in_flight;
  stats_=ConsensusBlsVerificationCacheStats{};stats_.generation=generation;stats_.in_flight=inflight;
  entries_=0;next_=0;
 }
 struct Ticket {Key key{};uint64_t generation=0;bool snapshot_available=false,cacheable=false,hit=false,malformed=false;};
 Ticket begin(std::span<const uint8_t> key,std::span<const uint8_t> message,std::span<const uint8_t> signature){
  Ticket t;
  std::lock_guard lock(mutex_);add(stats_.calls);t.generation=stats_.generation;
  if(key.size()!=48||signature.size()!=96){add(stats_.malformed);t.malformed=true;return t;}
  t.snapshot_available=message.size()==32;t.cacheable=t.snapshot_available&&!slots_.empty();
  // One byte snapshot binds lookup AND raw verification, even when disabled.
  if(t.snapshot_available){std::copy(key.begin(),key.end(),t.key.begin());std::copy(message.begin(),message.end(),t.key.begin()+48);std::copy(signature.begin(),signature.end(),t.key.begin()+80);}
  if(t.cacheable){
   for(size_t i=0;i<entries_;++i)if(slots_[i]==t.key){add(stats_.hits);t.hit=true;return t;}
   add(stats_.misses);
  }else add(stats_.bypasses);
  add(stats_.raw_calls);++stats_.in_flight;return t;
 }
 void finish(const Ticket& t,std::optional<bool> valid,uint64_t nanoseconds){
  std::lock_guard lock(mutex_);
  if(!stats_.in_flight)throw std::logic_error("BLS verification cache in-flight accounting");--stats_.in_flight;
  if(t.generation!=stats_.generation){add(stats_.stale_completions);return;}
  add(stats_.raw_crypto_nanoseconds,nanoseconds);
  if(!valid){add(stats_.raw_exceptions);return;}
  add(*valid?stats_.raw_positive:stats_.raw_negative);
  if(!*valid||!t.cacheable||slots_.empty())return;
  for(size_t i=0;i<entries_;++i)if(slots_[i]==t.key)return;
  if(entries_<slots_.size())slots_[entries_++]=t.key;
  else{slots_[next_]=t.key;next_=(next_+1)%slots_.size();add(stats_.evictions);}
 }
 static uint64_t elapsed(std::chrono::steady_clock::time_point began){
  const auto n=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-began).count();return n>0?uint64_t(n):0;
 }
public:
 explicit PositiveVerificationCache(ConsensusBlsVerificationCacheConfig config={}){(void)configure(config);}
 // Factory overload is only for separately constructed detail instances. It
 // stores no callable; production configure always uses the normal allocator.
 template<class Factory> bool configure(ConsensusBlsVerificationCacheConfig config,Factory&& factory){
  const bool disabled=!config.maximum_bytes||!config.maximum_entries;
  if(!disabled&&config.maximum_bytes<sizeof(*this)+sizeof(Key))throw std::invalid_argument("BLS verification cache byte budget too small");
  const size_t count=disabled?0:std::min(config.maximum_entries,(config.maximum_bytes-sizeof(*this))/sizeof(Key));
  Buffer candidate;
  try{candidate=factory(count);}catch(const std::bad_alloc&){std::lock_guard lock(mutex_);add(stats_.allocation_failures);return false;}
  if(candidate.size()!=count||(disabled&&candidate.capacity()!=0)||(!disabled&&candidate.capacity()>(config.maximum_bytes-sizeof(*this))/sizeof(Key)))throw std::invalid_argument("BLS verification cache allocation exceeds budget");
  {
   std::lock_guard lock(mutex_);new_generation();slots_.swap(candidate);
   stats_.maximum_bytes=config.maximum_bytes;stats_.maximum_entries=config.maximum_entries;
  }
  return true;
 }
 bool configure(ConsensusBlsVerificationCacheConfig config){return configure(config,[](size_t n){return Buffer(n);});}
 void clear(){
  std::lock_guard lock(mutex_);const auto bytes=stats_.maximum_bytes,entries=stats_.maximum_entries;new_generation();stats_.maximum_bytes=bytes;stats_.maximum_entries=entries;
 }
 ConsensusBlsVerificationCacheStats stats()const{
  std::lock_guard lock(mutex_);auto out=stats_;out.entries=entries_;out.capacity_entries=slots_.size();out.fixed_metadata_bytes=sizeof(*this);out.allocated_entry_bytes=slots_.capacity()*sizeof(Key);out.resident_bytes=out.fixed_metadata_bytes+out.allocated_entry_bytes;return out;
 }
 template<class Verify> bool verify(std::span<const uint8_t> key,std::span<const uint8_t> message,std::span<const uint8_t> signature,Verify&& actual_verifier){
  const auto ticket=begin(key,message,signature);if(ticket.malformed)return false;if(ticket.hit)return true;
  const auto began=std::chrono::steady_clock::now();bool valid;
  try{if(ticket.snapshot_available){const std::span<const uint8_t> copied(ticket.key);valid=actual_verifier(copied.first(48),copied.subspan(48,32),copied.subspan(80,96));}else valid=actual_verifier(key,message,signature);}catch(...){finish(ticket,std::nullopt,elapsed(began));throw;}
  finish(ticket,valid,elapsed(began));return valid;
 }
};
}
