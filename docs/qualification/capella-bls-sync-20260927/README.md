# Capella BLS admission sync failure and witnessed recovery — 2026-09-27

A new macOS native fault-injection scenario exercises BLS queue publication rather
than the existing received-block publication tests. No production code changed.
`consensus_capella_queued_bls_sync` passed all four dispositions in 6.21 s.

## What is tested

A child reconstructs an execution-bound reactor with its recovery witness, then
injects EIO on synchronization of the selected RocksDB WAL while admitting a valid
BLS withdrawal change. The existing test-only dyld interposer targets exactly the
temporary primary or witness WAL path. A separate unfaulted reactor constructs the
expected canonical durable envelope.

After the error, the child verifies reactor mutation/read operations and existing
owner/RPC reads are blocked. Primary errors block eight checked operations;
witness errors block six because the unaffected raw primary store remains
readable, while the reactor and owner are closed. The child exits without normal
destruction. The parent optionally truncates only that owned WAL's new suffix to
model losing the unconfirmed write, then reopens actual databases.

| Fault target | New WAL suffix | Pending operation after witnessed recovery |
| --- | --- | --- |
| Primary | Retained | Present |
| Primary | Removed | Present, recovered from witness |
| Witness | Retained | Present, replayed into primary |
| Witness | Removed | Absent; resubmission needed |

Each disposition checks the exact primary snapshot and sequence before recovery,
unchanged consensus state, paired execution head/state, expected pending queue,
resubmission and duplicate idempotency, a further reactor reopen and restored RPC
availability. The final canonical queue envelope must equal the unfaulted
reference and advance the original sequence exactly once. The last case must not
invent an operation absent from both retained journals.

## Evidence and initial harness correction

`tests.log` contains the four final outcomes. `initial-harness-failure.log` retains
the first failed attempt: the new test mistakenly constructed its Rpc object after
closing the owner through injected failure. Construction correctly failed with
`Beacon execution unavailable until checkpoint verification`. The test was fixed
to create the RPC before the fault, matching an already running server; no runtime
failure handling was weakened. `provenance.json` records source, runner and fault
library hashes after the final run. `source.patch` includes the new helper and
is reverse-checked against the final working tree.

## Limits

This is macOS EIO injection and controlled WAL-tail loss, not physical power loss,
Linux filesystem qualification, simultaneous corruption of independent devices,
or a SIGKILL at an arbitrary instruction. No network or real funds are involved.
The valid pending operation is restored/resubmitted here; later block inclusion
and payment are covered separately by the existing local and libp2p reports.
Production qualification still needs pruning, deep reorg/resource scenarios and
operational validation on the target systems. EIP-4895 remains partial.

Trailing whitespace in the initial failure log was normalized.
