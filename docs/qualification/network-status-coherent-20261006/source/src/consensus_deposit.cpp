#include "nano/consensus_deposit.hpp"
#include "nano/consensus_bls.hpp"
#include <algorithm>
namespace nano {
ConsensusRoot consensus_deposit_root(const ConsensusDepositMessage& deposit){
 ConsensusRoot first{},second{},amount{},zero{};
 std::copy_n(deposit.public_key.begin(),32,first.begin());
 std::copy(deposit.public_key.begin()+32,deposit.public_key.end(),second.begin());
 for(unsigned i=0;i<8;++i)amount[i]=static_cast<uint8_t>(deposit.amount_gwei>>(8*i));
 auto key_root=consensus_signing_root(first,second);
 return consensus_signing_root(consensus_signing_root(key_root,deposit.withdrawal_credentials),consensus_signing_root(amount,zero));
}
bool verify_consensus_deposit_signature(const ConsensusDepositMessage& deposit,std::span<const uint8_t> signature,const ConsensusBytes4& genesis_fork_version){
 auto domain=consensus_domain(ConsensusBytes4{3,0,0,0},genesis_fork_version,ConsensusRoot{});
 auto signing_root=consensus_signing_root(consensus_deposit_root(deposit),domain);
 return verify_consensus_bls(deposit.public_key,signing_root,signature);
}
}
