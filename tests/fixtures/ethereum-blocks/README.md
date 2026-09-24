# Official block execution regressions

Complete unmodified JSON files from `fixtures/blockchain_tests` in the pinned
`ethereum/execution-specs` release `tests@v20.0.0`; names replace `/` with `--`.
Hashes are recorded in `../ethereum-official.json` under
`bundledBlockRegressionFiles`. Distributed under CC0 (LICENSE.md).

This is a positive-case regression subset. The complete first diagnostic
covers 465 files: 7,369 matching cases, 779 unsupported cases, no mismatches
or adapter errors. The full diagnostic intentionally exits 1. It does not
prove full block validity or fork conformance. Invalid-block validation and
nonempty withdrawals remain unsupported and visible in the full report.

The adapter reads signed transaction bytes from block RLP and verifies header
encoding, genesis state, linear ancestry, uncle encoding, execution roots,
receipts, final state and final block hash. Proof-of-work seals and header
parent rules (difficulty, base fee, gas limit, timestamps) are not qualified.
The original fixture replay policy allows historical unprotected signatures;
Nano network replay policy is unchanged.

## Transaction rejection coverage

The extended full diagnostic matches 8,098 scenarios with 729 exact transaction
rejections and unchanged state. Fifty scenarios remain unsupported, so the
full diagnostic still exits 1. Initial and extended reports are both retained.
Additional complete files with negative scenarios are bundled and hashed; any
file above 10 MB is listed in `externalBlockRejectionRegressionFiles`.

## Parent header checks

The shared Nano gas-limit and base-fee checks now qualify 11 more rejections.
Full diagnostic: 8,109 matching scenarios, 740 atomic rejections, 39 unsupported
withdrawal/root cases, exit 1. Five complete header-regression files are added.
No fork activation or Ethereum consensus-seal validation is claimed.

## Withdrawals and current result

The current complete Berlin/Paris/Shanghai diagnostic passes all 8,148 cases
with zero errors or exclusions. The explicit Ethereum withdrawal policy is
used only by the fixture adapter; the Nano network defaults to rejection.
Twelve complete EIP-4895 files are included. Historical failures stay in the
manifest. Full Ethereum consensus, other forks and transitions are unproven.

## Cancun Beacon roots

Eight complete EIP-4788 files add 62 matching block cases. The full Cancun
block diagnostic remains in progress; these cases do not qualify blob header
validation, external consensus authentication or full Cancun conformance.
