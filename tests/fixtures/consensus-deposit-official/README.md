# Official Phase0 deposit operation fixtures

All 17 minimal Phase0 operations/deposit/pyspec_tests cases from
ethereum/consensus-spec-tests revision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`.
Original Snappy, unmodified SSZ decompressions, sizes, hashes and MIT license
are retained. Fifteen cases have post-states; two invalid proof cases do not.
No Python or Snappy runtime is required by the C++ tests.

Expected outcomes: six new validator admissions, five top-ups, four proven
positions consumed without admission because the new-key signature is invalid,
and two rejected Merkle proofs. Top-ups do not require a new signature and
cannot replace withdrawal credentials. New deposits do not activate validators.

The test compares the validator registry including withdrawal credentials and
activation eligibility, slashing buckets, trusted deposit root/count and cursor.
Preset/config hashes are checked through ../consensus-slashing-official.
This is the legacy Eth1/Merkle deposit mechanism, not EIP-6110 execution requests,
complete BeaconState root validation, or proof of remote-chain finality.
