#include "nano/core.hpp"
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <memory>
namespace nano {
bool p256_verify(const Bytes& input){
 if(input.size()!=160)return false;
 const Int prime("ffffffff00000001000000000000000000000000ffffffffffffffffffffffff",16);
 const Int order("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551",16);
 auto value=[&](size_t offset){return integer(Bytes(input.begin()+offset,input.begin()+offset+32));};
 Int r=value(32),s=value(64),x=value(96),y=value(128);
 if(r<=0||r>=order||s<=0||s>=order||x>=prime||y>=prime||(x==0&&y==0))return false;
 using BN=std::unique_ptr<BIGNUM,decltype(&BN_free)>;
 using Point=std::unique_ptr<EC_POINT,decltype(&EC_POINT_free)>;
 std::unique_ptr<EC_GROUP,decltype(&EC_GROUP_free)> group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1),EC_GROUP_free);
 std::unique_ptr<BN_CTX,decltype(&BN_CTX_free)> context(BN_CTX_new(),BN_CTX_free);
 if(!group||!context)throw std::runtime_error("P256 context allocation failed");
 auto decode=[&](size_t offset){BN n(BN_bin2bn(input.data()+offset,32,nullptr),BN_free);if(!n)throw std::runtime_error("P256 scalar allocation failed");return n;};
 auto alloc=[](){BN n(BN_new(),BN_free);if(!n)throw std::runtime_error("P256 scalar allocation failed");return n;};
 auto h=decode(0),rr=decode(32),ss=decode(64),xx=decode(96),yy=decode(128);
 Point q(EC_POINT_new(group.get()),EC_POINT_free),recovered(EC_POINT_new(group.get()),EC_POINT_free);
 if(!q||!recovered)throw std::runtime_error("P256 point allocation failed");
 if(EC_POINT_set_affine_coordinates(group.get(),q.get(),xx.get(),yy.get(),context.get())!=1||EC_POINT_is_on_curve(group.get(),q.get(),context.get())!=1||EC_POINT_is_at_infinity(group.get(),q.get()))return false;
 const BIGNUM* n=EC_GROUP_get0_order(group.get());auto inv=alloc(),u1=alloc(),u2=alloc(),rx=alloc();
 if(!BN_mod_inverse(inv.get(),ss.get(),n,context.get())||!BN_mod_mul(u1.get(),h.get(),inv.get(),n,context.get())||!BN_mod_mul(u2.get(),rr.get(),inv.get(),n,context.get())||!EC_POINT_mul(group.get(),recovered.get(),u1.get(),q.get(),u2.get(),context.get()))throw std::runtime_error("P256 verification operation failed");
 // EIP-7951 requires both infinity rejection and comparison modulo the order.
 if(EC_POINT_is_at_infinity(group.get(),recovered.get()))return false;
 if(!EC_POINT_get_affine_coordinates(group.get(),recovered.get(),rx.get(),nullptr,context.get())||!BN_nnmod(rx.get(),rx.get(),n,context.get()))throw std::runtime_error("P256 coordinate extraction failed");
 return BN_cmp(rx.get(),rr.get())==0;
}
}
