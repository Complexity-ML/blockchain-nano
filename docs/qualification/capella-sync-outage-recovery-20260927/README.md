# Capella remote sync recovery after transport outage

Status: PASS (exit 0). Build succeeded; pre/post-run source and binary hashes match.
Base revision: `573acba115f9e5a59afa78b8d8ccbc6fd2d4f687`, plus `source.patch`.

The new `--sync-transport-outage-cli` mode combines the existing bounded transport
outage with independent verification of remote sync signatures in recovered
finalized blocks. Two local peers hold disjoint groups of 32 validator keys. Once
heads agree past slot 10, only both child P2P daemons are suspended with SIGSTOP
for 96 seconds. Core schedulers, clocks and storage remain active. The harness
checks that neither isolated half advances finality past the pre-outage epoch
bound and that both keep proposing onto different branches. SIGCONT resumes both.
This is an outage of transport processes, NOT packet loss between live daemons.

The existing 240-second recovery deadline is unchanged. Success requires common
finalized checkpoints/headers for three observations and at least three sampled
finalized blocks containing independently verified sync signatures from keys
owned by the other node. A new inclusion-slot cutoff excludes all messages signed
at or before the highest observed head slot at reconnection. This is a head-slot
cutoff, not a timestamp proof that each signature was generated after SIGCONT.
Old pre-outage aggregates
cannot satisfy the added recovery gate. Signed blocks must have been observed
identically by both peers and be ancestors of the common finalized root. The
initial authenticated committee is used; other periods are explicitly excluded.

No production acceptance rule, rate limit, slot duration or deadline is relaxed.
No transaction flood, real funds, VPS changes, public network or physical crash.
The test does not qualify asymmetric delay, committee rotation during a partition,
independent hosts, malicious votes, disk failure or large validator populations.

Reproduce:
`build-release/nano-capella-transition-tests --sync-transport-outage-cli build-release/nano build-network/nano-p2p`

## Observed result

Both nodes stopped cleanly with healthy clocks/validators and finalized epoch 4,
root `0x5f8a12fbb583e7437754bac4c8c989195bc1c3a0f4c69476080fe09ad42dec57`.
Recovery required epoch >=3, three stable observations and >=3 remote samples.
The inclusion cutoff was 27. Independently checked samples:

| Inclusion slot | Participating committee positions / 32 | Remote positions |
| --- | --- | --- |
| 28 | 15 | 0 |
| 29 | 16 | 1 |
| 30 | 31 | 14 |
| 31 | 31 | 16 |
| 32 | 30 | 15 |

Four samples qualify as remote. Slot 28 does not. Recovery is progressive, not
instant full participation. Receiver sync outcomes: 857 accepted, 29 refused;
no observed budget refusal, local core transport failure or ACK write failure.
Core sync rejection counters total 19 root mismatches and 10 already reached
inclusion slots. These early refusals do not establish signature validity.
Sender reports one unconfirmed sync delivery, plus one attestation admission
refusal and one block catalog admission refusal. All are retained in the log.
Core accepted counts include local signing/duplicates, not unique network traffic.

Artifacts: `run.log`, `build.log`, `summary.json`, `provenance.json`, `source.patch`.
This is bounded integration evidence, not production approval or complete EIP
qualification. Next gates include asymmetric transport, partition across rotation,
crash recovery under load and independent-host/large-population qualification.
