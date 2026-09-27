# Capella coherent archive export — 2026-09-27

Base revision `6bacfcf` plus accompanying changes.

`export_message` projects archive bytes, network identity and checkpoint under the
reactor mutex. Optional expected checkpoint fences stale requests. Blocks reuse
the archive authentication path (header/body/root/signature); retained votes and
slashing proofs are exported from admitted durable state with digest verification.
Unknown blocks and the unsigned trusted anchor do not fabricate wire payloads.
The caller bounds raw SSZ bytes and must additionally budget hexadecimal JSON
framing. Cancellation leaves consensus untouched.

The extended wire test passed in 5.38 seconds: exact export and envelope decode
for nine signed blocks, six votes and one slashing proof, after two reorgs.
Additional assertions cover unsigned anchor, unknown root, stale checkpoint,
one-byte export budget and cancellation. No repeated unchanged runtime suite.
A declaration comment documenting framing limits was added during compilation;
it has no executable effect.

Scope: C++ export primitive only, not a network transfer or full synchronization.
Inventory pagination, private authenticated RPC dispatch, peer resource budgets,
libp2p wiring and mixed-fork migration remain open. No production activation.
