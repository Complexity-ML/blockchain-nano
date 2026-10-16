# Full payout aggregate Gwei boundary

Source-only proposal. The terminal auditor emits an exact GMP sum of positive uint64 withdrawals. The JavaScript terminal validator previously applied a single-withdrawal uint64 bound to that sum, rejecting valid totals above UINT64_MAX.

Only terminal total validation changes: retain canonical lowercase hexadecimal, parse as BigInt, and require `fullPayouts <= totalGwei <= fullPayouts * UINT64_MAX`. The existing safe-integer count and minimum two payouts are checked first. All individual uint64 quantities, uint256 Wei credits, epochs, identities and other terminal guards remain unchanged. No C++, RPC, CSV, header66819 or node runtime implementation is modified.

The new pure test file has two groups and ten expected refusals. It covers the minimum positive aggregate, two and three maximal withdrawals, zero/under-count/over-bound totals, unsafe counter, noncanonical/nonstring totals, unchanged slot uint64, the individual UINT64_MAX boundary and exact uint256 recipient balances. Existing lifecycle-runtime.test.mjs is unchanged and need not be replayed for this proposal.

ROOT may place the complete overlay and new test in a separate source replica with the unchanged lifecycle.mjs and lifecycle-owner.mjs imports, then run only `node --test tests/multihost/lifecycle-payout-total.test.mjs`. The author executed no Node/test/project/SDK/Git operation. Test results are pending ROOT; the target branch and consumer source stay untouched until its ongoing qualification is complete. This parser fix does not qualify actual full payouts, withdrawals end to end, restart safety, HTTP/exec transport, duties, finality or production.
