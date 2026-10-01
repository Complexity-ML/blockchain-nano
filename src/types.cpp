#include "nano/types.hpp"
#include <limits>
#include <openssl/evp.h>
namespace nano {
Bytes unhex(std::string s) {
 if(s.starts_with("0x") || s.starts_with("0X")) s.erase(0,2);
 if(s.size()%2) throw std::invalid_argument("odd hex length");
 auto digit=[](char c)->int { if(c>='0'&&c<='9') return c-'0'; if(c>='a'&&c<='f') return c-'a'+10; if(c>='A'&&c<='F') return c-'A'+10; throw std::invalid_argument("invalid hex"); };
 Bytes b; b.reserve(s.size()/2); for(size_t i=0;i<s.size();i+=2) b.push_back(uint8_t(digit(s[i])*16+digit(s[i+1]))); return b;
}
std::string hex(const Bytes& b) { const char* digits="0123456789abcdef"; std::string s="0x"; for(auto c:b) { s+=digits[c>>4]; s+=digits[c&15]; } return s; }
Bytes bytes(const Int& n,size_t width) { if(n<0) throw std::invalid_argument("negative integer"); size_t size=n==0?0:(mpz_sizeinbase(n.get_mpz_t(),2)+7)/8; if(width && size>width) throw std::overflow_error("integer too large"); Bytes b(std::max(size,width)); if(size) mpz_export(b.data()+b.size()-size,nullptr,1,1,1,0,n.get_mpz_t()); return b; }
Int integer(const Bytes& b) { Int n; if(!b.empty()) mpz_import(n.get_mpz_t(),b.size(),1,1,1,0,b.data()); return n; }
Int quantity(const Json& j) { std::string s; if(j.is_string()) s=j.get<std::string>(); else if(j.is_number_unsigned() || j.is_number_integer()) s=j.dump(); else throw std::invalid_argument("expected quantity"); Int n; int base=10; if(s.starts_with("0x")) {s.erase(0,2);base=16;} if(s.empty()||mpz_set_str(n.get_mpz_t(),s.c_str(),base)!=0||n<0) throw std::invalid_argument("invalid quantity"); return n; }
std::string quantity(const Int& n) { if(n<0) throw std::invalid_argument("negative quantity"); return "0x"+n.get_str(16); }
std::string address(std::string s) { auto b=unhex(s); if(b.size()!=20) throw std::invalid_argument("address must be 20 bytes"); return hex(b); }
Bytes concat(Bytes a,const Bytes& b) { a.insert(a.end(),b.begin(),b.end()); return a; }
uint64_t u64(const Int& n) {
 // Base 2 reports the exact width. Validate before conversion, without
 // rebuilding the decimal maximum at every EVM jump or memory operation.
 if(mpz_sgn(n.get_mpz_t())<0||mpz_sizeinbase(n.get_mpz_t(),2)>64)throw std::overflow_error("uint64 overflow");
 if constexpr(sizeof(unsigned long)>=sizeof(uint64_t))return n.get_ui();
 else{uint64_t result=0;mpz_export(&result,nullptr,1,sizeof(result),0,0,n.get_mpz_t());return result;}
}
static uint64_t rol(uint64_t x,unsigned n) { return n ? (x<<n)|(x>>(64-n)) : x; }
Bytes keccak(const Bytes& input) {
 constexpr uint64_t rc[]={0x1ULL,0x8082ULL,0x800000000000808aULL,0x8000000080008000ULL,0x808bULL,0x80000001ULL,0x8000000080008081ULL,0x8000000000008009ULL,0x8aULL,0x88ULL,0x80008009ULL,0x8000000aULL,0x8000808bULL,0x800000000000008bULL,0x8000000000008089ULL,0x8000000000008003ULL,0x8000000000008002ULL,0x8000000000000080ULL,0x800aULL,0x800000008000000aULL,0x8000000080008081ULL,0x8000000000008080ULL,0x80000001ULL,0x8000000080008008ULL};
 constexpr unsigned rotations[]={0,1,62,28,27,36,44,6,55,20,3,10,43,25,39,41,45,15,21,8,18,2,61,56,14};
 uint64_t state[25]={}; Bytes data=input; data.push_back(0x01); while(data.size()%136!=0) data.push_back(0); data.back()|=0x80;
 for(size_t off=0;off<data.size();off+=136) { for(size_t i=0;i<136;++i) state[i/8]^=uint64_t(data[off+i])<<(8*(i%8));
  for(auto round:rc) { uint64_t c[5],d[5],b[25]; for(int x=0;x<5;++x) c[x]=state[x]^state[x+5]^state[x+10]^state[x+15]^state[x+20]; for(int x=0;x<5;++x)d[x]=c[(x+4)%5]^rol(c[(x+1)%5],1); for(int y=0;y<5;++y)for(int x=0;x<5;++x) { state[x+5*y]^=d[x]; b[y+5*((2*x+3*y)%5)]=rol(state[x+5*y],rotations[x+5*y]); } for(int y=0;y<5;++y)for(int x=0;x<5;++x)state[x+5*y]=b[x+5*y]^((~b[(x+1)%5+5*y])&b[(x+2)%5+5*y]); state[0]^=round; }
 }
 Bytes out(32);for(size_t i=0;i<32;++i)out[i]=uint8_t(state[i/8]>>(8*(i%8)));return out;
}
Bytes sha256(const Bytes& input) { Bytes out(32); unsigned len=0; if(EVP_Digest(input.data(),input.size(),out.data(),&len,EVP_sha256(),nullptr)!=1) throw std::runtime_error("SHA256 failed");return out; }
std::string checksum_address(const std::string& input) { auto s=address(input).substr(2); auto h=hex(keccak(Bytes(s.begin(),s.end()))).substr(2); for(size_t i=0;i<s.size();++i) if(s[i]>='a'&&s[i]<='f'&&h[i]>='8')s[i]=char(s[i]-32);return "0x"+s; }
}
