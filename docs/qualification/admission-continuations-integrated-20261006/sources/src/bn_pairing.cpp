#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <mcl/bn.hpp>
#include <mutex>
namespace nano {
using Bytes=std::vector<uint8_t>;
bool bn_pairing_check(const Bytes& input){
 if(input.size()%192)throw std::invalid_argument("BN128 pairing input length");
 static std::once_flag initialized;std::call_once(initialized,[]{mcl::initPairing(mcl::BN_SNARK1);});
 const std::string modulus="30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47";
 auto coordinate=[&](size_t offset){std::string value;value.reserve(64);const char* digits="0123456789abcdef";for(size_t i=offset;i<offset+32;++i){value+=digits[input[i]>>4];value+=digits[input[i]&15];}if(value>=modulus)throw std::invalid_argument("BN128 noncanonical coordinate");mcl::Fp result;result.setStr(value,16);return result;};

 mcl::Fp12 product=1;
 for(size_t offset=0;offset<input.size();offset+=192){
  auto x=coordinate(offset),y=coordinate(offset+32);mcl::G1 p;
  if(x.isZero()&&y.isZero())p.clear();else p.set(x,y);
  mcl::Fp2 qx,qy;qx.b=coordinate(offset+64);qx.a=coordinate(offset+96);qy.b=coordinate(offset+128);qy.a=coordinate(offset+160);mcl::G2 q;
  if(qx.isZero()&&qy.isZero())q.clear();else q.set(qx,qy);
  if(!p.isValid()||!q.isValid()||!q.isValidOrder())throw std::invalid_argument("BN128 invalid curve point or subgroup");
  if(p.isZero()||q.isZero())continue;
  mcl::Fp12 value;mcl::millerLoop(value,p,q);product*=value;
 }
 mcl::Fp12 result;mcl::finalExp(result,product);return result.isOne();
}
}
