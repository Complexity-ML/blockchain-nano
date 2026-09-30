# Page yielding runtime images — 30 September 2026

Both cached Docker volumes built nano and nano-p2p from d0cc038; the revision marker was updated after successful target builds. Only nano-p2p executable hashes changed from the clock-diagnostic candidates. Runtime packaging reused the original immutable runtime bases, checked executable hashes, shared libraries and --help. Independent post-packaging checksum probes confirmed installed binaries; architecture, source labels and UID/GID are recorded. Native P2P was rebuilt; core sources are Git-equivalent to 10e2f9e.

Relay tests are in catalog-page-yield-20260930 (39 scenarios, 4 new, two architectures). Images are preparation for another finite WAN qualification, not evidence that recovery or production gates pass. No consensus/finality/clock rules or deadlines were changed.
