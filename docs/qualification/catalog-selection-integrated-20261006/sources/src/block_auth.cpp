#include "nano/block_auth.hpp"
#include "nano/crypto.hpp"
namespace nano {
Bytes bft_round_extra(uint64_t round){return concat(Bytes{'N','B','F',1},bytes(Int(std::to_string(round)),8));}
uint64_t bft_proposal_round(const Block& block){const auto& extra=block.header.extra_data;if(extra.size()!=12||!std::equal(extra.begin(),extra.begin()+4,Bytes{'N','B','F',1}.begin()))throw std::invalid_argument("invalid BFT proposal round encoding");return u64(integer(Bytes(extra.begin()+4,extra.end())));}

Bytes block_signing_hash(const Block& block,const Int& chain_id,const Bytes& genesis_hash){
 if(chain_id<=0||chain_id>=(Int(1)<<256)||genesis_hash.size()!=32)throw std::invalid_argument("invalid block signature domain");
 if(block.header.number==0||!block.uncles.empty()||!block.withdrawals.is_array()||!block.withdrawals.empty())throw std::invalid_argument("unsupported signed PoS block structure");
 const std::string domain="Nano/PoS/Block";
 std::vector<Rlp> transactions;for(const auto& tx:block.transactions)transactions.emplace_back(tx.raw());
 return keccak(Rlp(std::vector<Rlp>{Rlp(Bytes(domain.begin(),domain.end())),Rlp(Int(1)),Rlp(chain_id),Rlp(genesis_hash),Rlp(block.header.encode()),Rlp(transactions)}).encode());
}
void sign_block(Block& block,const Bytes& key,const Int& chain_id,const Bytes& genesis_hash){
 if(address(public_address(public_key(key)))!=address(block.header.coinbase))throw std::invalid_argument("block signer differs from producer");
 auto signature=sign_hash(key,block_signing_hash(block,chain_id,genesis_hash));
 auto proof=concat(bytes(signature.r,32),bytes(signature.s,32));proof.push_back(uint8_t(signature.v));block.producer_signature=std::move(proof);
}
bool verify_block_signature(const Block& block,const Int& chain_id,const Bytes& genesis_hash){
 try{
  const auto& proof=block.producer_signature;if(proof.size()!=65||proof[64]>1)return false;
  Signature signature{proof[64],integer(Bytes(proof.begin(),proof.begin()+32)),integer(Bytes(proof.begin()+32,proof.begin()+64))};
  const Int order("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
  if(signature.s<=0||signature.s>order/2)return false;
  auto signer=public_address(recover_public_key(block_signing_hash(block,chain_id,genesis_hash),signature));
  return address(signer)==address(block.header.coinbase);
 }catch(const std::exception&){return false;}
}
}
