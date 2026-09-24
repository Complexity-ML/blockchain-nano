# Official Phase0 voluntary exit fixtures

All 10 minimal Phase0 operations/voluntary_exit/pyspec_tests cases from
ethereum/consensus-spec-tests revision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`.
The manifest records original paths, sizes and SHA-256 values. Original Snappy,
unmodified decompressed SSZ and upstream MIT license are retained. No Python
or Snappy runtime is needed by the C++ tests.

Four valid cases cover basic exit, a subsequent/default-epoch exit and minimum
or scaled churn queues. Six invalid cases cover signature, already exited,
future request, unknown index, inactive validator and insufficient active tenure.
The runner compares every projected validator and all slashing buckets with
unmodified official post-states, including balances remaining unchanged.

Preset/config and hashes come from ../consensus-slashing-official. This is
Phase0 semantics: the signed exit epoch selects the domain. It does not qualify
EIP-7044's Capella-domain rule, EIP-7002 withdrawal-key requests, payouts,
complete BeaconState roots, network activation or entire block transitions.
