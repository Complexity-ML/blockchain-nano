# WAN partition cycles N — 2026-10-01

Runtime `1e1e73208a96268e060c9ac92a871ec0eadab4f1`, harness
`06004e27967a31aa6c93177a426d9de7f62e7419`; frozen sources/images/configuration
throughout. Three processes, two physical hosts, 64 synthetic keys (22/21/21),
Capella/Shanghai, 8 slots/epoch and 6 seconds/slot. Packaged ARM64/amd64 images
and native hashes are in the preceding duty-timing candidate archive. Optional
network trace disabled, Docker nofile 65536, finite 96/192-second cuts.

Command terminated exit 1. First recovery: finalized 2→7 in 140894.719167 ms
(monotonic), below the unchanged 180000 ms limit. Second reconnect wall timestamp
1790847525106; first complete common-finality read 7→15 at 1790847705233. Wall
difference 180127 ms. The strict monotonic gate rejected it; its exact complete
monotonic elapsed value was not logged. This is a failure, not a tolerated margin.
The late diagnostic's 2257.034708 ms is a NEW observation window, not the original
recovery duration. No Beacon HTTP error interrupted this campaign.

After late recovery, stable common finality advanced 15→18 in 144311 ms. All
three signers/clocks were healthy at stop (425/402/401 admitted attestations).
Both containers exited 0 without OOM. Journals, witnesses and databases remain
outside Git; only public test logs/statuses are retained here. Root consistency
in sampled observations and this stability scope do not qualify the whole network.

The scheduler counter shows deferred enumeration, but this did not close the
recovery gate. The scope correction archive proves sync and attestation signing
guards already existed before this change. No new signature vulnerability was
closed; neither a WAN improvement nor M's cause is established. Do not rebuild or
repeat the same candidate to obtain green. Preserve original criteria and earlier
failures; investigate actual convergence/vote inclusion and observation costs,
then qualify genuinely new coverage. Production and bridge/fair launch remain open.
