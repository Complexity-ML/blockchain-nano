# Restored three-node v4: FAILED

Controller f0c4cb03 completed exit 1 after its fixed 900-second observation window. No distinct shared finality advancement beyond epoch 1284 was observed; connected peers and convergent heads did not imply finality. Native and both Docker processes stopped exit 0; Docker reports no OOM. All three external stop receipts are retained.

The VPS validator reported unhealthy with `deposit history contract view failed`: only 2 proposals and 22 attestations, while both Mac owners remained healthy and admitted 304 attestations each. Post-stop readonly audits of both Mac datasets verified exact terminal commitments, witness/journal inventories and canonical history without changing file hashes. Their authenticated state at epoch 1321 contains previous timely-target flags 16/16/0 by owner group. These facts explain the lost observed participation; they do not prove an exclusive cause of every network failure. VPS post-stop readonly audit was initially open and has now completed, as detailed below.

The typed duty-expiry failure inside the deposit contract EVM view is separately reproduced against the old native core and fixed by preserving the original checkpoint exception. See deposit-view-deadline-20261005. Docker and network requalification of that fix remain open. No automatic restart or journal reset occurred. All source data and commitments remain available.

Post-stop VPS readonly audit is now PASSED: 486 canonical blocks, 33 signer/witness pairs, exact stop checkpoint 5549/6ac92944, and all 1029 files unchanged. Existing AMD64 reader core 2795d4b5 was used with no writer, network disabled and readonly mounts; no Mac database transferred. This does not undo the failed runtime campaign.
