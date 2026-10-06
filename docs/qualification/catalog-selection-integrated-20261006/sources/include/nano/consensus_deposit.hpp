#pragma once
#include "nano/consensus_signing.hpp"
#include <span>
namespace nano {
struct ConsensusDepositMessage {
 std::array<uint8_t,48> public_key{};
 ConsensusRoot withdrawal_credentials{};
 uint64_t amount_gwei=0;
};
ConsensusRoot consensus_deposit_root(const ConsensusDepositMessage& deposit);
// Ethereum deposit proof of possession: genesis fork version, zero registry
// root, DOMAIN_DEPOSIT. This does NOT prove funds were deposited. Authenticate
// the execution request and its ordering separately before registry admission.
// Deposits deliberately remain valid across subsequent fork versions.
bool verify_consensus_deposit_signature(const ConsensusDepositMessage& deposit,std::span<const uint8_t> signature,const ConsensusBytes4& genesis_fork_version);
}
