# Bounded dependency route — native draft, not integrated

Proposed relay-only change: preserve at most dependencyBlocks authenticated local root identifiers after an admission quantum yield. Next turn exports/decodes and sends the next bounded segment instead of walking the full unadmitted suffix. Every block still requires remote full admission; local metadata is never an ACK. No payload budget, deadline, consensus rule or quantum enlargement. Session metadata is reset with existing peer-session reset handling.

Original fixed model5942/0(fe8793):513/935/2049 cases retain exactly the parent export counts; no improvement claimed for that cache-stable metric. Native compilation occurred concurrently with V36; no resource isolation claim.

Original churn model63976/0(0d2ee9), same synthetic ancestry/quantum8/ACK1024/dependency256/cache256/16MiB:

| Blocks | Parent exports | Draft exports | Turns, unchanged |
| --- | --- | --- | --- |
| 513 | 9217 | 1777 | 68 |
| 2049 | 43009 | 13249 | 293 |

Independent full cache eviction between turns models contention; it is not a measured WAN schedule. Exact admissions and parent-first sends asserted. Draft route roots remain at most256. Head regression26061/0(6e92b9) covers existing head validation, shared budget, timeouts, cancellation and duplicate callbacks. New adversarial route driver31297/0(9182c1): definitive refusal followed by contradictory accepted callback, cancellation with held ACK followed by a late accepted callback, then full retry. No improper acknowledgement or skipped child permitted.

This is native scoped evidence for a proposed optimization, not deployed code or global cause proof. ARM64/AMD64 affected relay qualification, moving branches/unavailable exports/remote session reset route cases, integration and fresh WAN finality remain open. V36 FAILED remains preserved. No EIP/production/bridge/Doppler qualification.
