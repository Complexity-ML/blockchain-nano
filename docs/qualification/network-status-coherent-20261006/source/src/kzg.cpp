#include "nano/core.hpp"
#include "ckzg.h"
#include "nano_kzg_setup.hpp"
#include <cstring>
namespace nano {
namespace {
struct TrustedSetup {
 KZGSettings settings{};
 TrustedSetup(){
  if(load_trusted_setup(&settings,G1_MONOMIAL,sizeof(G1_MONOMIAL),G1_LAGRANGE,sizeof(G1_LAGRANGE),G2_MONOMIAL,sizeof(G2_MONOMIAL),0)!=C_KZG_OK)throw std::runtime_error("KZG trusted setup initialization failed");
 }
 ~TrustedSetup(){free_trusted_setup(&settings);}
 TrustedSetup(const TrustedSetup&)=delete;
};
}
bool kzg_verify(const Bytes& input){
 if(input.size()!=192)return false;
 Bytes48 commitment{},proof{};Bytes32 z{},y{};
 std::memcpy(z.bytes,input.data()+32,32);std::memcpy(y.bytes,input.data()+64,32);
 std::memcpy(commitment.bytes,input.data()+96,48);std::memcpy(proof.bytes,input.data()+144,48);
 // C++ static initialization is synchronized; verification only reads settings.
 static const TrustedSetup setup;
 bool valid=false;
 return verify_kzg_proof(&valid,&commitment,&z,&y,&proof,&setup.settings)==C_KZG_OK&&valid;
}
}
