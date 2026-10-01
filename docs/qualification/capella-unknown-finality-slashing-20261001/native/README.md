# Native missing-member finality advancement — passed

Base checkout 25e38e9; exact test delta recorded in source.patch. Runtime src/include/network/third_party unchanged from qualified 74784c4. New CTest consensus_capella_unknown_finality_slashing passes in 176.14 s. No new runtime image, consensus rule, threshold or time limit.

Four epochs of real duties are signed with existing test journals, held, and admitted at the next boundary as previous-epoch votes. No injected finality/checkpoint. At slot 96 the new validator proof is admitted with justified 10 / finalized 8 (member absent from finalized registry). At slot 104 finalized advances to 9 while member remains absent, proof archive remains exact, reverse replay does not rewrite checkpoint. Both execution owners are reopened before and after this advancement; committed checkpoint, head state, archive and absent registry member are verified. Final sanction later retires evidence. 16 forged-proof controls pass; one actual unknown-member finality advance and 64 missing-member observations.

The earlier two-epoch fixture failed its coverage criterion and is preserved in ../native-first-attempt. Docker qualification still pending. This is two durable execution owners with synthetic keys; no network transport, physical crash, production launch or all-validator lifecycle qualification.
