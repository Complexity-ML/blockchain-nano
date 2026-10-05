# Restored network v6: full-window terminal failure

Original controller session 91002, checkout 1a51dfad, runtime core b5ee00ac,
P2P 8adbabff. Genuine network readiness reached after 4434 ms. Three owners
16/16/33 actually signed and admitted proposals/attestations/sync messages.
The fixed 900-second observation window expired: zero distinct shared advances
past finalized epoch 1284. Exit 1; all three node processes exited cleanly 0,
without OOM. Stop receipts show all validator/clock loops healthy, unlike v4.
VPS nevertheless recorded 15 polls longer than the six-second slot (max 6697 ms).
No HTTP 503 was observed in v6; this does not retrospectively diagnose v5's 503.

Independent post-stop read-only audits reproduce exact core and owned signer
commitments; files unchanged, including 1260 on VPS. Each Mac retains its own
full 32-journal inventory, with inactive owner pins from its own initial capsule.
The shared finalized ancestor is verified separately; canonical heads diverge.
Historical standalone votes and fork choice are not replayed. No signing journal
is merged or reset, no database crosses hosts, no production qualification.

The one sequential cross-head probe returned own-root 200 and six foreign-root
404 responses at its sampling time. Later observations show some pairwise shared
heads. These observations are insufficient for an exclusive transport cause.
The local deep-head catalogue starvation defect is independently reproduced and
patched in 81e5ab31; v6 uses its original executables throughout, so it does not
qualify that patch. New P2P compilation began only after actual node stops.
Docker inspect signing environment values are redacted in committed evidence.
