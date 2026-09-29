# Slashing proof across new validator registration — 2026-09-30

New `consensus_capella_new_validator_slashing` test; native Mac PASS in 162.25 s. Parent revision `e365fbac01e230539ac93c19fc5043473f210057`. Runtime sources unchanged from `940736fd2078eb058289434cfefa2c2147df690d`. Linux ARM64 PASS in 151.55 s; VPS x86-64 PASS in 532.84 s. Both containers exited 0 without OOM. The unchanged deposit-activation path passes its native regression in 160.07 s. Test revision: `fd8d02e6aae6808b59bc686ddf6cb9cf2138af30`.

## Actual sequence

The existing native deposit fixture submits a signed execution transaction to the deposit contract. Deposit consumption occurs at slot 80. Before the justified registry knows index 64, signed double-vote proof admission must return false without changing the checkpoint or archive. At slot 88, justified epoch 10 knows the new validator while finalized epoch 9 does not. The public synthetic key signs contradictory attestations; the proof is admitted by both durable owners.

Both owners are closed and reopened at that exact missing-finalized-member boundary. Their checkpoint tokens, complete Beacon state and exact archived proof must remain unchanged; the finalized registry must still lack index 64. Reverse-proof replay remains idempotent. Across 32 observations with missing finalized membership, the proof is retained.

The normal native lifecycle continues: activation epoch 17, automatic proof inclusion, and retirement only when the validator is finalized slashed. The chain reaches finalized epoch 18 at slot 161. One selected offender proposal is skipped rather than reassigned; index 64 deliberately does not attest once known equivocal. Both owners reopen again after the sequence and retained/retired proof state is rechecked.

## Limits

`unknownFinalityAdvances` is **0**: the finalized root does not advance while membership is absent after proof admission in this run. This proves admission and durable retention across that registry gap, not execution of the missing-member cleanup branch during a different finalized-root advancement. That exact branch-coverage gap remains explicit. No WAN transport or physical power-loss test is implied. The earlier WAL campaign and reorg campaign remain separate evidence. No additional EVM fork or overall production readiness is claimed.

Archived logs were checked for matching admission/registration epochs, 32 missing-finalized-member observations, exact scope limits, activation epoch 17 and final epoch 18. See `verified-results.json` and artifact hashes in `manifest.json`. Existing images/volumes were reused.
