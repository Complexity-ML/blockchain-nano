# Capella local duty timing — Docker ARM64, 2026-10-01

The scheduler now waits for an admitted selected-head block of the current slot,
or the authenticated profile attestation deadline, before its attestation/sync
phase. Attestation preparation already contained a timing guard; the gap was
that the scheduler continued to sync messages before that phase. Poll intervals
are bounded by that configured deadline. Consensus rules, signing journals,
BLS checks and WAN recovery/HTTP budgets are unchanged.

Docker uses the existing builder image
`sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4`,
existing `nano-linux-sync-build` volume, read-only source, network none,
`nofile=65536:65536`, and `cmake --build /build --target nano-capella-transition-tests -j 2`.
Then, sequentially: `--duty-timing`, `--scheduler`, `--automatic-recovery`.
The complete command finished exit 0. Exact source hashes and diff are retained.

New timing coverage: four isolated 64-key fixtures warmed to authenticated epoch
boundary 8, delayed valid block releases duties before deadline, malformed block
cannot release duties, no-block deadline fallback, no slot-9 backfill while
legitimate slot-10 proposal/votes/sync proceed, duplicate polls do not resign,
and a real worker with configured full-slot interval reaches deferred duties.
The scheduler additionally qualifies 40 blocks/272 votes/1280 sync messages,
finalized epoch 3, restart at slot 24 and 64 reopened journals.
Automatic recovery qualifies 16 blocks/64 keys and signer witnesses.

Preserved failed drafts: v1 used an anchor without the epoch target needed for
attestation; v2 incorrectly prohibited every signature in the next slot; v3
looked for `ssz` at the wrong JSON level rather than `envelope.ssz`. These are
fixture/test failures, not established consensus defects. v4 allows the next
slot and checks decoded archived votes, proposals and sync messages for absence
of missed-slot signing. No production gate was relaxed.

Scope: in-process Docker ARM64. No transport, WAN timing improvement, native Mac
or amd64 qualification of this revision yet. WAN M remains failed; this change
is not proven to explain its late recovery. Runtime nano images have not been
rebuilt or replaced by this test-only build. Production remains unqualified.
