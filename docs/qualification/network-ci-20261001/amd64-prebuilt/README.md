# AMD64 transport CI recipe qualification

VPS Git pull at 85a4a7c, existing builder
0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c and
nano-linux-sync-build volume, NANO_CI_REUSE_DEPS=1. Container/network/source
read-only where applicable, no network, capabilities dropped, 2 CPU/3 GiB,
nofile 65536, writable build volume/reports/tmpfs only, no chain/key mounts.

Exact 21-test registry and 21 JUnit cases verified; all pass, no failures,
errors or skips, 1.24 seconds. nano-p2p binary hash unchanged 2bf67374...
Default network targets checked/built; existing dependencies reused, no
download or new image. Fresh GitHub dependency construction still pending.

Two environment failures happened before tests: missing nested report
mountpoint under read-only source, then report directory not writable by the
capability-free builder UID. Both preserved. Created only the ignored empty
source mountpoint and changed the temporary report directory ownership to
root, matching the builder UID. No capability or read-only relaxation.
This does not qualify a WAN topology or complete production readiness.
