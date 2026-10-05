# Stopped VPS v7 snapshot: block retention at configured capacity

After original v7 terminal exit 1 and successful separate canonical/signer audits,
a diagnostic-only Docker AMD64 tool reads the same stopped VPS database and core
witness. Exact stop token 7285/7f4d9e3f matches; all 1393 file hashes unchanged.
The retained snapshot has 256 blocks against configured limit 256, 65 latest
messages against 2048 and no equivocators against 1000. Timely=255 and
unrealized=256. This supports block retention exhaustion as the next regression
target for the original line-39 failure; the rejected in-memory candidate was
not captured, so this does not independently prove its full structure.

Core b5ee00ac, runtime and original capture binaries unchanged. A fresh single
capture-derived diagnostic object links to the existing core archive; no image
rebuild. First build exit 1 (copied source relative include unavailable) preserved;
fixed copy uses the explicit existing /work/src include, compile/link/container
exit 0, no OOM. Parent executable/library hashes unchanged. The reader verifies
core digest/witness only and emits counters; it does not repeat canonical replay
or signer audits, infer runtime health, or qualify recovery/production.

Next work: reproduce reaching the configured retention budget and prevent
expected capacity pressure from disabling the signer; preserve authentic finality,
rollback, storage bounds and journal commitments. Raising the fixture budget
alone does not repair vote propagation or demonstrate advancing common finality.
No node restart, signing, database transfer or public deployment.
