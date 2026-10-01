# Explicit CTest budgets and corrected RPC harness — 2026-10-01

Fifteen registrations previously had no explicit timeout. Individual TIMEOUT
properties are now assigned: core/chain/rpc 120 s, Beacon errors 30 s,
bootstrap 20 s, migration 60 s, uncles 30 s, three expiry-loop scenarios
240 s (same budget as the adjacent validator-loop scenario), deposits/history,
exit identity, Capella withdrawal and secp256k1 60 s. Existing finite budgets
are unchanged. This is an operational test-runner bound, not a consensus limit.

Existing ARM64 builder and volume, no new image/download, network none,
2 CPU/3 GiB, read-only source/root, no capabilities, no-new-privileges.
The first exact 15-test attempt passed 14 and failed RPC (-32602) before timeout.
A method/type diagnostic reproduced `eth_estimateGas (params=object)`.
The test helper's implicit single-JSON argument conversion was the cause:
`{context_call}` did not produce a positional array. RPC correctly rejected it.

The helper now uses JSON's initializer-list type and explicit `Json::array`.
Two new checks assert a single object is one positional parameter and no
arguments means an empty positional array. Deliberately malformed fee-history
parameters are still assigned directly; runtime RPC validation is unchanged.
An intermediate compilation refusal is preserved (wrong initializer-list type
and missing loop braces); these errors were corrected before the successful run.

Only modified `rpc` was rebuilt/replayed after the correction: 993 checks pass,
0.58 s, CTest exit 0, no skips/errors. The other 14 prior passes are retained,
not rerun. This is combined scoped evidence, not a new all-15 run or complete
regression. Runtime nano/p2p hashes match the existing WAN candidates exactly.

The Docker configuration contains 400 registrations, all with explicit timeout.
Compared with the previous native 422 registry, 22 are absent due to optional
platform/tool configuration. They are listed in the manifest and remain separate
coverage requirements; Linux success must not silently close those scopes.
Full candidate regression, AMD64 budget qualification, sustained WAN/lifecycle,
EEST global coverage, bridge/launch and independent review remain open.
