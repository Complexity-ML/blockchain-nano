# Fixed-size validator SSZ tree — 27 September 2026

Production optimization: hash the eight-field Validator container using a stack
array and seven pair hashes. The 48-byte key uses exactly two padded chunks.
The previous generic container allocated vectors and recomputed unused zero
subtrees on each of a million records. Shape/boolean checks and bounded
cancellation checkpoints remain. No roots are trusted or cached across states;
input authentication and output root calculation both still execute in full.

Six suites pass: official Phase0 roots, Altair roots, Bellatrix containers,
Capella containers, Capella payload/withdrawal vectors, and large SSZ withdrawals.
Seventeen new mutations cover the boundaries of every Validator field, including
the second public-key chunk and boolean. Existing malformed/cancellation checks
remain. The scale runner now records pre/post roots for comparison.

| Validators | Before preparation µs | After preparation µs | Before root µs | After root µs |
| --- | ---: | ---: | ---: | ---: |
| 100000 | 436893 | 298736 | 226902 | 163519 |
| 1000000 | 4468134 | 3029580 | 2134565 | 1491193 |

Input and output roots match exactly before/after at both sizes, checked when
building comparison.json. Baseline is the parent production source with only
root logging added to the scale test. Candidate runs after changing the hasher.
One run per size/version on this Mac; no percentile or guaranteed speedup claim.
Peak RSS includes synthetic fixtures and validation copies, not just production.

At one million, preparation drops from ~4.47s to ~3.03s (about 32%). This remains
an important cost before BLS, full state transition, disk and networking. It does
not qualify runtime capacity, WAN, a million distributed validators, EIP closure
or production. Next: repeated whole-state authentication and Merkle work across
runtime steps, preserving explicit integrity/lifetime boundaries.
