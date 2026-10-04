# Included majority votes — 4 October 2026

Read-only inspection of the stopped native dataset from the failed majority-island campaign, bound to the independently captured core checkpoint. 52 archived blocks, 140131 archived bytes; 331 complete dataset files unchanged. Inspector linked against the qualified d159 native libraries. Single-signer attestations independently identified by verifying their BLS signatures against registry public keys. No standalone votes or historical fork choice replayed; full canonical execution replay not performed in this inspection.

| Target epoch | Distinct signers on canonical target | Included before next epoch boundary |
|---|---:|---:|
| 4 | 50 | 45 |
| 5 | 43 | 38 |
| 6 | 43 | 35 |
| 7 | 43 | 39 |

Epochs5–7 contain all43 connected-island signers, but fewer than43 before the following boundary. The remaining attestations are included in the boundary block itself; epoch processing precedes that block body. Total included vote counts therefore do not establish immediate justification/finality at the boundary. This evidence changes the next diagnostic priority to FFG transition timing with late previous-epoch inclusion, rather than assuming a lost majority link or missing signatures. It does not establish a sole cause or convert the failed192-second qualification into a pass. Recoverability and stability remain unqualified.

Docker compilation succeeded using qualified ARM64 static libraries, but Linux RocksDB refused the Mac SST format7 (`stderr.log`). A newly linked native inspector succeeded. This is a concrete cross-platform physical-database restore limitation; no corruption established, and protocol compatibility is a separate question. Physical backups require compatible RocksDB readers or a supported logical export/import path before claiming cross-platform portability.

Source, public receipt, votes, exact hashes and summaries are retained here. Private databases, keys and node configurations remain outside Git. No new network, image rebuild, consensus change, deadline relaxation or production claim.
