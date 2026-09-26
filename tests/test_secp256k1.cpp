#include "nano/crypto.hpp"
#include <fstream>
#include <future>
#include <iostream>
using namespace nano;
namespace {
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::invalid_argument&){failed=true;}check(failed,"invalid cryptographic input accepted");}
void vector_check(const Json& v){
 const auto key=unhex(v.at("privateKey")),hash=unhex(v.at("hash"));
 const auto sig=sign_hash(key,hash);const auto pub=public_key(key);
 check(hex(pub)==v.at("publicKey").get<std::string>().replace(0,4,"0x"),"public key differs from ethers");
 check(hex(bytes(sig.r,32))==v.at("r").get<std::string>()&&hex(bytes(sig.s,32))==v.at("s").get<std::string>()&&sig.v==v.at("v"),"RFC6979 signature differs from ethers");
 check(recover_public_key(hash,sig)==pub&&public_address(pub)==v.at("address").get<std::string>(),"recovery differs from ethers");
 const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
 check(sig.s<=order/2,"signer emitted high-s");
 check(recover_public_key(hash,Signature{sig.v^1,sig.r,order-sig.s})==pub,"high-s ECRECOVER semantics changed");
}
}
int main(){try{
 std::ifstream in("tests/fixtures/secp256k1-ethers.json");Json fixture;in>>fixture;
 // Race first context initialization, then exercise immutable context sharing.
 std::vector<std::future<void>> jobs;
 for(unsigned t=0;t<8;++t)jobs.push_back(std::async(std::launch::async,[&]{for(const auto& v:fixture.at("vectors"))vector_check(v);}));
 for(auto& job:jobs)job.get();
 const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");const auto key=bytes(Int(1),32),hash=Bytes(32,0);
 for(const auto& bad:std::vector<Bytes>{Bytes{},Bytes(31,1),Bytes(33,1),Bytes(32,0),bytes(order,32),Bytes(32,255)}){rejects([&]{public_key(bad);});rejects([&]{sign_hash(bad,hash);});}
 for(const auto& bad:std::vector<Bytes>{Bytes{},Bytes(31,0),Bytes(33,0)}){rejects([&]{sign_hash(key,bad);});rejects([&]{recover_public_key(bad,Signature{0,1,1});});}
 for(const auto& bad:std::vector<Signature>{{2,1,1},{27,1,1},{0,0,1},{0,1,0},{0,-1,1},{0,1,-1},{0,order,1},{0,1,order},{0,Int(1)<<256,1}})rejects([&]{recover_public_key(hash,bad);});
 const auto fresh=generate_private_key();check(fresh.size()==32&&public_key(fresh).size()==64,"generated key invalid");
 std::cout<<"18 ethers boundary vectors, high-s recovery, invalid inputs and 8 concurrent context users passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
