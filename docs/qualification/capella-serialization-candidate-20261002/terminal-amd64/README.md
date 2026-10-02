# AMD64 Capella terminal — 9/10, one timeout

Source 3030b799. Container/CTest exit 8; 9 passes, one timeout, zero skips,
1933.90 seconds total. Reindexed activation times out at 360.04 seconds, last
marker slot 152/finality 17/six new votes. Original bounds unchanged. Simple
activation, activation restart and auto registration pass near 356 seconds.
Exit reindex passes in 426.80 seconds under its original 480-second bound.
All before/after source, executable and linked-input manifests match;
post-test manifest exit 0. The queued full AMD64 controller exited at its
all-ten-pass gate, without starting the full runtime campaign.

Within the successful exit-reindex case, both architectures have exactly equal
exit assertions and final activation assertions, including finality epoch 20,
root 0xf345c36fd1615d05920d8d23bf5fd03c5dd59ecef1777b2ecaf05097fc88f5ec,
nine votes, index 65, two reopened owners. This does not override the separate
360-second activation-case failure. In-process, synthetic clock, no transport;
not WAN convergence or production qualification.

A single diagnostic replay is now instrumented only after this campaign ended.
The first diagnostic attempt failed on missing /work fixture mount before
sampling; its failure is preserved. Corrected attempt B uses the same binary
and frozen fixture source, original 360-second bound and 30-second CPU sample.
An instrumented result earns no qualification. Diagnostic outcomes pending.
