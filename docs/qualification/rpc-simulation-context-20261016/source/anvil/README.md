# Oracle V2: exact Anvil client version and preparation diagnostics

SOURCE ONLY; no Node, Anvil, Docker or tests have been executed by this author. The frozen V1 package remains unchanged. Root reported its first actual run stopped before case zero at `Anvil 1.5.1 readiness`, while its owned Anvil remained alive. The old substring expression required a word boundary before `1`: in the normal `anvil/v1.5.1`, `v` and `1` are both word characters, so that boundary is absent.

The only changes to the runner are the anchored version expression and bounded preparation diagnostics. Accepted names are exactly `anvil/1.5.1` or `anvil/v1.5.1`, optionally followed by nonempty ASCII SemVer-style prerelease/build identifiers introduced by `-`/`+`. Input length is at most 256 characters. Other clients, versions such as 1.5.10/1.5.2, prefixes/suffix text, whitespace and empty suffixes are refused. The official tagged source defines `CLIENT_VERSION` as `anvil/v` followed by `CARGO_PKG_VERSION`, and its `web3_clientVersion` handler returns that value: [Foundry v1.5.1 API](https://github.com/foundry-rs/foundry/blob/v1.5.1/crates/anvil/src/eth/api.rs#L101-L102), [handler](https://github.com/foundry-rs/foundry/blob/v1.5.1/crates/anvil/src/eth/api.rs#L574-L579).

`readinessObservations` now retains the fork, observed client version (bounded to 256 characters), last preparation error (2048 characters), attempt count and ready flag, even when preparation refuses. A preparation refusal includes this observation in `firstFailure`. A successful observation may still show the last transient preparation error; its `ready` flag distinguishes success. These are observations of the fixed local RPC only. No CLI version, admitted binary SHA/stat material, chain check, fixture, comparison policy, time budget or cleanup rule is relaxed.

All 258 candidate records and both sequential Paris/Shanghai profiles remain unchanged. Root will rerun the oracle using its public complete candidate copy; V1's zero-case refusal stays preserved. No parity or actual V2 success is claimed by this source. `source.patch` is the complete V1-to-V2 runner delta. The invocation and limits below remain those of V1, using this V2 `runner.mjs` path.

---

# Local Anvil oracle for RPC simulation case records

SOURCE ONLY. Root is the sole operator. The author has not run Node, Anvil, a compiler, Docker or any test. This proposal neither starts a campaign nor qualifies production, a global SDK, signed transaction admission or consensus.

The input is the public stdout from one successful `--simulation-context-self-test` candidate run: four JSONL group events and its summary, or an array of the four group objects with an optional fifth summary. Exact group order, cumulative checks 104/264/336/476, refusals 0/6/6/126, and record counts 34/66/32/126 are required. The 258 records must contain only `eth_call`, `eth_estimateGas` and `eth_createAccessList` with an explicit `latest` selector. Root must select and pin the successful candidate output; accepting this shape does not establish its source provenance. In particular, the V2 test requires a successful child CREATE or a parent REVERT; the preserved V1 failure is not a complete candidate input.

Invocation, reserved for Root:

```sh
ANVIL=/absolute/physical/admitted/anvil OUT=/absolute/new/output \
  node /absolute/path/runner.mjs /absolute/public/candidate-events.jsonl
```

`ANVIL` must name the already admitted official Linux Anvil 1.5.1 binary. Root retains its original authority and supplies the admitted Node/runtime and resource envelope. The runner captures and finally rechecks exact FD/path BigInt stat-nine plus SHA for this binary and input; those observations do not recover an earlier authority. `OUT` must not exist, its parent must be physical, and its creation is exclusive with mode 0700. Inputs are limited to 4 MiB and the selected binary to 64 MiB. No private configuration, wallet, seed, endpoint credential, library map or package installation is consumed.

The runner creates two sequential, owned Anvil children, one Paris and one Shanghai. Each has chain 1337, zero accounts, gas limit 30000000 and base fee 1000000000. A temporarily reserved loopback port is released immediately before spawn; readiness must then verify the exact Anvil version and chain. There is no fork URL, transaction, signature, mining or public port publication. Docker `--network none` supports the private loopback used inside that container; Root must not publish these ports or supply upstream networking. The small port handoff race remains fail-closed at readiness.

Only the fixture's public balances, nonces and bytecodes are installed, using the three fixed Anvil setters. Nonempty fixture storage is rejected. Readback verifies these values and slot zero, and verifies the requested block gas limit/base fee. All groups must carry identical fixtures for each fork. After 129 records per fork the header hash and allocated balance/nonce/code/slot-zero observations must be unchanged. This is not a dump or equality proof of every storage slot or the complete Anvil world state.

Comparison is deliberately explicit:

- `eth_call`: exact return bytes for successful results; otherwise recognized refusal classes. Unknown errors are mismatches, and complete candidate/Anvil responses remain in the trace.
- `eth_estimateGas`: positive bounded quotes and actual Anvil `eth_call` replays at both quotes. Differing valid quotes remain recorded as informational differences; numeric quote equality is not required. A successful replay means the parent execution succeeded. The CREATE fixture's own REVERT guard supplies its stronger child-success requirement.
- `eth_createAccessList`: normalized addresses/storage keys, exact `gasUsed`, and presence/class of its embedded execution error. The candidate's two explicit zero-fee rejection policies can be labelled as documented only when Anvil succeeds and the sole difference is the result class. Other errors, transport failures and access-list differences cannot be excused by this policy. Default fee behavior is not silently rewritten to manufacture parity.

Every case produces one trace record, including RPC/comparison failures. The record contains the original request, actual candidate response, actual Anvil response when one was received, differences and estimate replays. All bytes are written and fsynced before incrementing the compared counter. The runner continues to collect individual mismatches within its overall deadline. `result-public.json` distinguishes completion, unexpected comparisons, documented policy differences, informational quote differences and exact equality of the compared values. `passed` requires all 258 cases to have been collected, no unexpected comparison, no final-fence failure and successful cleanup. A passing qualitative comparison with recorded quote/policy differences is not exact value parity.

The monotonic run budget is 180 seconds, each RPC/body is limited to 3 seconds and 256 KiB, each child log to 1 MiB, and the comparison trace to 16 MiB. These are refusal bounds, not a promise that arbitrary providers or file systems respond within them. Synchronous local file I/O/fsync cannot be preempted by a JavaScript timer. Root supplies outer CPU/memory/process limits and a final wall-time envelope; the runner does not install a resource controller. The fixed local RPC methods have no remote transport or arbitrary command interface.

Only the handle returned by this runner's Anvil spawn can be signalled. Cleanup sends TERM with a two-second wait, then KILL with a two-second join bound if the same child remains live. It waits for the child pipe closure, closes logs even if fsync fails, and preserves the first refusal while recording cleanup errors. No PID search, existing service, Docker object, image, volume or PID 28722 is targeted. Signal interruption aborts an active HTTP request and reaches this same cleanup. Root must run the script in a dedicated environment; this source does not authorize any operation itself.

Outputs are `inputs-public.json`, `comparisons.jsonl`, two bounded Anvil logs, and `result-public.json`, all under the new output directory. No actual comparison result is claimed by this source package. Node syntax and all real Anvil behavior are pending Root execution.
