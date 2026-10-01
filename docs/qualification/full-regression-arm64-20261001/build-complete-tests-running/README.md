# Complete Linux build succeeded; 400-test campaign still running

Baseline 78fd880 plus archived fixture-only source patch/header/hash map.
All default targets compiled successfully after portability repair. Exact
400-name guard verifies command presence, positive budgets and no skips.
Nano and nano-p2p hashes exactly match the prior ARM64 WAN candidate.
No new runtime image or core source change. Existing builder/volume, network
none, 2 CPU/3 GiB, read-only root/source, no capabilities/no-new-privileges.

The log is a live snapshot, not a terminal JUnit or full-suite result.
Manifest records only finished test lines at capture. Campaign handle is
exec session 32017, Docker nano-full-regression-arm64-20261001-b.
Continue that same handle; don't relaunch because a poll expires.
The earlier failed compiler log is preserved. 13 Linux registration omissions
and 9 optional tooling/corpus registrations remain separate gates. The prepared
Linux injection patch has not been applied while this campaign is running.
All full production requirements remain open until their own evidence exists.
