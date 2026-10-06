#pragma once
#include "core.hpp"
namespace nano {
Bytes bft_round_extra(uint64_t round);
uint64_t bft_proposal_round(const Block& block);
Bytes block_signing_hash(const Block& block,const Int& chain_id,const Bytes& genesis_hash);
void sign_block(Block& block,const Bytes& key,const Int& chain_id,const Bytes& genesis_hash);
bool verify_block_signature(const Block& block,const Int& chain_id,const Bytes& genesis_hash);
}
