# AMD64 missing-member finality advancement — passed

Test source c2e9e26, pulled from Git before compiling in existing Docker build volume. New test passes in 585.94 s under unchanged 600 s CTest limit (2 CPU / 3 GiB / nofile 65536); this is a compressed durable-state fixture runtime, not a network recovery latency. Timing remains recorded, no deadline increase.

One actual unknown-member finality advancement; 64 missing-member observations; two owners reopened before and after advancement with identical committed checkpoint/head/proof archive; reverse replay preserves checkpoint; finalized sanction retires proof; 16 forged-proof refusals. See result.json and last-test.log.

Core/network input diff against runtime 74784c4 is empty. Core/P2P hashes match the existing reviewed candidate, so no runtime image rebuild or relabelling. No network transport or physical power-loss claim. This closes this missing-member retention scenario on native, ARM64 and AMD64; the rest of the original production criteria remain open. First failed fixture is preserved.
