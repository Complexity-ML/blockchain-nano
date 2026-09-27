# Disjoint sync participation merging — 2026-09-27

## Implementation

Native aggregate admission now merges two verified, nonempty sync aggregates
when their committee-position bitfields are disjoint and their slot/parent
context is identical. It sums their compressed G2 signatures, unions their bits
and revalidates the combined aggregate before replacing the ephemeral cache.
Overlapping inputs are never summed. Exact replay is idempotent; otherwise an
overlapping input only replaces the cache if its participation count is higher.
The cache remains bounded to one aggregate. This is not an optimal arbitrary
set-cover algorithm and it cannot recover individual signatures from overlap.

The new BLS signature-sum primitive rejects empty input, wrong widths, decoding
failures, points outside G2, infinity inputs and an infinity result. It retains
multiplicity; committee positions, not unique public keys, define participation.
The primitive alone does not authenticate messages or membership. Native
admission verifies both contributions in their authenticated state context.

## Qualification

The existing two-owner local-sync test is strengthened: on each even slot it
constructs complementary even/odd-position aggregates independently of the
production helper. It submits the halves in alternating orders, repeats the
first half, merges the second and resubmits an overlapping subset. The resulting
body must exactly equal the independently constructed full aggregate. Full
replay, empty downgrade, corrupted signature, future slot, stale checkpoint,
cancellation and odd-slot expiry checks remain. Both nodes execute and finalize
the assembled blocks.

The BLS aggregate runner adds singleton preservation, duplicated signature/key
multiplicity, cancelling points, invalid widths/encodings and infinity checks
to its official verification fixtures. Only these changed areas are run here.

## Remaining work

Individual sync-message generation and admission, transport, subnet aggregation,
scheduler wiring, branch switches during collection and restart between partial
submissions remain open. The primitive's dedicated negative tests do not include
a new encoded non-subgroup fixture in this tranche. No production-readiness or
complete sync-network claim follows from this bounded evidence.

## Result

Passed in 36.01 s combined: native merging 35.66 s, cryptographic runner 0.34 s.
The native loop produced 40 blocks (20 full aggregates assembled from halves),
272 attestation votes and finalized epoch 3, with two database reopens. The
cryptographic runner retained its 12 official cases (three valid, nine invalid)
and 33 existing mutations, plus the new signature-sum assertions described above.
Source/binary hashes were verified unchanged after execution. Logs normalize
trailing whitespace only. Existing compiler deprecation warnings remain.
