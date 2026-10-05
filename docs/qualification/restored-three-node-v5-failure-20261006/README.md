# Restored network v5: terminal failure, not finality qualification

Candidate C++ b5ee00ac, relay 8adbabff, clean controller checkout b8567771.
Original session 40470 exited 1. Started 2026-10-05T22:02:41.690Z;
terminal at 22:03:44.659Z, prematurely stopped on HTTP 503 from
VPS `/eth/v1/beacon/headers/head`. The old controller discarded its response
body: the precise refusal cause is unknown and must not be inferred.
Zero distinct common finalized advances; baseline remains epoch 1284.

All three stop receipts report healthy validator and clock loops, with
actual proposals 4/1/5, attestations 20/21/32 and sync messages 63/60/133.
VPS maximum poll 8039 ms and proposal 7513 ms exceed the six-second slot;
resource/timing qualification remains open. This short run cannot establish
that the previous deadline defect is resolved under sustained WAN operation.
Native and Docker processes stopped cleanly; Docker environment signing
values are redacted in the archived inspect records.

Controller follow-up records HTTP error bodies and skips only the three known
Beacon read capacity/queue/execution-budget refusals. It never extends the
900-second deadline or treats an incomplete sample as shared finality.
Unknown failures remain fatal. Five new tests also verify that a refused sample
breaks consecutive stable observations. No C++ or image rebuild for this fix.
Before another run, audit each stopped store against its own v5 terminal
commitments; old v4 pins must not be reused for signing.

Completed independent stopped-store audits with the b5ee00ac candidate
read-only capture tools: native 490 canonical blocks, VPS 491 blocks;
all three core terminal commitments and 16/16/33 owned signer commitments
match exactly. All file hashes remain unchanged (VPS 1139 files).
The two Mac datasets retain separate complete 32-journal inventories; inactive
owner pins come from each original capsule, without merging signing journals.
Canonical heads diverge; finalized ancestor epoch 1284 is verified separately.
Historical standalone votes/fork choice are not replayed; production remains open.
No database was transferred between hosts. The physical ARM→AMD portability
export still awaits its separate permission.

The observation fix also walks cached ancestry parents to the trusted baseline
after an interrupted read. A partially filled cache cannot certify ancestry.
Conflicting bodies and cycles remain fatal and tested.
