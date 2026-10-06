#include "nano/core.hpp"
namespace nano {
Rlp Authorization::payload()const{
 if(chain_id<0||chain_id>=(Int(1)<<256)||nonce<0||nonce>=(Int(1)<<64))throw std::invalid_argument("authorization field out of bounds");
 if(y_parity<0||y_parity>255||r<0||r>=(Int(1)<<256)||s<0||s>=(Int(1)<<256))throw std::invalid_argument("invalid authorization signature encoding");
 return Rlp(std::vector<Rlp>{Rlp(chain_id),Rlp(unhex(address(target))),Rlp(nonce),Rlp(y_parity),Rlp(r),Rlp(s)});
}
Bytes Authorization::signing_hash()const{return keccak(concat(Bytes{5},Rlp(std::vector<Rlp>{Rlp(chain_id),Rlp(unhex(address(target))),Rlp(nonce)}).encode()));}
std::string Authorization::authority()const{
 const Int order("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141",16);
 if(y_parity<0||y_parity>1||s>order/2)throw std::invalid_argument("invalid authorization signature");
 return public_address(recover_public_key(signing_hash(),{unsigned(u64(y_parity)),r,s}));
}
Json Authorization::to_json()const{return {{"chainId",quantity(chain_id)},{"address",checksum_address(target)},{"nonce",quantity(nonce)},{"yParity",quantity(y_parity)},{"r",quantity(r)},{"s",quantity(s)}};}
Authorization Authorization::from_json(const Json& j){Authorization a;a.chain_id=quantity(j.at("chainId"));a.target=address(j.at("address"));a.nonce=quantity(j.at("nonce"));a.y_parity=quantity(j.at("yParity"));a.r=quantity(j.at("r"));a.s=quantity(j.at("s"));a.payload();return a;}
std::optional<std::string> delegation_target(const Bytes& code){if(code.size()!=23||code[0]!=0xef||code[1]!=1||code[2]!=0)return {};return hex(Bytes(code.begin()+3,code.end()));}
}
