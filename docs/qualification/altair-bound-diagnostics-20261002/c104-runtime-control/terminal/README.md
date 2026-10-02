# Altair full-block boundary diagnostic on original c104 AMD64

Single original consensus_altair_libp2p case failed in 2.17 seconds. Original
120-second limit and admission assertions unchanged. Runtime/header/static
libraries and fixtures are c104; only the test translation unit is instrumented
from f92533e9. This diagnostic composition is not a coherent production candidate.
Exact result, source/program hashes, compiler/link commands and link-input hashes
are retained. Source, programs and link inputs compare identically before/after;
post manifest exit 0. Result audit exit 1 preserves the failure.

The full-bound payload is 8 388 608 bytes, JSON wire frame 16 777 370 bytes.
Reply is [null], highWater stays 0x1, checkpoint unchanged and clock healthy.
The recent completion list contains invalid-envelope and an earlier rejected ACK;
it does not identify a completion for the full-bound frame. These facts do not
prove the precise transport failure. Probe write failure versus remote stream
handling remains to be distinguished. No runtime change or weakened limit is
justified from this output alone. The underlying c104 full-campaign failure remains.

Text reports are archived; logs are compressed without altering decompressed
bytes. Transient scratch/ctest directories and binary/object build products are
not copied into Git; build-product hashes are recorded separately and the exact
program manifest remains. This archive performs no extra runtime test.
