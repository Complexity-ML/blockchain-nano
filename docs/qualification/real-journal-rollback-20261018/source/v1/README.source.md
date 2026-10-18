# Real physical signing-journal rollback — source proposal

This is an unexecuted C++ source proposal, based on the current public API. It changes no Core code. Suggested destination: `tests/qualification/beacon_journal_physical_rollback.cpp`.

The sole CLI argument is a new absolute run directory whose physical parent exists. The program refuses an existing root and retains its output, including partial output on failure. It creates the current primary/witness pair through `BeaconSigningJournal`, closes every handle, and copies both complete stopped stores into a separate old pair. The existing readonly helper checks equal contents and distinct inodes. The same public test key/domain is then reopened on the current pair and signs exactly one proposal; the two real commitments advance from sequence 1 to 2. The test key, provider closure, signer instances and mutable stores are destroyed before checking.

`commitments.json` is created exclusively beside `old/` and `current/`, outside both trees. It contains the initial and recent tokens obtained from the actual stores. File and parent are fsynced; mode is 0400. Its captured FD9, known bytes and SHA are checked before and after each cold read. No recent floor is inferred from the old pair. This is an internally generated, separately retained synthetic test authority, not a claim that an operator or external service protects that authority from simultaneous rollback.

Two cold passes open all four stores strictly read-only and require their chain/namespaces and exact current pins. Core receives those actual records/current tokens. The old pair authenticates under old floors; the recent pair authenticates under recent floors. On the first pass, the old pair must throw exactly `std::invalid_argument("Invalid beacon signing journal")` under a recent primary floor alone, a recent witness floor alone, and both recent floors. An unrelated failure or arbitrary exception cannot satisfy a negative case.

Expected fixed counts: four database trees, two read-only passes/eight RO opens, seven Core authentications, four positives, three distinct expected refusals, two producer signer constructions, one provider call and one BLS signature. Checking constructs no signer and calls no provider. Both full stopped-tree inventories (FD9 and every regular-file SHA) and the external commitment remain unchanged. The shared helper is reused without modification.

Bounds: 180 seconds, observed peak RSS at most 128 MiB, aggregate payload of all four retained stores at most 32 MiB and 4096 entries. Cooperative checks cover bounded copying, hashing and Core calls; the invoking host/CTest must also enforce its process timeout. No process, socket, duty loop, recovery, existing database, private key file or production configuration is used. No production, common finality, operator restoration, signature-delivery history or historical digest-ancestry qualification is claimed. Rejecting an old pair under independently retained recent floors does not prove that the external floors themselves cannot be rolled back.

Proposed sibling CMake target, **not applied** (three commands):

```cmake
add_executable(nano-beacon-journal-physical-rollback tests/qualification/beacon_journal_physical_rollback.cpp)
target_link_libraries(nano-beacon-journal-physical-rollback PRIVATE nano_core)
target_compile_features(nano-beacon-journal-physical-rollback PRIVATE cxx_std_20)
```

No automatic CTest is proposed with a reused output path. Root can qualify Native/CMake and retained-Core Linux with a fresh run directory and an external 180-second process budget. No build, test, signature or database operation has been executed by the source author. Sources remain editable until Root review and explicit freeze.
