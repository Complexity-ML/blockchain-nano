# Recovery observation input guards

Recovery refuses absent/short roots, negative/noncanonical epochs and peer
counts, and non-finite/non-positive/non-integral deadlines before recording
credit. Complete canonical decimal and hexadecimal RPC quantities remain
accepted. Distinct observers, conflict/regression checks, strict advancement
and the original absolute timeout are unchanged.

19 deterministic checker tests pass, zero skips; 13 new cases and 6 adjacent
existing recovery cases. No runtime source/build changes or new WAN claim.
The independently observed final checkpoint still needs binding to the offline
replayed finalized ancestry when the stopped receipt has advanced further.
The current six-field native audit request does not yet accept that external
checkpoint; its canonical replay alone must not close this additional criterion.
Active candidate 3030b799 campaigns are untouched.
