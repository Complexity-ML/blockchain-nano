# Direct authenticated SSZ withdrawal sweep — 27 September 2026

The SSZ withdrawal candidate no longer materializes and copies a complete map
of validators. It authenticates the entire input root, validates list
cardinalities, then reads the bounded selection window directly from SSZ.
The map and SSZ paths share one selection implementation (full/partial eligibility,
ordering, index overflow, amount/address extraction and wraparound).

After exact comparison with the advertised withdrawals, only the private output
byte vector is copied and changed. The full post-state root is still calculated.
The public registry projection API remains available and is still used by other
paths; this change does not eliminate all runtime registry projections.

Five suites pass: withdrawals/execution, official Capella payload/withdrawals,
paired EVM+consensus state, registry-scale and SSZ-scale. Two new cancellation
cases interrupt the direct selection and late post-root hashing without source
mutation. Official corpus includes 35 matching states and 19 rejected withdrawal
lists. Existing dense/sparse/empty cases and full SSZ byte comparisons pass.

| Validators | Previous peak RSS | New peak RSS | Previous preparation µs | New preparation µs |
| --- | ---: | ---: | ---: | ---: |
| 100000 | 95338496 | 68714496 | 298736 | 291692 |
| 1000000 | 854589440 | 639205376 | 3029580 | 2911148 |

Input/output roots match the prior fixed-tree run at both sizes. Peak process
RSS includes synthetic fixtures and copies used by test validation, not just
production allocations. One measured run per version; no latency guarantee.
The million-entry case remains ~2.91s: hashing still dominates. Other SSZ vectors
use the minimal preset. No BLS, complete transition, disk or network qualification.

No validation was bypassed and no validator cap was raised. Production and EIP
closure remain open; next work must address repeated whole-state Merkle work
with explicit integrity boundaries, then measure actual runtime cost.
