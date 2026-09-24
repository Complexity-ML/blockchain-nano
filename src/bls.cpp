#include "nano/precompiles.hpp"
#include "blst.h"
#include <array>
namespace nano {
namespace {
// EIP-2537, CC0: https://eips.ethereum.org/EIPS/eip-2537
constexpr std::array<unsigned,128> g1_discount={
1000,949,848,797,764,750,738,728,719,712,705,698,692,687,682,677,673,669,665,661,658,654,651,648,645,642,640,637,635,632,630,627,
625,623,621,619,617,615,613,611,609,608,606,604,603,601,599,598,596,595,593,592,591,589,588,586,585,584,582,581,580,579,577,576,
575,574,573,572,570,569,568,567,566,565,564,563,562,561,560,559,558,557,556,555,554,553,552,551,550,549,548,547,547,546,545,544,
543,542,541,540,540,539,538,537,536,536,535,534,533,532,532,531,530,529,528,528,527,526,525,525,524,523,522,522,521,520,520,519};
constexpr std::array<unsigned,128> g2_discount={
1000,1000,923,884,855,832,812,796,782,770,759,749,740,732,724,717,711,704,699,693,688,683,679,674,670,666,663,659,655,652,649,646,
643,640,637,634,632,629,627,624,622,620,618,615,613,611,609,607,606,604,602,600,598,597,595,593,592,590,589,587,586,584,583,582,
580,579,578,576,575,574,573,571,570,569,568,567,566,565,563,562,561,560,559,558,557,556,555,554,553,552,552,551,550,549,548,547,
546,545,545,544,543,542,541,541,540,539,538,537,537,536,535,535,534,533,532,532,531,530,530,529,528,528,527,526,526,525,524,524};
blst_fp fp(const Bytes& input,size_t offset){
 static const Bytes modulus=unhex("1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab");
 auto start=input.begin()+offset;
 if(!std::all_of(start,start+16,[](uint8_t b){return b==0;})||!std::lexicographical_compare(start+16,start+64,modulus.begin(),modulus.end()))throw std::invalid_argument("noncanonical BLS field element");
 blst_fp out;blst_fp_from_bendian(&out,input.data()+offset+16);return out;
}
blst_fp2 fp2(const Bytes& input,size_t offset){return {{fp(input,offset),fp(input,offset+64)}};}
blst_p1_affine g1(const Bytes& input,size_t offset,bool subgroup){
 blst_p1_affine p{fp(input,offset),fp(input,offset+64)};
 if(!blst_p1_affine_on_curve(&p)||(subgroup&&!blst_p1_affine_in_g1(&p)))throw std::invalid_argument("invalid BLS G1 point");return p;
}
blst_p2_affine g2(const Bytes& input,size_t offset,bool subgroup){
 blst_p2_affine p{fp2(input,offset),fp2(input,offset+128)};
 if(!blst_p2_affine_on_curve(&p)||(subgroup&&!blst_p2_affine_in_g2(&p)))throw std::invalid_argument("invalid BLS G2 point");return p;
}
void encode_fp(Bytes& out,size_t offset,const blst_fp& v){blst_bendian_from_fp(out.data()+offset+16,&v);}
Bytes encode(const blst_p1& p){Bytes out(128);if(blst_p1_is_inf(&p))return out;blst_p1_affine a;blst_p1_to_affine(&a,&p);encode_fp(out,0,a.x);encode_fp(out,64,a.y);return out;}
Bytes encode(const blst_p2& p){Bytes out(256);if(blst_p2_is_inf(&p))return out;blst_p2_affine a;blst_p2_to_affine(&a,&p);encode_fp(out,0,a.x.fp[0]);encode_fp(out,64,a.x.fp[1]);encode_fp(out,128,a.y.fp[0]);encode_fp(out,192,a.y.fp[1]);return out;}
void length(const Bytes& input,size_t n,bool variable=false){if(variable?(input.empty()||input.size()%n!=0):input.size()!=n)throw std::invalid_argument("invalid BLS input length");}
}
ExecutionResult bls_precompile(unsigned number,const Bytes& input,uint64_t budget){
 ExecutionResult result;
 try{
  Int cost=0;size_t k=0;
  switch(number){
   case 11:cost=375;break;
   case 12:case 14:{k=input.size()/(number==12?160:288);if(k)cost=Int(k)*(number==12?12000:22500)*(number==12?g1_discount:g2_discount)[std::min(k,size_t(128))-1]/1000;break;}
   case 13:cost=600;break;
   case 15:cost=Int(input.size()/384)*32600+37700;break;
   case 16:cost=5500;break;
   case 17:cost=23800;break;
   default:throw std::invalid_argument("unknown BLS precompile");
  }
  if(cost>Int(std::to_string(budget)))throw std::runtime_error("Out of gas");result.gas_used=u64(cost);
  if(number==11){
   length(input,256);auto a=g1(input,0,false),b=g1(input,128,false);blst_p1 p,q,out;blst_p1_from_affine(&p,&a);blst_p1_from_affine(&q,&b);blst_p1_add_or_double(&out,&p,&q);result.return_data=encode(out);
  }else if(number==13){
   length(input,512);auto a=g2(input,0,false),b=g2(input,256,false);blst_p2 p,q,out;blst_p2_from_affine(&p,&a);blst_p2_from_affine(&q,&b);blst_p2_add_or_double(&out,&p,&q);result.return_data=encode(out);
  }else if(number==12||number==14){
   bool first=number==12;size_t stride=first?160:288;length(input,stride,true);
   std::vector<blst_scalar> scalars(k);std::vector<const byte*> scalar_ptrs(k);
   for(size_t i=0;i<k;++i){blst_scalar_from_bendian(&scalars[i],input.data()+stride*i+stride-32);scalar_ptrs[i]=scalars[i].b;}
   if(first){
    std::vector<blst_p1_affine> points(k);std::vector<const blst_p1_affine*> ptrs(k);for(size_t i=0;i<k;++i){points[i]=g1(input,stride*i,true);ptrs[i]=&points[i];}
    blst_p1 out;
    if(k==1){blst_p1 p;blst_p1_from_affine(&p,&points[0]);blst_p1_mult(&out,&p,scalar_ptrs[0],256);}
    else{std::vector<limb_t> scratch((blst_p1s_mult_pippenger_scratch_sizeof(k)+sizeof(limb_t)-1)/sizeof(limb_t));blst_p1s_mult_pippenger(&out,ptrs.data(),k,scalar_ptrs.data(),256,scratch.data());}
    result.return_data=encode(out);
   }else{
    std::vector<blst_p2_affine> points(k);std::vector<const blst_p2_affine*> ptrs(k);for(size_t i=0;i<k;++i){points[i]=g2(input,stride*i,true);ptrs[i]=&points[i];}
    blst_p2 out;
    if(k==1){blst_p2 p;blst_p2_from_affine(&p,&points[0]);blst_p2_mult(&out,&p,scalar_ptrs[0],256);}
    else{std::vector<limb_t> scratch((blst_p2s_mult_pippenger_scratch_sizeof(k)+sizeof(limb_t)-1)/sizeof(limb_t));blst_p2s_mult_pippenger(&out,ptrs.data(),k,scalar_ptrs.data(),256,scratch.data());}
    result.return_data=encode(out);
   }
  }else if(number==15){
   length(input,384,true);blst_fp12 product=*blst_fp12_one();
   for(size_t off=0;off<input.size();off+=384){auto p=g1(input,off,true);auto q=g2(input,off+128,true);if(blst_p1_affine_is_inf(&p)||blst_p2_affine_is_inf(&q))continue;blst_fp12 value;blst_miller_loop(&value,&q,&p);blst_fp12_mul(&product,&product,&value);}
   blst_fp12 out;blst_final_exp(&out,&product);result.return_data=Bytes(32);result.return_data.back()=blst_fp12_is_one(&out)?1:0;
  }else if(number==16){length(input,64);auto u=fp(input,0);blst_p1 out;blst_map_to_g1(&out,&u,nullptr);result.return_data=encode(out);
  }else{length(input,128);auto u=fp2(input,0);blst_p2 out;blst_map_to_g2(&out,&u,nullptr);result.return_data=encode(out);}
  result.success=true;
 }catch(const std::exception& e){result.success=false;result.error=e.what();result.gas_used=budget;result.return_data.clear();}
 return result;
}
}
