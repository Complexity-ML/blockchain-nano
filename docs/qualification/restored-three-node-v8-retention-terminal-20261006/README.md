# Restored network v8 retention experiment: original terminal evidence

Controller session 94490, checkout b553d337, core 05649987, P2P 5902c983. Original exit 1; result FAILED. No controller restart.
Failure: Three-node common finality did not advance through three distinct checkpoints before deadline

Original fixed 900-second window and three-distinct-checkpoint requirement retained. Local retained-block budget was explicitly increased from 256 to 1024, unchanged byte limits and consensus rules. This does not close old-boundary pressure recovery, pruning or production. Live alerts are not substitutes for original terminal receipts.
- mac-native: validator healthy=True; failure=None; proposals=37; attestations=297; sync=1430; core restart={'digest': '0x1bb5ebdcd78acde31b7ef977225b47d306fbf61e741499e1c1b044cf6438ec6a', 'sequence': '0x1f49'}.
- mac-docker: validator healthy=True; failure=None; proposals=35; attestations=298; sync=1188; core restart={'digest': '0x1cccef80d1cb410ef2ba1279eb1d6f56da242cf80f3998324978a40aaef9b650', 'sequence': '0x1e8e'}.
- vps-docker: validator healthy=True; failure=None; proposals=74; attestations=376; sync=122; core restart={'digest': '0x32da76f94d8880ed7c833121153753405d51290379ebd204a86228218913e369', 'sequence': '0x1f5d'}.

Owned signer pins advanced: 16/16 native, 16/16 Mac Docker, 33/33 VPS. All 65 owner pins advanced; this does not prove individual participation in common finality.
Independent stopped read-only audits matched exact core and owned signer commitments. Full Mac journal inventories remain separate; no journal reset or merge. Source/config copies are from each owner capsule. Canonical execution history and required finalized ancestor verified; historical standalone votes and fork choice not replayed. No database transferred across hosts.

Diagnostic live samples are preserved. Docker inspection contains only State, Name and Image; environment omitted. Network recovery and production criteria remain open; this bundle is not production qualification.
