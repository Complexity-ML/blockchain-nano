# Historical Capella export — native scope

Base revision: 443db5c; uncommitted source patch archived here.

New historical-export CTest passes (40.43 s): 22 canonical archived blocks after pruning/reopen, five corrupt/incomplete/noncanonical controls, authenticated historical duplicate ACK without checkpoint mutation, differing signature rejection, cancellation and per-request proof budget with authenticated-prefix resumption. Existing signed-transition regression passes (12.54 s). Network block-delivery and head-catalog models pass. These are native tests, not a WAN recovery pass.

Prior failures preserved: an incorrect network build target was corrected; the first depth-budget test incorrectly expected permanent rejection on a second request, despite safe authenticated prefix caching. The corrected test requires first-request budget refusal and second-request bounded resumption; corruption/signature/canonical-ancestry controls remain enforced.

Docker ARM64 qualification is running. AMD64, real daemon catch-up and the 384-second connected-majority WAN recovery gate remain unqualified. No production claim or source marker advancement.

Read-only probe of actual stopped WAN S native database, matched to its external checkpoint: 85 formerly refused archived canonical blocks now exportable. This is serving evidence on real failure data, not a network catch-up or finality recovery pass.
