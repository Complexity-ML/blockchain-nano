#include "nano/consensus_bls.hpp"
#include "blst.h"
namespace nano {
bool verify_consensus_bls(std::span<const uint8_t> public_key,std::span<const uint8_t> message,std::span<const uint8_t> signature){
 if(public_key.size()!=48||signature.size()!=96)return false;
 blst_p1_affine key;blst_p2_affine sig;
 if(blst_p1_uncompress(&key,public_key.data())!=BLST_SUCCESS||blst_p2_uncompress(&sig,signature.data())!=BLST_SUCCESS)return false;
 if(blst_p1_affine_is_inf(&key)||!blst_p1_affine_in_g1(&key)||blst_p2_affine_is_inf(&sig)||!blst_p2_affine_in_g2(&sig))return false;
 // This is the scheme's domain separation tag, not the chain signing domain.
 static constexpr char dst[]="BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_";
 return blst_core_verify_pk_in_g1(&key,&sig,true,message.data(),message.size(),reinterpret_cast<const uint8_t*>(dst),sizeof(dst)-1,nullptr,0)==BLST_SUCCESS;
}
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
