#include "nano/precompiles.hpp"
#include <openssl/evp.h>
#include <bit>
namespace nano {
bool bn_pairing_check(const Bytes& input);
namespace {
Bytes padded(const Bytes& input,size_t offset,size_t size){Bytes out(size);if(offset<input.size())std::copy_n(input.begin()+offset,std::min(size,input.size()-offset),out.begin());return out;}
const Int field("21888242871839275222246405745257275088696311157297823662689037894645226208583");
Int mod(const Int& x){Int out=x%field;if(out<0)out+=field;return out;}
struct Point{Int x=0,y=0;bool infinity=true;};
Point point(const Bytes& in,size_t off){Int x=integer(padded(in,off,32)),y=integer(padded(in,off+32,32));if(x==0&&y==0)return {};if(x>=field||y>=field||mod(y*y)!=mod(x*x*x+3))throw std::invalid_argument("invalid BN128 point");return {x,y,false};}
Point add(const Point& p,const Point& q){if(p.infinity)return q;if(q.infinity)return p;if(p.x==q.x&&mod(p.y+q.y)==0)return {};Int top,bottom;if(p.x==q.x){top=3*p.x*p.x;bottom=2*p.y;}else{top=q.y-p.y;bottom=q.x-p.x;}bottom=mod(bottom);Int inv;if(!mpz_invert(inv.get_mpz_t(),bottom.get_mpz_t(),field.get_mpz_t()))throw std::invalid_argument("invalid BN128 denominator");Int slope=mod(top*inv),x=mod(slope*slope-p.x-q.x),y=mod(slope*(p.x-x)-p.y);return {x,y,false};}
Point multiply(Point p,Int scalar){Point result;while(scalar>0){if(mpz_odd_p(scalar.get_mpz_t()))result=add(result,p);p=add(p,p);scalar>>=1;}return result;}
Bytes encode(const Point& p){return p.infinity?Bytes(64):concat(bytes(p.x,32),bytes(p.y,32));}
}
ExecutionResult precompile(unsigned number,const Bytes& input,uint64_t budget){
 ExecutionResult result;
 auto charge=[&](uint64_t cost){if(cost>budget)throw std::runtime_error("Out of gas");result.gas_used=cost;};
 try{uint64_t words=(input.size()+31)/32;
 switch(number){
 case 1:{charge(3000);auto h=padded(input,0,32);Int v=integer(padded(input,32,32));result.return_data=Bytes(32);if(v==27||v==28){try{auto key=recover_public_key(h,{unsigned(v.get_ui()-27),integer(padded(input,64,32)),integer(padded(input,96,32))});auto hash=keccak(key);std::copy(hash.begin()+12,hash.end(),result.return_data.begin()+12);}catch(const std::invalid_argument&){}}break;}
 case 2:charge(60+12*words);result.return_data=sha256(input);break;
 case 3:{charge(600+120*words);result.return_data=Bytes(32);unsigned length=0;if(!EVP_Digest(input.data(),input.size(),result.return_data.data()+12,&length,EVP_ripemd160(),nullptr)||length!=20)throw std::runtime_error("RIPEMD160 failure");break;}
 case 4:charge(15+3*words);result.return_data=input;break;
 case 5:{
  Int b=integer(padded(input,0,32)),e=integer(padded(input,32,32)),m=integer(padded(input,64,32));
  if(b>1048576||e>1048576||m>1048576)throw std::runtime_error("modexp length limit");
  size_t bl=b.get_ui(),el=e.get_ui(),ml=m.get_ui();
  // EIP-2565 uses eight-byte limbs and only the leading exponent word for gas.
  Int limbs=Int((std::max(bl,ml)+7)/8);
  Int head=integer(padded(input,96+bl,std::min(el,size_t(32))));
  size_t head_bits=head==0?0:mpz_sizeinbase(head.get_mpz_t(),2)-1;
  size_t iterations=(el>32?8*(el-32):0)+head_bits;
  Int cost=std::max(Int(200),Int(limbs*limbs*Int(std::max(iterations,size_t(1)))/3));
  if(cost>Int(std::to_string(budget)))throw std::runtime_error("Out of gas");
  charge(u64(cost));
  if(ml==0){result.return_data.clear();break;}
  Int modulus=integer(padded(input,96+bl+el,ml)),value=0;
  if(modulus!=0){Int base=integer(padded(input,96,bl)),exponent=integer(padded(input,96+bl,el));mpz_powm(value.get_mpz_t(),base.get_mpz_t(),exponent.get_mpz_t(),modulus.get_mpz_t());}
  result.return_data=bytes(value,ml);break;
 }
 case 6:charge(150);result.return_data=encode(add(point(input,0),point(input,64)));break;
 case 7:charge(6000);result.return_data=encode(multiply(point(input,0),integer(padded(input,64,32))));break;
 case 8:charge(45000+34000*(input.size()/192));result.return_data=bytes(Int(bn_pairing_check(input)?1:0),32);break;
 case 9:{
  if(input.size()!=213||input[212]>1)throw std::invalid_argument("Invalid BLAKE2F input");
  uint32_t rounds=uint32_t(input[0])<<24|uint32_t(input[1])<<16|uint32_t(input[2])<<8|input[3];charge(rounds);
  constexpr uint64_t iv[8]={0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,0x3c6ef372fe94f82bULL,0xa54ff53a5f1d36f1ULL,0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL};
  constexpr unsigned sigma[10][16]={
   {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
   {14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3},
   {11,8,12,0,5,2,15,13,10,14,3,6,7,1,9,4},
   {7,9,3,1,13,12,11,14,2,6,5,10,4,0,15,8},
   {9,0,5,7,2,4,10,15,14,1,11,12,6,8,3,13},
   {2,12,6,10,0,11,8,3,4,13,7,5,15,14,1,9},
   {12,5,1,15,14,13,4,10,0,7,6,3,9,2,8,11},
   {13,11,7,14,12,1,3,9,5,0,15,4,8,6,2,10},
   {6,15,14,9,11,3,0,8,12,2,13,7,1,4,10,5},
   {10,2,8,4,7,6,1,5,15,11,9,14,3,12,13,0}};
  auto little=[&](size_t off){uint64_t value=0;for(unsigned i=0;i<8;++i)value|=uint64_t(input[off+i])<<(8*i);return value;};
  uint64_t h[8],v[16],m[16];for(unsigned i=0;i<8;++i){h[i]=little(4+8*i);v[i]=h[i];v[i+8]=iv[i];}for(unsigned i=0;i<16;++i)m[i]=little(68+8*i);v[12]^=little(196);v[13]^=little(204);if(input[212])v[14]=~v[14];
  auto mix=[&](unsigned a,unsigned b,unsigned c,unsigned d,uint64_t x,uint64_t y){v[a]+=v[b]+x;v[d]=std::rotr(v[d]^v[a],32);v[c]+=v[d];v[b]=std::rotr(v[b]^v[c],24);v[a]+=v[b]+y;v[d]=std::rotr(v[d]^v[a],16);v[c]+=v[d];v[b]=std::rotr(v[b]^v[c],63);};
  for(uint32_t r=0;r<rounds;++r){const auto& t=sigma[r%10];mix(0,4,8,12,m[t[0]],m[t[1]]);mix(1,5,9,13,m[t[2]],m[t[3]]);mix(2,6,10,14,m[t[4]],m[t[5]]);mix(3,7,11,15,m[t[6]],m[t[7]]);mix(0,5,10,15,m[t[8]],m[t[9]]);mix(1,6,11,12,m[t[10]],m[t[11]]);mix(2,7,8,13,m[t[12]],m[t[13]]);mix(3,4,9,14,m[t[14]],m[t[15]]);}
  result.return_data.resize(64);for(unsigned i=0;i<8;++i){h[i]^=v[i]^v[i+8];for(unsigned j=0;j<8;++j)result.return_data[8*i+j]=uint8_t(h[i]>>(8*j));}break;
 }
 default:throw std::runtime_error("Precompile not implemented: "+std::to_string(number));
 }
 result.success=true;
 }catch(const std::exception& e){result.success=false;result.error=e.what();result.return_data.clear();result.gas_used=budget;}
 return result;
}
}
