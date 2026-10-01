# Connected majority island S — liveness passes, recovery fails

Runtime 418aceb8, harness 3e210dc. Terminal exit 1. Existing packaged ARM64
and amd64 candidates, 3 processes on 2 physical hosts, 64 public synthetic
validators (22/21/21 keys), Capella/Shanghai, slots 6 s, epochs 8 slots.
Effective stake bound from hash-matched initial SSZ: Mac island
1,376,000,000,000 / 2,048,000,000,000 gwei (>2/3); VPS 672,000,000,000.
No source change or runtime build during the campaign.

Requested 384-second isolated-VPS cut passes its majority safety/liveness
gate: Mac peers remain mutually connected, VPS zero peers, new common
majority finality 6 then 7 exceeds conservative pre-cut ceiling 5. Minority
stays at finalized epoch 2. Original dedicated-network IP restored. This
qualifies this sustained cut's connected-majority progression only; R's
failed 192-second criterion remains retained.

Strict recovery fails: observed complete read at 182905.931291 ms exceeds
the unchanged 180000 ms budget, unsuccessful read cost 2906.347416 ms.
This is deadline accounting, not a recovery duration. Another bounded
180-second diagnostic window also fails. Last observed majority finality
15 vs VPS 2; distinct heads, connected peers. Three-advance common-finality
stability not executed. Signers healthy (414/397/399 local attestations),
clock bounds retained, containers stopped exit 0 without OOM. No finalized
conflict or regression observed; no unlimited safety claim.

Both stopped canonical histories replay against independent exact stop
checkpoints: native 108 blocks including anchor / 314134 bytes / one tx;
VPS 79 / 199854 bytes / one tx. Neither replays historical fork choice or
standalone votes. Offline public root/payload comparison finds all 107
native canonical signed archives, of which only 31 have identical payload
hashes on the VPS (slots 3–33); 76 later canonical envelopes missing.
Pruned current-store absence alone was NOT treated as absence of receipt.
The VPS stopped database was copied only after verified exit, separately
from keys/journals/witnesses; not included in this archive. Its own canonical
replay and external digest validate the copy used for analysis.

A confirmed serving gap: at stop the native core retains anchor slot 120
and newer state, but refuses export of 85 authentic signed canonical
archives at slots 3–119. Anchor 120 itself is known, available and signed;
the current dependency walker nevertheless stops at any local anchor
before delivering that anchor. The VPS does not know this newer anchor.
Thus recent descendant delivery lacks the receiver's required parent path.
Source snapshots and exact export metadata retained. This diagnoses a
concrete pruning/synchronization gap; it is not a controlled proof that no
other relay issue contributed. Fix not implemented or qualified here.
Do not solve it with trusted checkpoint jumps, weaker admission or a
longer recovery verdict. Next: authenticated bounded historical ancestry
export/delivery plus unknown-anchor regression and then this WAN scenario.

Read-only C++ probe uses existing core libraries and verifies individual
BLS votes only for epochs 4–7; full canonical replay is a separate audit.
Its bounded root inventory/export probes carry no private state mutations.
49 relevant harness tests passed before campaign; source snapshots, logs,
resources, public fixtures and exact receipts retained. Native RSS excludes
P2P child, Docker BlockIO is cumulative I/O, sampled resources are not a
capacity certificate. No runtime image compilation, funds, public launch,
load flood, new EIP or production-readiness claim.
