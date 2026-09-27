# Capella configurable local runtime — 2026-09-27

Base revision `4a07104`, plus accompanying runtime and test sources.

## Implementation

`start_capella_runtime` owns a coordinator, local clock, optional validator
scheduler, checkpoint witness and per-key journals/witnesses through shared lifetime
aliases. Workers stop before the corresponding coordinator/storage is destroyed.
The Blockchain owner must outlive all aliases. `stop_capella_runtime` stops duties
and clock, then returns exact consensus/signing commitments and observed status.
The caller must stop external admission before requesting these receipts.

Configuration uses canonical hex quantities and rejects unknown fields:
- `profile`: bounded local file, expected SHA-256, maximum bytes;
- `limits`: envelope/blocks/messages/equivocations/state/replay capacities;
- `initial`: authenticated snapshot file and SHA-256, or null;
- `restart`: independently retained sequence/digest, or null;
- optional `recoveryWitness`: separate trusted directory (exclusive of restart);
- `clock`: interval and maximum forward step in milliseconds;
- optional `validator`: poll interval, transaction/vote trial budgets, keys;
- each key: registry index, fee recipient, environment variable naming the BLS
  scalar, journal path, explicit restart token or optional recovery witness.

Profile chain ID and protected transactions must match a Beacon Shanghai owner.
Loaded key public identities are checked against the authenticated validator
registry; duplicate indices/keys/journal paths, invalid scalars and invalid timing
are rejected. Transient scalar buffers are cleansed. No keys are logged.

## New evidence

`consensus_capella_configured_runtime` passed in 21.43 seconds. It loads 64 synthetic
BLS keys via environment variables, submits 16 transactions to the mempool,
and produces 16 blocks. Mid-run it stops workers, checks receipts, closes the
runtime/owner and restarts using the consensus witness plus all signing witnesses,
without externally supplied restart tokens. Production continues afterward.
Unknown configuration field, wrong profile digest, zero interval and mismatching
validator key are refused; failed key loading and runtime destruction close owner
reads. Test environment variables are removed afterward.

The local background workers are started by the factory; deterministic atomic
wall time and explicit scheduler polls drive the scenario. This is not a timed
WAN/load benchmark. No pre-existing Bellatrix implementation was modified.

## Remaining scope

This is a C++ runtime factory, not yet CLI dispatch or public JSON genesis fork
activation. No Capella P2P registration, private RPC worker or Beacon HTTP service
is connected. Shanghai Beacon genesis remains programmatic in tests. Mixed-fork
Store migration and remaining consensus operation assembly are still open.
Witness directories require a trustworthy operational placement/backup policy;
coordinated rollback of all trusted stores is not prevented by this feature.
