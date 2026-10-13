#pragma once
#include "nano/consensus_capella_store_snapshot.hpp"
#include <openssl/evp.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <limits>
#include <memory>
#include <string_view>

namespace nano::capella_detail {
// Private encoding helper, not an authentication/admission API. The caller may
// seal a certificate only AFTER complete validation of its owned source state.
// It stores no state, pointer certificates, global cache, or imported trust.
class TargetSourceFingerprintHasher {
 std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context_{EVP_MD_CTX_new(),EVP_MD_CTX_free};
 const std::function<void()>& checkpoint_;
public:
 // This internal object borrows its callback: accept only a caller-owned lvalue.
 TargetSourceFingerprintHasher(std::function<void()>&&)=delete;
 TargetSourceFingerprintHasher(const std::function<void()>&&)=delete;
 explicit TargetSourceFingerprintHasher(const std::function<void()>& checkpoint):checkpoint_(checkpoint){
  if(checkpoint_)checkpoint_();
  if(!context_||EVP_DigestInit_ex(context_.get(),EVP_sha256(),nullptr)!=1)
   throw std::runtime_error("Capella target source digest initialization");
 }
 void raw(const uint8_t* bytes,size_t count){
  // Only sequential source bytes are read. The caller serializes source access;
  // its cancellation callback must not mutate that state during validation.
  constexpr size_t chunk=64*1024;
  size_t offset=0;
  while(offset<count){
   if(checkpoint_)checkpoint_();
   const auto n=std::min(chunk,count-offset);
   if(EVP_DigestUpdate(context_.get(),bytes+offset,n)!=1)
    throw std::runtime_error("Capella target source digest update");
   offset+=n;
  }
  if(checkpoint_)checkpoint_();
 }
 void number(uint64_t value){
  uint8_t encoded[8];for(unsigned i=0;i<8;++i)encoded[i]=uint8_t(value>>(8*i));
  raw(encoded,sizeof(encoded));
 }
 void length(size_t value){
  if constexpr(std::numeric_limits<size_t>::digits>64)
   if(value>UINT64_MAX)throw std::overflow_error("Capella target source digest length");
  number(static_cast<uint64_t>(value));
 }
 void frame(std::string_view bytes){
  length(bytes.size());raw(reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size());
 }
 ConsensusRoot finish(){
  if(checkpoint_)checkpoint_();
  ConsensusRoot result{};unsigned size=0;
  if(EVP_DigestFinal_ex(context_.get(),result.data(),&size)!=1||size!=result.size())
   throw std::runtime_error("Capella target source digest finalization");
  if(checkpoint_)checkpoint_();return result;
 }
};
inline ConsensusRoot target_source_fingerprint(const CapellaSlotState& state,
 const ConsensusBeaconHeader& parent,const CapellaForkBlockRules& rules,
 const std::function<void()>& checkpoint={}){
 if(checkpoint)checkpoint();
 if(state.ssz.size()>rules.transition.maximum_state_bytes)
  throw std::length_error("Capella target source fingerprint byte budget");
 TargetSourceFingerprintHasher digest(checkpoint);
 digest.frame("Nano/Capella/TargetSourceAuthentication/v1");
 const auto profile=capella_fork_profile(rules).dump();
 if(checkpoint)checkpoint();digest.frame(profile);
 digest.number(parent.slot);digest.number(parent.proposer_index);
 digest.raw(parent.parent_root.data(),parent.parent_root.size());
 digest.raw(parent.state_root.data(),parent.state_root.size());
 digest.raw(parent.body_root.data(),parent.body_root.size());
 const uint8_t present=state.last_processed_epoch?1:0;
 digest.raw(&present,1);if(state.last_processed_epoch)digest.number(*state.last_processed_epoch);
 digest.length(state.ssz.size());digest.raw(state.ssz.data(),state.ssz.size());
 return digest.finish();
}
}
