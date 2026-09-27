# Capella withdrawal registry scale — 27 September 2026

PASS: six new cases (100,000 and 1,000,000 synthetic validators, each with dense,
sparse and absent withdrawals). Existing withdrawal/execution integration also
passes. No production source change was required by these cases.

Selection starts eight entries before the end of the registry, exercising wrap.
An independent closed-form expected list checks exact indices, addresses, amounts
and ordering; every candidate validator is compared with its expected post-state.
Source balances/cursors remain unchanged; the summed debit matches withdrawals.
Altered lists are refused for each size/distribution.

The selection checkpoint counts are 17 for 16 immediately eligible entries and
16,385 for sparse/empty scans (one entry check plus the 16,384 sweep). The result
capacity is 16, independently of total registry size. These are protocol parameters
in this test, not a cap on registration or distributed nodes.

| Registry | Distribution | Selection µs | Candidate µs | Peak process RSS bytes |
| --- | --- | ---: | ---: | ---: |
| 100000 | dense | 68 | 3866 | 46202880 |
| 100000 | sparse | 629 | 2919 | 46301184 |
| 100000 | empty | 562 | 2036 | 46301184 |
| 1000000 | dense | 9 | 32442 | 394657792 |
| 1000000 | sparse | 442 | 13788 | 394772480 |
| 1000000 | empty | 450 | 17208 | 394772480 |

Timing excludes fixture construction and post-result validation; candidate time
includes a second selection and a full registry copy. The full copy is O(N), even
when no withdrawals exist. Selection uses map lookups over a bounded sweep.
Peak RSS is the process high-water mark, not per-case or incremental allocation;
all six cases run sequentially in one process. One Release run on this Mac is
not a percentile, throughput target or node hardware recommendation.

Synthetic unique keys are assumed authenticated. SSZ decode/root authentication,
BLS, full transition, persistence and network are excluded. There are zero
network nodes in this benchmark. It does not prove one million active validators
in the live consensus, nor complete EIP-4895 or production readiness.

Next capacity measurement must include complete authenticated SSZ state
projection/root calculation and the actual runtime, rather than extrapolating
these registry-only times. Existing committee/registry benchmarks are retained.
