# Native finalized ancestry audit candidate

Runtime candidate 736dab46, parent 3030b799, published separately. Main runtime
is not promoted. Optional `requiredFinalized` adds a seventh canonical-audit
request field: epoch must be encoded as a Nano hex quantity, root canonical
32-byte hex. Legacy six-field requests remain accepted and explicitly report
externalFinalizedAncestorVerified false. The new check traverses the authenticated
replayed finalized branch to the last block at or before the requested epoch
boundary, including empty slots. Future epochs, incorrect roots, missing parents,
cycles, overflow/budgets and cancellation fail closed. Historical standalone
votes and historical fork choice remain outside this audit's scope.

ARM64 Docker direct compilation: 12 actual graph-helper cases pass, not signature
qualification. Native integration on a stopped restored database replays 99
blocks and 97 transactions: valid final checkpoint passes; unfinalized head,
future epoch and malformed root refuse. Legacy compatibility also passes.
Complete fixture hashes and driver hashes before/after integration match.
The requested checkpoint for this synthetic integration was selected from the
legacy audit output, not independently captured WAN evidence.

First compiler attempt refused a read-only temporary directory before compiling;
TMPDIR was corrected to the executable tmpfs. First integration's valid case
and branch/future negatives pass, but its malformed root fails through unhex
with an unexpected `odd hex length` diagnostic, causing the controller to fail.
Evidence is preserved under attempt-a. The length check now precedes decoding;
only the modified translation unit/driver was rebuilt, then all four integration
cases pass. The unchanged helper was not rerun.

Direct O2 objects link existing frozen O3 libraries readonly; this is not a full
CMake rebuild. See provenance for manifest coverage and timing. AMD64, full
regression and binding the independently observed last checkpoint in the live
backup/restore runner remain pending. Existing 3030b799 campaigns are untouched.
No WAN, power-loss, independent security review or production closure.
