# AMD64 activation case passed; campaign still running

Candidate 3030b799. The original 360-second bound is unchanged. The case
completed in 356.26 seconds, leaving only 3.74 seconds of margin. This is not
a claim of adequate operating headroom or a controlled performance benchmark.

The actual terminal case output verifies deposit consumption at slot 80,
activation epoch 17, three new-validator votes, continued duties through slot
161, finality epoch 18, two reopened owners and idempotent block replay.
The enclosing ten-case campaign and its after-test binary/source/library
manifests are not terminal. The raw CTest log is explicitly an in-progress
copy; no final-root equality is inferred. Synthetic time, two in-process
independent stores, networkTransport false; no WAN or power-loss qualification.

The historical parent timeout remains preserved. Full AMD64 regression is
still gated on all ten cases passing and all terminal manifests matching.
