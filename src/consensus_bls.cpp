#include "nano/consensus_bls.hpp"
#include "blst.h"
#include "consensus_bls_verification_cache_detail.hpp"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace nano {
std::optional<std::vector<uint8_t>> aggregate_consensus_bls_keys(std::span<const std::vector<uint8_t>> keys){
 if(keys.empty())return std::nullopt;blst_p1 sum{};bool first=true;
 for(const auto& bytes:keys){
  if(bytes.size()!=48)return std::nullopt;blst_p1_affine key;
  if(blst_p1_uncompress(&key,bytes.data())!=BLST_SUCCESS||blst_p1_affine_is_inf(&key)||!blst_p1_affine_in_g1(&key))return std::nullopt;
  if(first){blst_p1_from_affine(&sum,&key);first=false;}else blst_p1_add_or_double_affine(&sum,&sum,&key);
 }
 std::vector<uint8_t> result(48);blst_p1_compress(result.data(),&sum);return result;
}
std::optional<std::vector<uint8_t>> aggregate_consensus_bls_signatures(std::span<const std::vector<uint8_t>> signatures){
 if(signatures.empty())return std::nullopt;blst_p2 sum{};bool first=true;
 for(const auto& bytes:signatures){
  if(bytes.size()!=96)return std::nullopt;blst_p2_affine signature;
  if(blst_p2_uncompress(&signature,bytes.data())!=BLST_SUCCESS||blst_p2_affine_is_inf(&signature)||!blst_p2_affine_in_g2(&signature))return std::nullopt;
  if(first){blst_p2_from_affine(&sum,&signature);first=false;}else blst_p2_add_or_double_affine(&sum,&sum,&signature);
 }
 if(blst_p2_is_inf(&sum))return std::nullopt;
 std::vector<uint8_t> result(96);blst_p2_compress(result.data(),&sum);return result;
}
namespace {
void report_verification_cache_at_exit()noexcept;
bls_verification_cache_detail::PositiveVerificationCache& positive_verification_cache(){
 static bls_verification_cache_detail::PositiveVerificationCache cache;
 // Reporter registration happens AFTER construction: atExit runs before the
 // cache destructor. One bounded public-counter row, only on explicit opt-in.
 static const bool reporting=[](){const auto value=std::getenv("NANO_BLS_VERIFICATION_CACHE_METRICS");return value&&std::strcmp(value,"1")==0&&std::atexit(report_verification_cache_at_exit)==0;}();
 (void)reporting;return cache;
}
void report_verification_cache_at_exit()noexcept{
 try{const auto s=positive_verification_cache().stats();std::fprintf(stderr,
 "{\"scenario\":\"bls-verification-cache-process-metrics\",\"calls\":%" PRIu64 ",\"hits\":%" PRIu64 ",\"rawCryptoCalls\":%" PRIu64 ",\"rawCryptoNanoseconds\":%" PRIu64 ",\"entries\":%zu,\"capacityEntries\":%zu,\"residentCacheBytes\":%zu,\"maximumBytes\":%zu,\"inFlight\":%zu,\"productionQualified\":false}\n",
 s.calls,s.hits,s.raw_calls,s.raw_crypto_nanoseconds,s.entries,s.capacity_entries,s.resident_bytes,s.maximum_bytes,s.in_flight);}catch(...){/* Optional metrics never change verification or exit result. */}
}
bool verify_consensus_bls_uncached(std::span<const uint8_t> public_key,std::span<const uint8_t> message,std::span<const uint8_t> signature){
 if(public_key.size()!=48||signature.size()!=96)return false;
 blst_p1_affine key;blst_p2_affine sig;
 if(blst_p1_uncompress(&key,public_key.data())!=BLST_SUCCESS||blst_p2_uncompress(&sig,signature.data())!=BLST_SUCCESS)return false;
 if(blst_p1_affine_is_inf(&key)||!blst_p1_affine_in_g1(&key)||blst_p2_affine_is_inf(&sig)||!blst_p2_affine_in_g2(&sig))return false;
 // This is the scheme's domain separation tag, not the chain signing domain.
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
 return blst_core_verify_pk_in_g1(&key,&sig,true,message.data(),message.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0)==BLST_SUCCESS;
}
}
bool verify_consensus_bls(std::span<const uint8_t> public_key,std::span<const uint8_t> message,std::span<const uint8_t> signature){
 return positive_verification_cache().verify(public_key,message,signature,[](auto copied_key,auto copied_message,auto copied_signature){return verify_consensus_bls_uncached(copied_key,copied_message,copied_signature);});
}
bool configure_consensus_bls_verification_cache(ConsensusBlsVerificationCacheConfig config){return positive_verification_cache().configure(config);}
void clear_consensus_bls_verification_cache(){positive_verification_cache().clear();}
ConsensusBlsVerificationCacheStats consensus_bls_verification_cache_stats(){return positive_verification_cache().stats();}

bool verify_consensus_bls_fast_aggregate(std::span<const std::vector<uint8_t>> public_keys,std::span<const uint8_t> message,std::span<const uint8_t> signature){
 if(public_keys.empty()||signature.size()!=96)return false;
 blst_p1 sum{};bool first=true;
 for(const auto& encoded:public_keys){
  if(encoded.size()!=48)return false;
  blst_p1_affine key;
  if(blst_p1_uncompress(&key,encoded.data())!=BLST_SUCCESS||blst_p1_affine_is_inf(&key)||!blst_p1_affine_in_g1(&key))return false;
  if(first){blst_p1_from_affine(&sum,&key);first=false;}else blst_p1_add_or_double_affine(&sum,&sum,&key);
 }
 uint8_t compressed[48];blst_p1_compress(compressed,&sum);
 return verify_consensus_bls(compressed,message,signature);
}

}
