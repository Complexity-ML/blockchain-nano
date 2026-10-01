# Transport CI recipe, prebuilt ARM64 qualification

Existing builder 6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4,
existing nano-linux-sync-build volume, 2 CPU/3 GiB, nofile 65536. Network disabled,
read-only container and source, capabilities dropped, writable reports/build
volume/tmpfs only. NANO_CI_REUSE_DEPS=1; no dependency download/build or new image.
Dependency source pin matches lock c03bf7c44c4fd30a390f3ef93bd1eab482e686a6.

Exact complete network CTest registry of 21 names and 21 JUnit cases verified.
All passed, no skips/failures/errors, 1.02 seconds. Default network targets
built/checked; existing nano-p2p binary hash unchanged ae567df...
Checkout at 0e22f8f with the new CI recipe still uncommitted at execution;
script snapshot hashes identify the tested recipe. Network sources unchanged.

First attempt stopped before configure because the existing builder lacks
Python. Verification moved to already-required CMake JSON handling, preserving
exact registrations and dependency pin checks. Eight routing/gate controls
pass locally, including three new controls and refusal of missing/duplicate
tests or a foreign dependency revision.

Fresh dependency construction on GitHub, AMD64 recipe and remaining global
production gates are not qualified by this reused ARM64 run. No WAN network,
validators, chain state or keys were mounted or launched.

After the successful suite, the registry gate was strengthened to refuse
DISABLED/SKIP_RETURN_CODE/SKIP_REGULAR_EXPRESSION properties. The updated CMake
gate was reapplied to the saved real 21-test registry without rerunning tests.
Eight local routing/gate controls still pass (1312.497166 ms), including an
actual CMake refusal of a disabled test. The snapshot hashes above identify
this final recipe; original test execution predates this metadata-only guard.
