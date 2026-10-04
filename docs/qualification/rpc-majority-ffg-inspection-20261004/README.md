# Persisted FFG boundary inspection — 4 October 2026

Read-only inspection of the exact stopped d159 native majority fixture, bound to its external core checkpoint. Canonical stored states are advanced through any skipped slots to the end of epoch, then across the next boundary using the existing Capella slot/epoch implementation. A separate Python calculator implements Ethereum FFG weighting from the exported effective target balances, checkpoints, historical target roots and justification bits. All four available boundaries match both computed post-state and the first persisted canonical post-boundary block, including checkpoint roots. 331 complete input files unchanged.

| Boundary slot | Previous/current target participants | Justified epoch | Finalized epoch | Bits |
|---|---|---:|---:|---:|
| 40 | 64 / 45 | 4 | 3 | 15 |
| 48 | 50 / 38 | 4 | 3 | 14 |
| 56 | 43 / 35 | 5 | 4 | 14 |
| 64 | 43 / 39 | 6 | 4 | 14 |

Current-epoch target quorum is absent at the last three boundaries; previous-epoch quorum arrives later. The independent rules therefore reproduce epoch6 justified and epoch4 finalized at slot64. This explains the observed checkpoint progression on these states; it does not authenticate every historical fork-choice vote, reconstruct the network, or prove a performance bound. Only four available retained boundaries are checked; full canonical execution replay is not performed here. The old 192-second cut verdict remains failed, and recovery/stability remain unqualified.

Next coverage is a separately named sustained majority isolation (384s) to observe continuing finality beyond the fixed received-commitment ceiling and then recover in the unchanged180s window. This is stronger duration coverage, not a rewritten or relaxed192s result. d159 artifacts remain the basis; main's new SST-format correction is not part of those binaries and must qualify separately.

Reference: https://github.com/ethereum/consensus-specs/blob/master/specs/phase0/beacon-chain.md#justification-and-finalization and Altair participation flags. Verifier compares independent formulas, not a claim of running the full official consensus-spec test suite. No consensus source change or quorum modification. Production remains unqualified.
