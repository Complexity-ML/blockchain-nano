# Official Phase0 proposer slashing operation fixtures

All 15 cases at `tests/minimal/phase0/operations/proposer_slashing/pyspec_tests`
from ethereum/consensus-spec-tests revision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`.
Original SSZ Snappy, unmodified decompressed SSZ, hashes and upstream MIT
license are retained. Valid cases have official post-state files; invalid
cases have no post-state. No Python/Snappy test runtime is needed.

Three valid cases: basic, block_header_from_future, slashed_and_proposer_index_the_same.
Twelve invalid cases cover differing indices/slots, repeated headers, signature
errors, inactivity, previous slashing and withdrawal. One invalid component
pre-state (`invalid_proposer_is_withdrawn`) has withdrawable_epoch=0 with
exit_epoch=FAR_FUTURE; Nano rejects its inconsistent registry before checking
its proof. The test reports this separately and preserves strict validation.

The C++ runner compares the full slashing-state projection: all balances,
public keys, effective balances, activation/exit/withdrawal epochs, flags and
slashing buckets. It does not compare full BeaconState SSZ roots or execute an
entire block. Minimal preset/config files and their pinned source hashes come
from ../consensus-slashing-official.
