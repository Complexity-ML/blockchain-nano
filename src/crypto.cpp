#include "nano/crypto.hpp"
#include <memory>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
namespace nano {
namespace {
using BN=std::unique_ptr<BIGNUM,decltype(&BN_clear_free)>;
using Point=std::unique_ptr<EC_POINT,decltype(&EC_POINT_free)>;
struct Curve {
 std::unique_ptr<EC_GROUP,decltype(&EC_GROUP_free)> group{EC_GROUP_new_by_curve_name(NID_secp256k1),EC_GROUP_free};
 std::unique_ptr<BN_CTX,decltype(&BN_CTX_free)> ctx{BN_CTX_new(),BN_CTX_free};
 BN order{BN_new(),BN_clear_free};
 Curve(){if(!group||!ctx||!order||!EC_GROUP_get_order(group.get(),order.get(),ctx.get()))throw std::runtime_error("curve init failed");}
 Point point(){return Point(EC_POINT_new(group.get()),EC_POINT_free);}
};
BN bn(const Bytes& b){return BN(BN_bin2bn(b.data(),int(b.size()),nullptr),BN_clear_free);}
Bytes raw(const BIGNUM* b){Bytes out(32);if(BN_bn2binpad(b,out.data(),32)!=32)throw std::runtime_error("BN export");return out;}
Bytes export_point(Curve& c,const EC_POINT* p){Bytes out(65);if(EC_POINT_point2oct(c.group.get(),p,POINT_CONVERSION_UNCOMPRESSED,out.data(),out.size(),c.ctx.get())!=65)throw std::runtime_error("invalid curve point");return Bytes(out.begin()+1,out.end());}
void check_key(const Bytes& key,Curve& c){auto n=bn(key);if(key.size()!=32||!n||BN_is_zero(n.get())||BN_cmp(n.get(),c.order.get())>=0)throw std::invalid_argument("invalid private key");}
Bytes hmac(const Bytes& key,const Bytes& data){Bytes out(32);unsigned len=0;if(!HMAC(EVP_sha256(),key.data(),int(key.size()),data.data(),data.size(),out.data(),&len))throw std::runtime_error("HMAC failed");return out;}
Bytes random_bytes(size_t size){Bytes out(size);if(RAND_bytes(out.data(),int(size))!=1)throw std::runtime_error("random source failed");return out;}
Bytes aes(const Bytes& input,const Bytes& key,const Bytes& iv){if(iv.size()!=16||key.size()<16)throw std::invalid_argument("invalid AES parameters");auto ctx=std::unique_ptr<EVP_CIPHER_CTX,decltype(&EVP_CIPHER_CTX_free)>(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);Bytes out(input.size()+16);int count=0,last=0;if(!ctx||!EVP_EncryptInit_ex(ctx.get(),EVP_aes_128_ctr(),nullptr,key.data(),iv.data())||!EVP_EncryptUpdate(ctx.get(),out.data(),&count,input.data(),int(input.size()))||!EVP_EncryptFinal_ex(ctx.get(),out.data()+count,&last))throw std::runtime_error("AES failed");out.resize(size_t(count+last));return out;}
}
Bytes generate_private_key(){Curve c;for(;;){auto key=random_bytes(32);try{check_key(key,c);return key;}catch(const std::invalid_argument&){}}}
Bytes public_key(const Bytes& key){Curve c;check_key(key,c);auto n=bn(key);auto p=c.point();if(!EC_POINT_mul(c.group.get(),p.get(),n.get(),nullptr,nullptr,c.ctx.get()))throw std::runtime_error("public key failed");return export_point(c,p.get());}
std::string public_address(const Bytes& key){if(key.size()!=64)throw std::invalid_argument("public key must be 64 bytes");auto h=keccak(key);return checksum_address(hex(Bytes(h.begin()+12,h.end())));}
Signature sign_hash(const Bytes& key,const Bytes& hash){
 Curve c;check_key(key,c);if(hash.size()!=32)throw std::invalid_argument("hash must be 32 bytes");Int order=integer(raw(c.order.get()));Int z=integer(hash)%order;
 Bytes k(32),v(32,1);k=hmac(k,concat(concat(concat(v,Bytes{0}),key),bytes(z,32)));v=hmac(k,v);k=hmac(k,concat(concat(concat(v,Bytes{1}),key),bytes(z,32)));v=hmac(k,v);
 for(;;){v=hmac(k,v);Int nonce=integer(v);if(nonce>0&&nonce<order){auto n=bn(v);auto p=c.point();if(!EC_POINT_mul(c.group.get(),p.get(),n.get(),nullptr,nullptr,c.ctx.get()))throw std::runtime_error("sign failed");auto xy=export_point(c,p.get());Int x=integer(Bytes(xy.begin(),xy.begin()+32));Int r=x%order;Int inverse;mpz_invert(inverse.get_mpz_t(),nonce.get_mpz_t(),order.get_mpz_t());Int s=((z+r*integer(key))*inverse)%order;
 if(r!=0&&s!=0&&x<order){unsigned recovery=xy.back()&1;if(s>order/2){s=order-s;recovery^=1;}return {recovery,r,s};}}
 k=hmac(k,concat(v,Bytes{0}));v=hmac(k,v);}
}
Bytes recover_public_key(const Bytes& hash,const Signature& sig){
 Curve c;Int order=integer(raw(c.order.get()));if(hash.size()!=32||sig.v>1||sig.r<=0||sig.r>=order||sig.s<=0||sig.s>=order)throw std::invalid_argument("invalid signature");
 auto r=bn(bytes(sig.r,32));auto p=c.point();if(!EC_POINT_set_compressed_coordinates(c.group.get(),p.get(),r.get(),int(sig.v),c.ctx.get()))throw std::invalid_argument("invalid signature point");
 auto check=c.point();if(!EC_POINT_mul(c.group.get(),check.get(),nullptr,p.get(),c.order.get(),c.ctx.get())||!EC_POINT_is_at_infinity(c.group.get(),check.get()))throw std::invalid_argument("invalid point order");
 Int inv;mpz_invert(inv.get_mpz_t(),sig.r.get_mpz_t(),order.get_mpz_t());Int u1=(-integer(hash)*inv)%order;if(u1<0)u1+=order;Int u2=(sig.s*inv)%order;auto a=bn(bytes(u1,32));auto b=bn(bytes(u2,32));auto out=c.point();if(!EC_POINT_mul(c.group.get(),out.get(),a.get(),p.get(),b.get(),c.ctx.get())||EC_POINT_is_at_infinity(c.group.get(),out.get()))throw std::invalid_argument("signature recovery failed");return export_point(c,out.get());
}
Bytes personal_hash(const Bytes& message){std::string prefix=std::string(1,char(0x19))+"Ethereum Signed Message:\n"+std::to_string(message.size());return keccak(concat(Bytes(prefix.begin(),prefix.end()),message));}
Bytes sign_message(const Bytes& key,const Bytes& message){auto s=sign_hash(key,personal_hash(message));return concat(concat(bytes(s.r,32),bytes(s.s,32)),Bytes{uint8_t(s.v+27)});}
std::string recover_message(const Bytes& message,const Bytes& signature){if(signature.size()!=65||signature[64]<27||signature[64]>28)throw std::invalid_argument("invalid personal signature");Signature s{unsigned(signature[64]-27),integer(Bytes(signature.begin(),signature.begin()+32)),integer(Bytes(signature.begin()+32,signature.begin()+64))};return public_address(recover_public_key(personal_hash(message),s));}
Json encrypt_key(const Bytes& key,const std::string& password){
 auto addr=public_address(public_key(key));auto salt=random_bytes(32),iv=random_bytes(16);Bytes derived(32);if(EVP_PBE_scrypt(password.data(),password.size(),salt.data(),salt.size(),16384,8,1,64*1024*1024,derived.data(),derived.size())!=1)throw std::runtime_error("scrypt failed");auto encrypted=aes(key,derived,iv);auto mac=keccak(concat(Bytes(derived.begin()+16,derived.end()),encrypted));auto id=random_bytes(16);id[6]=(id[6]&15)|64;id[8]=(id[8]&63)|128;auto uuid=hex(id).substr(2);uuid=uuid.substr(0,8)+"-"+uuid.substr(8,4)+"-"+uuid.substr(12,4)+"-"+uuid.substr(16,4)+"-"+uuid.substr(20);
 OPENSSL_cleanse(derived.data(),derived.size());return Json{{"version",3},{"id",uuid},{"address",address(addr).substr(2)},{"crypto",{{"cipher","aes-128-ctr"},{"ciphertext",hex(encrypted).substr(2)},{"cipherparams",{{"iv",hex(iv).substr(2)}}},{"kdf","scrypt"},{"kdfparams",{{"dklen",32},{"n",16384},{"r",8},{"p",1},{"salt",hex(salt).substr(2)}}},{"mac",hex(mac).substr(2)}}}};
}
Bytes decrypt_key(const Json& file,const std::string& password){
 if(file.at("version")!=3)throw std::invalid_argument("unsupported keystore version");const auto& c=file.contains("crypto")?file.at("crypto"):file.at("Crypto");if(c.at("cipher")!="aes-128-ctr")throw std::invalid_argument("unsupported cipher");const auto& p=c.at("kdfparams");auto salt=unhex(p.at("salt"));if(p.at("dklen")!=32||salt.size()>1024)throw std::invalid_argument("invalid KDF parameters");Bytes derived(32);
 if(c.at("kdf")=="scrypt"){auto n=p.at("n").get<uint64_t>(),r=p.at("r").get<uint64_t>(),parallel=p.at("p").get<uint64_t>();if(n<2||n>(1<<20)||r==0||r>32||parallel==0||parallel>16||(n&(n-1)))throw std::invalid_argument("scrypt limits exceeded");if(!EVP_PBE_scrypt(password.data(),password.size(),salt.data(),salt.size(),n,r,parallel,512ULL*1024*1024,derived.data(),derived.size()))throw std::invalid_argument("scrypt failed");}
 else if(c.at("kdf")=="pbkdf2"){int iterations=p.at("c");if(p.at("prf")!="hmac-sha256"||iterations<1||iterations>10000000)throw std::invalid_argument("invalid PBKDF2 parameters");if(!PKCS5_PBKDF2_HMAC(password.data(),int(password.size()),salt.data(),int(salt.size()),iterations,EVP_sha256(),32,derived.data()))throw std::runtime_error("PBKDF2 failed");}else throw std::invalid_argument("unsupported KDF");
 auto encrypted=unhex(c.at("ciphertext"));if(encrypted.size()!=32)throw std::invalid_argument("invalid ciphertext length");auto mac=keccak(concat(Bytes(derived.begin()+16,derived.end()),encrypted));auto expected=unhex(c.at("mac"));if(expected.size()!=32||CRYPTO_memcmp(mac.data(),expected.data(),32)){OPENSSL_cleanse(derived.data(),derived.size());throw std::invalid_argument("wrong password or corrupt keystore");}auto key=aes(encrypted,derived,unhex(c.at("cipherparams").at("iv")));OPENSSL_cleanse(derived.data(),derived.size());auto addr=public_address(public_key(key));if(file.contains("address")&&address(addr)!=address(file.at("address")))throw std::invalid_argument("keystore address mismatch");return key;
}
}
