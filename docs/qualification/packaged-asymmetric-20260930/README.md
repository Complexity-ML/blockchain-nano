# Packaged asymmetric WAN qualification

The exact final runtime images are used without a build-volume mount. VPS
netem changes only the labelled isolated container network namespace, with
500 ms egress delay and no artificial loss. Under delay the common finalized
checkpoint advances 2 to 4. Netem counters and verified removal are in run.jsonl.

Immutable state selectors avoid the historical head read race. Common jumps
4 to 6 and 6 to 8 reset the consecutive window without credits, safety-history
reset or deadline extension. The accepted sequence is 8 to 9 to 10 to 11;
all execution hash bindings and individual head/epoch checks are preserved.
All three terminal signers are healthy; two Docker exits are zero without OOM.

This is one bounded asymmetric fault cycle, not prolonged repeated faults,
packet-loss coverage, production operational qualification or a resolution of
unexplained WAN A. The missing finalized member proof-retention branch during
a new finalized-root change remains uncovered, with its earlier zero counter
preserved. No smart-contract/EIP or newer fork conformance is inferred here.
