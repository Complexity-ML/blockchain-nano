# Official Deneb voluntary exit / EIP-7044 fixtures

All 16 minimal Deneb operations/voluntary_exit/pyspec_tests cases from
ethereum/consensus-spec-tests revision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`.
Original Snappy, unmodified SSZ decompressions, sizes, hashes and MIT license
are retained. Six valid cases have official post-states; ten invalid cases do
not. The fixture runner requires no Python or Snappy runtime.

The test-only Deneb minimal projection reads validators/balances/slashing buckets,
with checks for the 32-member sync-committee layout, parallel participation and
inactivity lists, execution-header extra-data and historical summaries. It is
not a production SSZ parser or a complete BeaconState root check.

The runner selects CAPELLA_FORK_VERSION from pinned minimal-config.yaml in
../consensus-slashing-official. It verifies all official outputs, local frozen
domain/genesis rejection, replay and unchanged inputs on failure. Two official
cases explicitly distinguish the historical Phase0 domain rule from EIP-7044.
Changing both state fork versions while preserving Capella exercises perpetual
domain validity only; this is not a complete later-fork transition test.
