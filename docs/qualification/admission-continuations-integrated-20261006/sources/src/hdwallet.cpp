#include "nano/hdwallet.hpp"
#include "bip39/english.hpp"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#include <sstream>
namespace nano {
namespace {
std::string nfkd(const std::string& input){
 if(input.size()>16384)throw std::invalid_argument("wallet text too long");
 UErrorCode error=U_ZERO_ERROR;std::vector<UChar> utf16(input.size()+1);int32_t length=0;
 u_strFromUTF8(utf16.data(),int32_t(utf16.size()),&length,input.data(),int32_t(input.size()),&error);
 if(U_FAILURE(error))throw std::invalid_argument("invalid wallet UTF-8");
 const auto* normalizer=unorm2_getNFKDInstance(&error);int32_t needed=unorm2_normalize(normalizer,utf16.data(),length,nullptr,0,&error);
 if(error!=U_BUFFER_OVERFLOW_ERROR&&U_FAILURE(error))throw std::runtime_error("normalization failed");error=U_ZERO_ERROR;
 std::vector<UChar> normalized(size_t(needed)+1);length=unorm2_normalize(normalizer,utf16.data(),length,normalized.data(),int32_t(normalized.size()),&error);
 if(U_FAILURE(error))throw std::runtime_error("normalization failed");
 std::string output(size_t(length)*4+1,'\0');u_strToUTF8(output.data(),int32_t(output.size()),&needed,normalized.data(),length,&error);
 if(U_FAILURE(error))throw std::runtime_error("normalization failed");output.resize(size_t(needed));return output;
}
Bytes hmac512(const Bytes& key,const Bytes& input){Bytes result(64);unsigned length=0;if(!HMAC(EVP_sha512(),key.data(),int(key.size()),input.data(),input.size(),result.data(),&length)||length!=64)throw std::runtime_error("HD HMAC failed");return result;}
}
std::string mnemonic_from_entropy(const Bytes& entropy){
 if(entropy.size()<16||entropy.size()>32||entropy.size()%4)throw std::invalid_argument("BIP39 entropy must be 128 to 256 bits in steps of 32");
 auto checksum=sha256(entropy);auto all=concat(entropy,checksum);size_t bit_count=entropy.size()*8+entropy.size()/4;std::string phrase;
 for(size_t bit=0;bit<bit_count;bit+=11){size_t index=0;for(size_t j=0;j<11;++j)index=index*2+((all[(bit+j)/8]>>(7-(bit+j)%8))&1);if(!phrase.empty())phrase+=' ';phrase+=bip39_words[index];}return phrase;
}
Bytes mnemonic_entropy(const std::string& input){
 auto normalized=nfkd(input);std::istringstream stream(normalized);std::vector<unsigned> indices;std::string word;
 while(stream>>word){auto it=std::lower_bound(bip39_words.begin(),bip39_words.end(),word);if(it==bip39_words.end()||*it!=word)throw std::invalid_argument("unknown mnemonic word");indices.push_back(unsigned(it-bip39_words.begin()));}
 if(indices.size()<12||indices.size()>24||indices.size()%3)throw std::invalid_argument("invalid mnemonic word count");
 size_t bits=indices.size()*11,entropy_bits=bits*32/33;Bytes combined((bits+7)/8);
 for(size_t i=0;i<indices.size();++i)for(size_t j=0;j<11;++j)combined[(i*11+j)/8]|=uint8_t(((indices[i]>>(10-j))&1)<<(7-(i*11+j)%8));
 Bytes entropy(combined.begin(),combined.begin()+entropy_bits/8);auto checksum=sha256(entropy);unsigned mask=0xffu<<(8-(bits-entropy_bits));
 if((combined[entropy.size()]&mask)!=(checksum[0]&mask))throw std::invalid_argument("invalid mnemonic checksum");return entropy;
}
Bytes mnemonic_seed(const std::string& mnemonic,const std::string& passphrase){
 auto canonical=mnemonic_from_entropy(mnemonic_entropy(mnemonic));auto salt="mnemonic"+nfkd(passphrase);Bytes seed(64);
 if(!PKCS5_PBKDF2_HMAC(canonical.data(),int(canonical.size()),reinterpret_cast<const unsigned char*>(salt.data()),int(salt.size()),2048,EVP_sha512(),64,seed.data()))throw std::runtime_error("mnemonic seed failed");return seed;
}
Bytes derive_private_key(const Bytes& seed,const std::string& path){
 if(seed.size()<16||seed.size()>64||path.size()>1024)throw std::invalid_argument("invalid HD seed or path");
 auto master=hmac512(Bytes{'B','i','t','c','o','i','n',' ','s','e','e','d'},seed);Bytes key(master.begin(),master.begin()+32),chain(master.begin()+32,master.end());public_key(key);
 std::istringstream parts(path);std::string part;if(!std::getline(parts,part,'/')||part!="m"||path.back()=='/')throw std::invalid_argument("HD path must start with m");
 Int order("115792089237316195423570985008687907852837564279074904382605163141518161494337");
 while(std::getline(parts,part,'/')){
  bool hardened=!part.empty()&&part.back()=='\'';if(hardened)part.pop_back();
  if(part.empty()||part.size()>10||!std::all_of(part.begin(),part.end(),[](char c){return c>='0'&&c<='9';}))throw std::invalid_argument("invalid HD child index");
  uint64_t requested=std::stoull(part);if(requested>=0x80000000ULL)throw std::invalid_argument("HD index must be below 2^31");uint32_t index=uint32_t(requested)|(hardened?0x80000000u:0);
  for(;;){Bytes data;
   if(hardened)data=concat(Bytes{0},key);else{auto pub=public_key(key);data={uint8_t(2+(pub.back()&1))};data.insert(data.end(),pub.begin(),pub.begin()+32);}
   data=concat(std::move(data),bytes(Int(std::to_string(index)),4));auto output=hmac512(chain,data);Int left=integer(Bytes(output.begin(),output.begin()+32));Int child=(left+integer(key))%order;
   if(left<order&&child!=0){key=bytes(child,32);chain=Bytes(output.begin()+32,output.end());break;}
   if(index==(hardened?0xffffffffu:0x7fffffffu))throw std::runtime_error("HD child range exhausted");++index;
  }
 }
 return key;
}
HDWallet::HDWallet(const std::string& mnemonic,const std::string& passphrase):mnemonic_(mnemonic_from_entropy(mnemonic_entropy(mnemonic))),seed_(mnemonic_seed(mnemonic_,passphrase)){}
HDWallet HDWallet::create(unsigned words,const std::string& passphrase){if(words<12||words>24||words%3)throw std::invalid_argument("invalid mnemonic word count");Bytes entropy(words/3*4);if(RAND_bytes(entropy.data(),int(entropy.size()))!=1)throw std::runtime_error("HD random source failed");return HDWallet(mnemonic_from_entropy(entropy),passphrase);}
Wallet HDWallet::derive_account(uint32_t index)const{if(index>=0x80000000u)throw std::invalid_argument("account index must be below 2^31");return Wallet(derive_private_key(seed_,"m/44'/60'/0'/0/"+std::to_string(index)));}
std::vector<std::string> HDWallet::addresses(size_t count)const{if(count>10000)throw std::invalid_argument("too many HD accounts");std::vector<std::string> result;for(size_t index=0;index<count;++index)result.push_back(derive_account(uint32_t(index)).address());return result;}
}
