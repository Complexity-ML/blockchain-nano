# Restored network v7: original terminal evidence

Controller session 62154, checkout 9bb88804, core b5ee00ac, P2P 81e5ab31. Original exit 1; result FAILED. No controller restart.
Failure: Three-node common finality did not advance through three distinct checkpoints before deadline

Original fixed 900-second window and three-distinct-checkpoint requirement retained. Live alerts are not substitutes for original terminal receipts.
- mac-native: validator healthy=True; failure=None; proposals=33; attestations=296; sync=1384; core restart={'digest': '0x506f1a1ba975dce082639165893c791ce1347ff1701842abe202b85b7e7a0e90', 'sequence': '0x1ccb'}.
- mac-docker: validator healthy=True; failure=None; proposals=29; attestations=302; sync=1176; core restart={'digest': '0x2679984f0cc8fd8a28d826fb3892c617da3ffbe8b0604a85d1e395e601f07ea4', 'sequence': '0x1c11'}.
- vps-docker: validator healthy=False; failure=invalid Capella Store snapshot at line 39; proposals=28; attestations=161; sync=49; core restart={'digest': '0x7f4d9e3fb24751b86883a3351adf1502e1c4fd8e691f158a920883087b550f35', 'sequence': '0x1c75'}.

Owned signer pins advanced: 16/16 native, 16/16 Mac Docker, 32/33 VPS. The registration-key pin remained unchanged; do not claim all 65 resumed duties.
Independent stopped read-only audits matched exact core and owned signer commitments. Full Mac journal inventories remain separate; no journal reset or merge. Source/config copies are from each owner capsule. Canonical execution history and required finalized ancestor verified; historical standalone votes and fork choice not replayed. No database transferred across hosts.

Diagnostic live samples and refused-while-active audit attempts are preserved. Docker signing environment values redacted. Network recovery and production criteria remain open; this bundle is not production qualification.
