# Permanent executable manifest guards

The existing main tool recorded present helper programs, but only the Altair
probe was explicitly required when an argument was missing or non-executable.
Now `nano` and `nano-finality-crash-fixture` are required too. This prevents their
silent omission from the qualification manifest when supplied as arguments.
Explicit NODE_EXECUTABLE checks and data/directory discrimination are unchanged.

`node --test scripts/ci/docker-manifest.test.mjs`: 7/7 pass, zero skips. Two new
integration cases exercise real CMake with missing helpers, execute-bit removal,
and helper-only replacement while test-driver/network hashes stay fixed.
The five earlier guard tests also pass. This is qualification tooling coverage,
not Nano consensus/runtime qualification.

The active runtime checkout stays frozen at 3030b799 and does not contain this
main tooling update. Its full run keeps the legacy runner plus separate already
captured before-test hashes of the three known external helpers, with after-test
comparison pending. No active source or binary changed by this fix.
