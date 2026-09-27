# Authenticated SSZ withdrawal scale — 27 September 2026

Correctness PASS, capacity bottleneck demonstrated. Two synthetic full SSZ states
extend a digest-checked official Capella fixture to 100,000 and 1,000,000 entries.
Other vector sizes retain the minimal preset; this is not a mainnet-preset node.
Keys are synthetic and no BLS or lifecycle validity is implied by SSZ validity.

The test measures initial root computation separately, then calls the production
`prepare_capella_withdrawal_state` path (input root authentication, registry
projection, withdrawal selection/copy, balance changes and full output root).
It compares every output byte with an independently patched expected state:
16 wraparound withdrawals, only their balances and the two cursors may change.
The source hash remains unchanged and an incorrect authenticated root is rejected.

| Validators | SSZ bytes | Initial root µs | Withdrawal preparation µs | Peak process RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| 100000 | 13910821 | 215134 | 422669 | 95354880 |
| 1000000 | 139010821 | 2068384 | 4258573 | 875020288 |

One Release run on this Mac; timings exclude fixture construction, independent
post-check and negative control. RSS is the high-water mark including test copies,
not incremental production memory. State budget is 256 MiB; timeout is 180s.

At one million entries, ~4.26s for this isolated operation already consumes a
substantial part of a short slot, before BLS, the rest of the transition, storage
or transport. The previous ~14–32ms registry-only measurements cannot establish
runtime scalability. No arbitrary registry cap was raised and no validation was
removed. Production source is unchanged.

Next work: profile full-state Merkle hashing and repeated authentication along the
runtime path; reuse only state/root associations whose integrity and lifetime are
established, or implement verified incremental Merkle updates. Never skip root
validation solely because a benchmark is slow. Re-measure the whole transition
and finality after changes. This is zero distributed nodes, not proof of a live
million-validator consensus. EIP-4895 and overall production remain open.
