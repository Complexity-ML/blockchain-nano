# Slashing retirement WAL failures — 2026-09-30

New `consensus_capella_slashing_sync` CTest: native Mac PASS in 137.75 s. Eight cases cover two real finalization boundaries (individual proof retirement with aggregate retained, then aggregate retirement) × primary/witness WAL fsync failure × retained/truncated unconfirmed suffix. Linux ARM64 PASS in 119.86 s; VPS x86-64 PASS in 447.12 s. Both containers exited 0 without OOM. Test revision: `36b799eb5f2df688e974a0510559ca40737d944d`. No runtime source changed.

The network fixture builds and admits signed Capella/Shanghai blocks with two offenders and 62 honest signing keys. At each boundary it closes the source databases, copies them into isolated test directories, computes the successful reference transition and requires exactly one checkpoint publication. A child injects an actual fsync error through the existing test library and exits without clean database shutdown. Only the copied test WAL suffix is truncated for simulated-loss cases.

The failed reactor refuses reads, further admission and RPC. Eight operations are blocked for primary failure, six for witness failure (the primary store has not itself failed in the latter case). Before recovery, the primary must exactly match the appropriate old/new checkpoint. Recovery through the independent witness must yield the full reference state; retry and duplicate operation must be idempotent. A second reopen checks the same complete snapshot, sequence, EVM head/state root, finality, finalized slashed flags and archive count. All eight cases passed; the uninterrupted reference chain reaches epoch 8 with 78 blocks and 566 votes.

## Limits

Controlled fsync failure and WAL suffix-loss simulation, not physical power-loss certification. Local native test, no transport partition or newly registered offender absent from the finalized registry. Automatic pruning is disabled in this scenario to isolate the single retirement publication; its combination with the lifecycle has separate prior evidence. The global production goal, remaining validator lifecycle gaps and independent security review stay open.

The archived logs were checked for all eight distinct combinations on each platform (24/24), successful recovery/retry flags, archive counts 1 then 0, and the same final epoch/block/vote counts. Binary/source and artifact hashes are in `manifest.json`; `verified-cases.json` records the checked coverage. Existing images and build volumes were reused.
