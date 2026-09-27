# Distributed Capella sync rotation — 2026-09-27

Status: bounded test passed (exit 0); production readiness is not established.

Command:
`build-release/nano-capella-transition-tests --sync-rotation-distributed-cli build-release/nano build-network/nano-p2p`

Two local native nodes, separate stores and disjoint sets of 32 validator keys,
real libp2p, unchanged minimal profile and six-second slots. Unlike the short
sync test, this waits for common finalized epoch 17 (three stable observations),
after the nontrivial committee transition at slot 128. Its bounded finality wait
is 1080 seconds; startup/shutdown and inspection are additional bounded operations.
No transaction flood, real funds, public nodes or VPS changes.

Signed head blocks are captured from both nodes before pruning. At completion,
both nodes stop cleanly; their stores are reopened using their recovery witnesses.
The state at the agreed finalized root must exist on both and be identical, with
its SSZ commitment matching the block state root. That state supplies the committee
for final aggregate verification, avoiding an unauthenticated RPC committee list.
Its committee must differ from the initial committee, and at least three sampled
finalized ancestors must have independently verified contributions from keys owned
by the other node. Blocks from other committee periods are excluded explicitly.

This remains one physical host and no adversarial network. It does not qualify
WAN, asymmetry, partitions across rotation, or large validator populations.

Source and binary hashes were captured before launch and checked after completion.
Full output is in run.log; summary.json contains exact counters, roots and samples.
Only trailing whitespace was normalized. source.patch applies to the recorded base.


## Observed result and remaining limits

Both nodes stop cleanly with healthy clocks and validators, common finalized epoch
17 observed three times, root
`0x462d1c18777a32499f3a10985fc4f05973d0803fe665aa3654bad79e212b470a`.
Both witness-backed stores reopen and agree on the committed state at slot 136.
Its committee hash differs from the initial committee (exact hashes in summary).
Eight finalized samples at slots 129–136 verify BLS using this authenticated new
committee, with 12–19 remote positions relative to their proposer.
Participation in descending slot order is **[32,31,30,32,30,32,32,28] / 32**.
Full participation is NOT established for every sample or block. The success gate
requires at least three finalized samples with independently verified remote
contributions; the observed eight samples exceed that gate, not a stronger 100% one.

Receiver: 4770 accepted and 80 rejected sync ACK writes, zero budget rejections,
zero local-core errors or failed ACK writes observed. Core rejection totals match
those 80 sync refusals: 65 root mismatches and 15 head-at-or-after-inclusion. No
membership or signature-check failure observed; earlier rejections do not validate
signatures. Sender: 2 unconfirmed sync deliveries and 9 unavailable sync exports
remain tracked. One attestation admission was rejected; two block catalog attempts
report rejection. Individual causality of these remaining outcomes is not established.

This extends native rotation evidence to two live libp2p peers with disjoint signing
keys, independent persistent stores, pruning and finalized-state recovery. It does
not qualify asymmetric latency, network partitions at the transition, independent
hosts, crash/power-loss recovery, or scale. Those gates and incomplete participation
remain open. No production closure or public deployment.
