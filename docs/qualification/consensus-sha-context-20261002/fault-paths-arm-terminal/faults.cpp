#include "nano/consensus_signing.hpp"
#include <openssl/evp.h>
#include <iostream>
#include <stdexcept>
using namespace nano;
enum class Fault {none,alloc,init,update,final};
static Fault fault=Fault::none;
static unsigned allocations=0,frees=0,inits=0,updates=0,finals=0;
extern "C" {
EVP_MD_CTX* __real_EVP_MD_CTX_new();
void __real_EVP_MD_CTX_free(EVP_MD_CTX*);
int __real_EVP_DigestInit_ex2(EVP_MD_CTX*,const EVP_MD*,const OSSL_PARAM*);
int __real_EVP_DigestUpdate(EVP_MD_CTX*,const void*,size_t);
int __real_EVP_DigestFinal_ex(EVP_MD_CTX*,unsigned char*,unsigned int*);
EVP_MD_CTX* __wrap_EVP_MD_CTX_new(){++allocations;return fault==Fault::alloc?nullptr:__real_EVP_MD_CTX_new();}
void __wrap_EVP_MD_CTX_free(EVP_MD_CTX* p){if(p)++frees;__real_EVP_MD_CTX_free(p);}
int __wrap_EVP_DigestInit_ex2(EVP_MD_CTX* p,const EVP_MD* m,const OSSL_PARAM* params){++inits;return fault==Fault::init?0:__real_EVP_DigestInit_ex2(p,m,params);}
int __wrap_EVP_DigestUpdate(EVP_MD_CTX* p,const void* b,size_t n){++updates;return fault==Fault::update?0:__real_EVP_DigestUpdate(p,b,n);}
int __wrap_EVP_DigestFinal_ex(EVP_MD_CTX* p,unsigned char* b,unsigned int* n){++finals;return fault==Fault::final?0:__real_EVP_DigestFinal_ex(p,b,n);}
}
void require(bool b){if(!b)throw std::runtime_error("fault isolation assertion failed");}
void refuse(Fault f){fault=f;bool thrown=false;try{(void)consensus_signing_root({},{});}catch(const std::runtime_error&){thrown=true;}fault=Fault::none;require(thrown);}
int main(){try{
 const ConsensusRoot zero={0xf5,0xa5,0xfd,0x42,0xd1,0x6a,0x20,0x30,0x27,0x98,0xef,0x6e,0xd3,0x09,0x97,0x9b,0x43,0x00,0x3d,0x23,0x20,0xd9,0xf0,0xe8,0xea,0x98,0x31,0xa9,0x27,0x59,0xfb,0x4b};
 refuse(Fault::alloc);require(allocations==1&&inits==0&&updates==0&&finals==0&&frees==0);
 require(consensus_signing_root({},{})==zero);require(allocations==2);
 for(auto f:{Fault::init,Fault::update,Fault::final}){
  auto a=allocations,d=frees,i=inits,u=updates,z=finals;
  refuse(f);require(allocations==a&&frees==d+1&&inits==i+1);
  require(updates==u+(f==Fault::init?0:1));require(finals==z+(f==Fault::final?1:0));
  require(consensus_signing_root({},{})==zero);require(allocations==a+1);
  for(unsigned n=0;n<128;++n)require(consensus_signing_root({},{})==zero);
  require(allocations==a+1); // Reuse really occurs after recovery.
 }
 std::cout<<"injectedFailureStages=4 errorsRefused=true failedContextFreed=true initializationFailureShortCircuits=true recoveryKnownVectors=388 allocationReuseAfterRecovery=true\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
