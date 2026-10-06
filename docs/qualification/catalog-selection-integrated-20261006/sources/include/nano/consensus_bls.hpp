#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <optional>
namespace nano {
// Ethereum BLS POP ciphersuite, compressed G1 public key / G2 signature.
// Cryptographic verification only: callers must construct the fork/chain-bound
// signing root and separately check validator eligibility and voting weight.
// Does not authorize aggregate keys or establish proof of possession.
bool verify_consensus_bls(std::span<const uint8_t> public_key,std::span<const uint8_t> message,std::span<const uint8_t> signature);
// FastAggregateVerify for a common message. PRECONDITION: every key has a
// verified proof of possession. This low-level primitive does not establish
// membership, uniqueness, weight, quorum, or the required possession proofs.
// Never authorize consensus votes by calling this primitive alone.
bool verify_consensus_bls_fast_aggregate(std::span<const std::vector<uint8_t>> public_keys,std::span<const uint8_t> message,std::span<const uint8_t> signature);
// Aggregate validated compressed G1 keys, retaining duplicates as required by
// sync committees. Does not establish possession or authorize membership.
std::optional<std::vector<uint8_t>> aggregate_consensus_bls_keys(std::span<const std::vector<uint8_t>> public_keys);
// Sum nonempty compressed G2 signatures, retaining multiplicity. Rejects bad
// encodings, non-subgroup points and infinity (including a cancelling sum).
// Does not verify messages/membership; callers must authenticate each input.
std::optional<std::vector<uint8_t>> aggregate_consensus_bls_signatures(std::span<const std::vector<uint8_t>> signatures);

}
