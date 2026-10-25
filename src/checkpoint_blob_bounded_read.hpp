#pragma once
#include "nano/core.hpp"
#include <stdexcept>
#include <string_view>
#include <utility>

namespace nano::storage_detail {
// The view already exists in RocksDB's PinnableSlice. Only the application
// hash/copy boundary is controlled here, not the allocation that produced it.
// The production caller supplies SHA-256 and std::string copy; injected
// callbacks let the regression observe that neither runs on oversized input.
template<class Hash, class Copy>
std::string validated_checkpoint_blob_copy(std::string_view value,
 const Bytes& expected_digest, size_t maximum_bytes, Hash&& hash, Copy&& copy) {
 if (value.empty() || value.size() > maximum_bytes)
  throw std::runtime_error("corrupt or oversized checkpoint blob");
 if (std::forward<Hash>(hash)(value) != expected_digest)
  throw std::runtime_error("corrupt or oversized checkpoint blob");
 return std::forward<Copy>(copy)(value);
}
}
