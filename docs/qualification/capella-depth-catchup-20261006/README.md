# Bounded dependency depth: fresh-peer catch-up

Old relay 8adbabff: a head needing three ancestor exports with a bound of two
returns false before any send or catalogue opening. Reproduced in the existing
ARM64 Docker image (test exits 1 intentionally; reproduction controller exits 0).
This demonstrates a modeled liveness defect, not the exclusive cause of WAN v6.

The block preparation result now distinguishes a local dependency-depth bound
from all other failures. Only that precise result makes the head relay fall back
to the historical catalogue within the original total deadline. In that bounded
catalogue, over-deep roots count as refusals and remain unacknowledged, but later
entries are scanned: a shallower authentic ancestor can seed confirmed ACKs.
The attempt remains FAILED if any root was deferred. Further attempts may then
admit the head using the accumulated session-specific real ACKs.

New regression covers adverse root ordering (deep entry sorts before shallow
ancestors), two-attempt progress with unchanged depth bound, parent-before-child
sends, no fake head ACK, and stopping on a real remote rejection. Existing
17 head + 45 catalogue + 28 ancestor scenarios pass, alongside these two new
modeled scenarios. Wire ACK, cryptographic admission, overall deadlines,
page/byte/work bounds, and committee/validator limits are unchanged.

Source patch integrated after isolated testing; active v6 still uses its original
8adbabff P2P binaries and b5ee00ac core. No running signer restarted. AMD64,
complete P2P executable builds, actual restored WAN finality, and broader
consensus/production remain unqualified for this patch. The new CTest target
is registered; the captured run used existing target compile/link flags.
