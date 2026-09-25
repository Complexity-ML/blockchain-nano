#pragma once
#include "nano/consensus_signing.hpp"
#include "nano/types.hpp"
#include <functional>
#include <span>
#include <optional>
namespace nano {
struct ExecutionPayloadLimits {uint32_t extra_data_bytes,transaction_bytes,transactions;};
// Bellatrix only. Transactions are opaque envelopes; SSZ validity is not EVM
// execution validity. The uint256 base fee is stored in SSZ little-endian order.
struct BellatrixExecutionPayload {
 ConsensusRoot parent_hash{},state_root{},receipts_root{},prev_randao{},base_fee_per_gas{},block_hash{};
 std::array<uint8_t,20> fee_recipient{};
 std::array<uint8_t,256> logs_bloom{};
 uint64_t block_number=0,gas_limit=0,gas_used=0,timestamp=0;
 Bytes extra_data;
 std::vector<Bytes> transactions;
 bool operator==(const BellatrixExecutionPayload&)const=default;
};
// Explicit transport budget before allocating. Malformed bytes return nullopt;
// invalid configured limits or cancellation throw. No allocation at type limits.
std::optional<BellatrixExecutionPayload> decode_bellatrix_execution_payload(
 std::span<const uint8_t>,const ExecutionPayloadLimits&,size_t maximum_bytes,
 const std::function<void()>& checkpoint={});
Bytes encode_bellatrix_execution_payload(const BellatrixExecutionPayload&,
 const ExecutionPayloadLimits&,size_t maximum_bytes,const std::function<void()>& checkpoint={});
ConsensusRoot bellatrix_execution_payload_root(const BellatrixExecutionPayload&,
 const ExecutionPayloadLimits&,const std::function<void()>& checkpoint={});
}
