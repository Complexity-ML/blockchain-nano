# WAN artifact selection prepared — 2 October 2026

Tested harness revision `dea3f827b068c0989179a5c99b0753d5ce9f7ed6` differs from active runtime candidate `9e9ebed3` only in three multihost JavaScript files. C++ source/include/network/third_party files are unchanged. Only those three tested files were cherry-picked to main; the optimized runtime remains isolated. Main and tested file hashes match.

`NANO_TEST_NETWORK_WITH_CORE=1` explicitly selects each owner's daemon from its campaign network directory. Remote Docker argv now translates the daemon path as well as the core/provenance/auditor paths. Prepared node JSON chooses AMD64's daemon for the VPS and ARM64's daemon for the Mac. Explicit native core/network selectors avoid the old shared daemon. Legacy paths remain available for old campaigns. Native path validation happens before tool calls or fixture creation; selections appear in the harness trace.

Validation: 15 path tests pass (11 existing plus four new cases). Two actual driver invocations with invalid selectors exit 1 before Git/SSH/SCP/Docker invocation or output creation, using tool spies. No real WAN run or new node was launched. Node syntax checked; path/hash routing tests qualify the harness changes only.

The planned artifact/environment file is **not authorization or a ready flag**. The new 427-test campaigns and queued 21-test supplements remain active/pending. Fixture-level EEST audits, signed AMD replay bound to the new binary, artifact re-attestation and pinned-host IPv6 routing must precede a real restore/duty/finality campaign. The old v2 pipeline process was observed absent; its saved waiting state is stale, not evidence of a running job. Its original failed 0f53 campaign remains refused.

After all gates pass, the real scenario must restore all three signer journal/witness files and peer identity, validate separately retained commitments, resume duties and advance at least three common finalized epochs. Every other production criterion remains open. No public deployment, funds, sale or consensus rule change occurred.
