# AMD64 Capella activation timeout, partial full campaign

Source c10426b2, existing AMD64 builder on the authorized OVH VPS. Case #394 consensus_capella_deposit_activation timed out at the original 360-second limit (reported 360.03 s). Exact result and progress lines are retained in observation.json. The last progress reports deposit index 1, slot 120, finalized epoch 13. Continued finality is observed, but the scenario's end checks for pending/active transition and new-validator voting were not reached. This is neither a successful activation qualification nor proof of a protocol stall.

At this observation, 394/417 cases were reported: 389 passed, five failures/timeouts. The four earlier failures remain. Case #395 activation_restart is still running and has not been assigned a result. The queued 19-case corrective campaign does not cover this new failure, nor the Altair and Bellatrix partition failures. No original timeout, slot bound, assertion, runtime source, executable or image has been modified.

The archived ARM64 c104 result for this same activation scenario passed in 136.363 seconds. This comparison motivates investigating execution cost; it does not identify a cause or prove performance on another candidate. The Bellatrix phase profile is separately queued and is not a Capella profile or a fix for this activation timeout. Full AMD64 source/program/result/EEST audits remain pending campaign termination. Candidate promotion and production remain blocked by unclosed qualification requirements.

## Subsequent activation restart timeout

Case #395 consensus_capella_deposit_activation_restart also timed out at 360.03 seconds with its original 360-second limit. restart-observation.json retains the complete output. The scheduled-pending reopening phase was observed for owner 0 at slot 104, then finality advanced through epoch 13 at slot 120. Complete active restart/signing and conflict-rejection end assertions were not demonstrated. This bounded reopening observation does not close the full restart requirement. The corresponding archived ARM64 c104 case passed in 139.204 seconds; cause of the AMD64 timeout remains unidentified.

By the subsequent observation, case #396 automatic_registration passed (29.71 seconds): 396/417 reported, 390 passed and six failures/timeouts. The original observation.json remains unchanged. Case #397 deposit_auto_registration is still running, not assigned a result. The queued 19-case correction and Bellatrix-only phase profile do not qualify either new Capella activation failure. Original criteria and candidate/production gates remain open.

## Deposit plus automatic registration timeout

Case #397 consensus_capella_deposit_auto_registration timed out at 360.03 seconds under its original 360-second limit. auto-registration-observation.json retains the output; deposit index 1 and finalized epoch 13 are observed at slot 120, but completion and active-new-validator end assertions are not demonstrated. The registration-only #396 success does not qualify this combined scenario. The archived ARM64 c104 counterpart passed in 137.396 seconds. Cause of the AMD64 timeout remains unidentified; the common progress cutoff of these three cases is an observation, not a demonstrated common defect.

At this observation, 397/417 cases are reported: 390 passed and seven failures/timeouts. The earlier two observations remain intact. No timeout, end condition, runtime source, executable or image has changed. None of these three Capella activation-family failures is covered by the queued 19-case correction or by the separately queued Bellatrix-only phase profile. Full AMD64 audits and corrective work remain pending; production is not qualified.
