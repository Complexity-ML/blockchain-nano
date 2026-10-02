# Bellatrix clock barrier: bounded ARM64 execution

Clean source 1ac57e25ff04f1d79b5c605acc2ae8ff1d73dcc9 includes code fix b0eb5ba1. Existing ARM64 builder digest 6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4; new isolated build /build/bellatrix-barrier-1ac57e25-20261002 in the existing volume. No image build or protocol tolerance change.

Five registered scenarios executed, all passed, no skipped tests; CTest exit 0, 31.69 seconds. They cover voluntary exit locally, real local P2P propagation, reorg, pruning after finality, and clock coalescing/discontinuity rejection. Sources and executable SHA256 hashes unchanged before/after. Result and inventory audits passed.

The preceding a00ff7d0 failure remains archived in the parent directory. Its first exception proved the synthetic observer jumped from 48000 to 60000 ms against a 6000 ms limit. The fix synchronizes the exit-inclusion slot before advancing the shared clock. Pruning now completes without weakening the guard.

This selection is not full regression, remote WAN, long-duration validator lifecycle, or AMD64 qualification. No main runtime promotion or production approval follows from these five cases.
