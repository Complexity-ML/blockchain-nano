# Official transaction envelope regressions

Complete unchanged files from `fixtures/transaction_tests` in the pinned
`ethereum/execution-specs` release `tests@v20.0.0`. File names replace `/` by
`--`. SHA-256 hashes are in `../ethereum-official.json`, under
`bundledTransactionRegressionFiles`. License: CC0, see LICENSE.md.

All 24 files contain 110 invalid EIP-7702 cases, 55 per fork (Prague/Osaka).
The native transaction runner checks exact rejection categories and unchanged
state. It uses the real signed-envelope decoder and, for decodable envelopes,
the execution validator with an explicitly synthetic funded account context.
It does not prove general transaction validity, network admission, positive
case coverage, full block validity, or fork conformance. Unsupported formats,
forks, positive cases, and unexpected exceptions cause a nonzero exit.

Initial diagnostic: 94 matched, 16 rejected with insufficiently specific errors.
Final diagnostic: 110 matched, no errors, mismatches, or skipped cases.
