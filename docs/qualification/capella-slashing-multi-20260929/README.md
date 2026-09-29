# Partial finalization of overlapping slashing evidence — 2026-09-29

## New coverage

`consensus_capella_slashing_multi` uses two durable execution owners and signed native Capella/Shanghai blocks. It admits a proof against validator 0, then an aggregated BLS double-vote proof against validators 0 and 1. The overlapping proof must be accepted because offender 1 is not yet covered. Reversing the aggregated proof must not change its checkpoint.

The proposer deliberately includes only the single-offender proof until validator 0 is finalized slashed. Blocks pass normal signing and admission; no registry/state mutation or transition bypass is used. The archive must retain the aggregate proof while validator 1 remains unslashed in the finalized state. Normal body selection then includes the remaining sanction. Both proofs must disappear only when their respective offenders are finalized slashed. Archive contents/counts are checked after every produced block and after the final reopening; aggregate replay must not republish or regrow it.

Native PASS in 65.46 s: 70 blocks, 504 votes, finalized epoch 7, six pruning anchor moves, 62 reopened signing journals and both execution stores reopened. Linux qualification and the existing single-offender regression are pending. Runtime sources remain unchanged from `940736fd2078eb058289434cfefa2c2147df690d`.

## Retained failure

Initial run failed after 1.89 s at `local loop RANDAO missing`. The fixture selected an offender with known equivocation before on-chain slashing; the existing signer correctly refused. The harness now leaves selected equivocal proposer slots empty (no substitute/reroll and no disabled signing guard). Two slots were omitted in the successful run. The initial log is retained.

## Limits

Two synthetic offenders and local native consensus; no WAN, newly registered validator absent from finalized registry, injected WAL failure or physical power loss. Failed writes at archive retirement remain the next gate. This closes neither the entire slashing lifecycle nor production readiness, and changes no EVM/EIP compatibility claim.
