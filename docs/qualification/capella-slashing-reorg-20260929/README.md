# Slashing inclusion reorg and durable replay — 2026-09-29

## Scope and current result

New `consensus_capella_slashing_reorg` CTest on the existing synthetic Capella / Shanghai transition fixture. Runtime sources are unchanged from `940736fd2078eb058289434cfefa2c2147df690d`; parent revision is `a21d0c27859c550276c990e0c391e547589b4c7b`. Native Mac result: PASS in 5.14 s; Linux ARM64: PASS in 3.99 s; VPS Linux x86-64: PASS in 16.87 s. Qualified test revision: `689db0c28592172e3e8a89446e555400f6787659`.

Two durable owners admit signed sibling blocks in opposite orders: one includes a real archived double-vote proof, the other omits it. Six distinct non-offender validators sign votes driving inclusion → omission → inclusion. After each selection, both owners reopen using their independent recovery witnesses. Checks compare full Beacon SSZ, offender registry/balance, checkpoint token, canonical execution block and execution state root.

The sibling balance difference equals the configured initial slashing penalty (1,000,000,000 Gwei for fixture offender 49). The archive survives the orphaned inclusion. A separate third durable owner automatically selects and includes that proof on the unslashed branch, survives reopening, and does not select it again for the following block. Duplicate block and reversed-proof admission must leave its checkpoint unchanged. Total: two branch switches and seven owner reopens.

These are signed public test keys and valid transition admission, not a bypass of block or vote validation. Conflicting proposals deliberately construct an adversarial fork.

## Failure retained

The initial new test failed after 4.63 s with `reorg local body unavailable`. Its final next-block assembly requested the next slot without advancing the reactor clock. The test now ticks explicitly before that request, respecting the existing runtime slot guard. No runtime guard, quorum or validation rule was changed. The failure and successful result are both retained.

## Limits and next gates

Local native consensus, one offender, no P2P transport, no injected write failure, no actual power loss. This does not newly qualify retirement after finalization, multi-offender partial finalization, unknown-validator retention or failed retirement writes. Existing pruning/retirement results remain separate evidence. No EVM fork or global production readiness is claimed. The shared runner regression passes on Mac (13.76 s), Linux ARM64 (9.94 s) and VPS Linux x86-64 (42.56 s). Both Linux containers exited 0 without OOM. This closes only the single-offender inclusion-reorg/restart qualification. Images and build volumes were reused; no public node was deployed.
