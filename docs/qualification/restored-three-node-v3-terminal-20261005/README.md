# Restored three-node terminal campaign

Terminal PASSED on controller revision 57761d82, core 2795d4b5 and relay 8adbabff. Mac native, Docker ARM64 and Docker AMD64/VPS used three distinct peers on two physical hosts, with 16/16/33 disjoint synthetic signer sets. Private cpp-libp2p transport used SSH forwarding; no public launch or real funds.

The common finalized checkpoint advanced once from epoch 75 to 1284 (root ccF88d18 as recorded in result.json), with three matching samples of epoch 1284. This is not three distinct common epoch advances. Each node actually proposed and attested; all 65 active signer sequences and digests advanced. Three external stop receipts and clean exit 0/no-OOM Docker states are retained. Program and controller hashes were independently checked again after terminal completion. Docker environment credential values are redacted.

45 proposals, 354 attestations and 1,562 sync messages were admitted across the three owners. VPS max poll was 5,699 ms against a 6-second slot; no poll exceeded a slot in this bounded run. This is not sustained resource/load qualification. Canonical signed body ancestry to the baseline was checked by the controller.

Post-stop readonly database/witness audit, multiple distinct advances over a longer window, prolonged/asymmetric partitions, rotations and full validator lifecycle remain open. Physical ARM64-to-AMD64 storage transfer is still pending dedicated authorization. Previous failed campaigns remain failed. This result does not qualify production, all forks, or historical standalone votes/fork choice.

Post-stop database/witness audits are now PASSED for all three datasets, with exact external stop commitments and unchanged before/after file hashes. See post-stop-mac and post-stop-vps; the Mac Docker dataset was audited with the native ARM64 tool, and the VPS dataset with its AMD64 Docker tool. Other open gates above remain unchanged.
