# Serial source-observation response projections

SOURCE ONLY proposal. No Node, tests, RPC/network, Docker, Git, project program, private/SDK reads or R writes were performed by author or peer. Root chooses and executes the targeted qualification. Source baseline is the public R reported at d6b6fee3; both original files are copied byte-exact under `baseline/`.

`observeBaseFinalizedReceipt` projects headers and receipts in a `.then` reaction. Its final `Promise.all` starts five request functions synchronously before those reactions run. An injected RPC using shared mutable cache objects can therefore return a divergent response, then erase it synchronously while serving a later request. The caller subsequently projects only the rewritten value. The current standalone HTTP transport parses fresh JSON; this bug concerns the observer's documented injected/cache API and must not be generalized into an exploit of all transports. The result remains ineligible for mint in all versions.

The proposal replaces only the five-request `Promise.all` with five sequential awaits of the existing header/receipt projection functions. Each earlier response is detached before the next request is invoked. Initial chain/header/receipt/code reads and the optional advanced-finalized header already use sequential awaits. No other concurrent request group remains in the observer. Request methods, successful-path counts, maximum counts, selectors, comparisons, API, code hash checks and result/trust flags are unchanged. A malformed response can stop a refused observation earlier, before later requests are started. RPC transport is byte unchanged. With the same per-request limits, serial latency can be greater than parallel latency; there is still no whole-observation timeout or hidden retry.

## One new causal group, three cases

`finality.test.mjs` retains the entire parent as an exact byte prefix and appends one parent group with three subtests. Each uses the existing pure RPC fixture, not a server or chain:

1. The second canonical receipt-height header returns a wrong hash; the subsequent second receipt call rewrites that original cached header to the good hash.
2. The old-finalized-height header returns a wrong hash; the following latest-finalized call rewrites its original cache to the good hash.
3. The second receipt returns changed event bytes; the following chain-ID call restores the original cached receipt bytes.

The unmodified parent starts the later calls before its projections and thus accepts each observation; each new `assert.rejects` must then fail with the missing-expected-rejection assertion. The candidate must reject with the precise corresponding canonical-header, finalized-checkpoint or receipt-change error. Each subtest also requires exactly one real cache collision, ten unchanged RPC methods with no retries, zero remaining in-flight requests, peak observer concurrency one, no send/sign and no safe/latest fallback. None of these expected outcomes has been executed or claimed as an actual result by the author.

For Root's causal qualification, copy the candidate test into separate fresh baseline/candidate trees and pair it respectively with `baseline/.../finality.mjs` and `source/.../finality.mjs`. On the baseline, select only `synchronous RPC cache collisions cannot erase already fulfilled divergences` with Node's test-name filter: three leaf cases must fail with missing expected rejection, plus their parent. Preserve those baseline failures; the older baseline groups are filtered. On the candidate, Root runs the COMPLETE existing test file: 31 historical Node results plus the three new leaf cases and their parent, 35 results in total. This candidate regression is justified by the changed asynchronous ordering and must have no failure or skip. No new CLI or runner is provided.

## Unchanged bridge limits

This closes response lifetime, not Base finality authentication. `cryptographicFinalityVerified:false`, `eligibleForMint:false` and `productionQualified:false` remain exact. RPC finalized/canonicality assertions still lack authenticated L1 derivation and receipt inclusion proof. No destination/asset/amount/recipient message domain, consumed-message contract, persistent atomic journal, restart reconciliation, reserve accounting, mint/burn/release or keys are added. Those are distinct material bridge gaps described by `docs/BRIDGE-SECURITY-REQUIREMENTS.md`, not flags this patch can enable.
