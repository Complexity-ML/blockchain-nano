# ARM64 missing-member finality advancement — passed

New scenario source c2e9e26, Docker test passes in 163.22 s with 2 CPU / 3 GiB / nofile 65536 in the existing build volume. One unknown-member finalized advancement, two durable owners reopened before and after that advancement, identical proof archive and checkpoint on reopen, finalized sanction retires evidence, 16 forged-proof refusals. Actual outputs in result.json and last-test.log.

Core/network sources remain identical to runtime 74784c4 (empty source diff). Recorded core and P2P hashes equal the existing qualified candidate; no new runtime image. This adds a native durable-state scenario, not network transport, physical power loss or complete production readiness. AMD64 qualification still pending at archive creation. First failed fixture retained.
