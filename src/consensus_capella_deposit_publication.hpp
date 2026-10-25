#pragma once
#include "nano/consensus_capella_cold_records.hpp"
#include "nano/storage.hpp"
#include <memory>

// Internal runtime preparation, not a peer codec or an activation API.
namespace nano::deposit_publication_detail {
using Checkpoint = std::function<void()>;

struct DepositDeltaLimits {
 size_t maximum_blobs, maximum_blob_bytes;
 size_t maximum_locators, maximum_locator_bytes;
};
struct DepositPublicationLimits {
 CapellaStoreSnapshotLimits store;
 CanonicalDepositContextLimits context;
 DepositCheckpointTreeBudget tree;
 size_t cache_records, cache_bytes;
 // The storage API applies this bound to its complete serialized WriteBatch.
 size_t maximum_batch_bytes;
 DepositDeltaLimits delta;
};

struct SealedDepositDeltaData;
struct DepositPublicationState;
struct DepositPrimaryGeneration;
class DepositPublicationFactory;

class SealedDepositDelta {
 std::shared_ptr<const SealedDepositDeltaData> data_;
 explicit SealedDepositDelta(std::shared_ptr<const SealedDepositDeltaData>);
 friend class DepositPublicationFactory;
public:
 // Owns the inputs. SHA, widths, counts, byte sums and duplicate conflicts are
 // checked before issuing this immutable value; it certifies no consensus rule.
 static SealedDepositDelta seal(std::vector<CheckpointBlob>,
  std::vector<CheckpointDepositLocatorRecord>,const DepositDeltaLimits&,
  const Checkpoint& = {});
 SealedDepositDelta(SealedDepositDelta&&) noexcept = default;
 SealedDepositDelta& operator=(SealedDepositDelta&&) noexcept = default;
 SealedDepositDelta(const SealedDepositDelta&) = delete;
 SealedDepositDelta& operator=(const SealedDepositDelta&) = delete;
};

// FIXED verifier implemented in reactor.cpp. It authenticates and exactly
// re-encodes the ENTIRE supplied durable envelope, including every queue.
// A reference-store-v2 alone is accepted only as the no-queue form. No caller
// callback can substitute a store-only check for certification of other bytes.
CapellaForkStore verify_deposit_durable_checkpoint(std::string_view,
 const Bytes& digest,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,size_t cacheRecords,size_t cacheBytes,
 const CanonicalDepositContextLimits&,const DepositCheckpointTreeBudget&,
 CanonicalDepositContext::Fetch,CanonicalDepositContext::FetchLocator,
 const std::function<void()>&);
// The fixed reactor implementation verifies each supplied archive's real SSZ,
// exact retained header, slot, proposer and BLS signature before any commit.
void verify_deposit_publication_archives(const std::vector<CheckpointArchiveEntry>&,
 const CapellaForkStore&,const CapellaForkBlockRules&,
 const CapellaStoreSnapshotLimits&,const Checkpoint&);

class VerifiedDepositPublication {
 std::unique_ptr<DepositPublicationState> state_;
 explicit VerifiedDepositPublication(std::unique_ptr<DepositPublicationState>);
 friend class DepositPublicationFactory;
public:
 ~VerifiedDepositPublication();
 VerifiedDepositPublication(VerifiedDepositPublication&&) noexcept;
 VerifiedDepositPublication& operator=(VerifiedDepositPublication&&) noexcept;
 VerifiedDepositPublication(const VerifiedDepositPublication&) = delete;
 VerifiedDepositPublication& operator=(const VerifiedDepositPublication&) = delete;
};

struct DepositPrimaryReadUsage {uint64_t attempts, backing_calls;};
class DepositPrimaryLifetime {
 std::weak_ptr<DepositPrimaryGeneration> generation_;
 explicit DepositPrimaryLifetime(const std::shared_ptr<DepositPrimaryGeneration>&);
 friend class PreparedDepositPublication;
public:
 bool expired() const noexcept;
 void close() const;
};
class PreparedDepositPublication {
 std::unique_ptr<DepositPublicationState> state_;
 explicit PreparedDepositPublication(std::unique_ptr<DepositPublicationState>);
 friend class DepositPublicationFactory;
 friend class ::nano::ChainStore;
 // Called exactly once by the same living ChainStore BEFORE any I/O. A failed
 // attempt also consumes the ticket. Root serializes this with runtime close.
 void consume_for_commit(ChainStore&);
 const Int& chain_id() const noexcept;
 const Bytes& genesis_hash() const noexcept;
 const Bytes& expected_primary_tip() const noexcept;
 const std::optional<ForkCheckpointToken>& expected_previous_token() const noexcept;
 const Header& certified_execution_header() const noexcept;
 const std::string& manifest() const noexcept;
 const std::vector<CheckpointBlob>& blobs() const noexcept;
 const std::vector<CheckpointDepositLocatorRecord>& locators() const noexcept;
 const std::vector<CheckpointArchiveEntry>& archives() const noexcept;
 size_t maximum_bytes() const noexcept;
 size_t maximum_blobs() const noexcept;
 size_t maximum_locators() const noexcept;
 const ForkCheckpointToken& predicted_token() const noexcept;
 // Storage calls these only after its synchronous batch succeeds. There is no
 // public receipt, arming flag or fallible validation step after that write.
 void arm_after_commit() noexcept;
 ForkCheckpointToken take_return_token() noexcept;
public:
 ~PreparedDepositPublication();
 PreparedDepositPublication(PreparedDepositPublication&&) noexcept;
 PreparedDepositPublication& operator=(PreparedDepositPublication&&) noexcept;
 PreparedDepositPublication(const PreparedDepositPublication&) = delete;
 PreparedDepositPublication& operator=(const PreparedDepositPublication&) = delete;
 const CapellaForkStore& verified_store() const;
 std::shared_ptr<const CapellaForkStore> primary_store() const;
 const ForkCheckpointToken& next_token() const;
 DepositPrimaryReadUsage primary_read_usage() const;
 DepositPrimaryLifetime primary_lifetime() const;
 void close_primary();
 // Allocate this WEAK callback BEFORE commit; close under the reactor
 // mutex before destroying ChainStore. The raw owner address is a lifetime
 // contract, not protection against use after ChainStore destruction.
 std::function<void()> close_primary_callback() const;
};

class DepositPublicationFactory {
public:
 static VerifiedDepositPublication preflight(ChainStore& owner,Int chain,
  Bytes genesis,std::optional<ForkCheckpointToken> expected_previous,
  Bytes expected_primary_tip,const ConsensusRoot& expected_network,
  CapellaForkBlockRules,DepositPublicationLimits,std::string owned_manifest,
  SealedDepositDelta&&,const Checkpoint& = {});
 static VerifiedDepositPublication preflight(ChainStore& owner,Int chain,
  Bytes genesis,std::optional<ForkCheckpointToken> expected_previous,
  Bytes expected_primary_tip,const ConsensusRoot& expected_network,
  CapellaForkBlockRules,DepositPublicationLimits,std::string owned_manifest,
  SealedDepositDelta&&,std::vector<CheckpointArchiveEntry> owned_archives,
  const Checkpoint& = {});
 // Allocates a fresh primary owner/cache and copies certified metadata only.
 // No from_checkpoint, CAS fetch, locator lookup or pending closure is used.
 static PreparedDepositPublication prepare(VerifiedDepositPublication&&,
  const Checkpoint& = {});
};
}
