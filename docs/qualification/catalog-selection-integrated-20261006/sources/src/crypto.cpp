#include "nano/crypto.hpp"
#include <memory>
#include <array>
#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
namespace nano {
namespace {
// Immutable after initialization: libsecp256k1 permits concurrent const use.
struct SecpContext {
 std::unique_ptr<secp256k1_context,decltype(&secp256k1_context_destroy)> value{secp256k1_context_create(SECP256K1_CONTEXT_NONE),secp256k1_context_destroy};
 SecpContext(){
  std::array<unsigned char,32> seed{};
  const bool entropy=RAND_bytes(seed.data(),int(seed.size()))==1;
  const bool randomized=entropy&&value&&secp256k1_context_randomize(value.get(),seed.data());
  OPENSSL_cleanse(seed.data(),seed.size());
  if(!randomized)throw std::runtime_error("secp256k1 context initialization failed");
 }
};
const secp256k1_context* secp(){static const SecpContext context;return context.value.get();}
void check_key(const Bytes& key){if(key.size()!=32||!secp256k1_ec_seckey_verify(secp(),key.data()))throw std::invalid_argument("invalid private key");}
Bytes export_public(const secp256k1_pubkey& key){
 Bytes out(65);size_t size=out.size();
 if(!secp256k1_ec_pubkey_serialize(secp(),out.data(),&size,&key,SECP256K1_EC_UNCOMPRESSED)||size!=65)throw std::runtime_error("public key serialization failed");
 return Bytes(out.begin()+1,out.end());
}
Bytes random_bytes(size_t size){Bytes out(size);if(RAND_bytes(out.data(),int(size))!=1)throw std::runtime_error("random source failed");return out;}
Bytes aes(const Bytes& input,const Bytes& key,const Bytes& iv){if(iv.size()!=16||key.size()<16)throw std::invalid_argument("invalid AES parameters");auto ctx=std::unique_ptr<EVP_CIPHER_CTX,decltype(&EVP_CIPHER_CTX_free)>(EVP_CIPHER_CTX_new(),EVP_CIPHER_CTX_free);Bytes out(input.size()+16);int count=0,last=0;if(!ctx||!EVP_EncryptInit_ex(ctx.get(),EVP_aes_128_ctr(),nullptr,key.data(),iv.data())||!EVP_EncryptUpdate(ctx.get(),out.data(),&count,input.data(),int(input.size()))||!EVP_EncryptFinal_ex(ctx.get(),out.data()+count,&last))throw std::runtime_error("AES failed");out.resize(size_t(count+last));return out;}
}
Bytes generate_private_key(){for(;;){auto key=random_bytes(32);if(secp256k1_ec_seckey_verify(secp(),key.data()))return key;OPENSSL_cleanse(key.data(),key.size());}}
Bytes public_key(const Bytes& key){check_key(key);secp256k1_pubkey out;if(!secp256k1_ec_pubkey_create(secp(),&out,key.data()))throw std::runtime_error("public key failed");return export_public(out);}
std::string public_address(const Bytes& key){if(key.size()!=64)throw std::invalid_argument("public key must be 64 bytes");auto h=keccak(key);return checksum_address(hex(Bytes(h.begin()+12,h.end())));}
Signature sign_hash(const Bytes& key,const Bytes& hash){
 check_key(key);if(hash.size()!=32)throw std::invalid_argument("hash must be 32 bytes");
 secp256k1_ecdsa_recoverable_signature signature;
 if(!secp256k1_ecdsa_sign_recoverable(secp(),&signature,hash.data(),key.data(),secp256k1_nonce_function_rfc6979,nullptr))throw std::runtime_error("sign failed");
 Bytes compact(64);int recovery=0;
 if(!secp256k1_ecdsa_recoverable_signature_serialize_compact(secp(),compact.data(),&recovery,&signature))throw std::runtime_error("signature serialization failed");
 // Ethereum carries y parity only; never truncate a non-representable recovery id.
 if(recovery>1)throw std::runtime_error("signature recovery id not representable in Ethereum");
 return {unsigned(recovery),integer(Bytes(compact.begin(),compact.begin()+32)),integer(Bytes(compact.begin()+32,compact.end()))};
}
Bytes recover_public_key(const Bytes& hash,const Signature& sig){
 static const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
 if(hash.size()!=32||sig.v>1||sig.r<=0||sig.r>=order||sig.s<=0||sig.s>=order)throw std::invalid_argument("invalid signature");
 const auto compact=concat(bytes(sig.r,32),bytes(sig.s,32));secp256k1_ecdsa_recoverable_signature parsed;secp256k1_pubkey pub;
 // Recovery deliberately accepts high-s: ECRECOVER differs from transaction admission.
 if(!secp256k1_ecdsa_recoverable_signature_parse_compact(secp(),&parsed,compact.data(),int(sig.v))||!secp256k1_ecdsa_recover(secp(),&pub,&parsed,hash.data()))throw std::invalid_argument("invalid signature point");
 return export_public(pub);
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
