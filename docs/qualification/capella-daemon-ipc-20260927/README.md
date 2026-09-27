# Capella IPC with the real libp2p daemon — 2026-09-27

Base revision `5c8ac30` plus accompanying changes.

The daemon now recognizes exactly one Altair/Bellatrix/Capella IPC profile,
routes shared recovery/dispatch calls to the matching methods, requires execution
binding for Bellatrix and Capella, and preserves the full Capella identity.
Capella uses the execution-consensus guards against the old height-only import,
legacy block announcements and BFT block protocols. The old archive relay cannot
run in Capella mode. Capella admission remains explicitly disabled: an enabled
flag is refused until catalog/archive relay integration is ready.

Commands:
```
cmake --build build-network --target nano-p2p -j4
cmake --build build-release --target nano-capella-transition-tests -j4
build-release/nano-capella-transition-tests --daemon-cli build-release/nano build-network/nano-p2p
ctest --test-dir build-network -R '^altair_core_(dispatch|recovery)$' --output-on-failure
ctest --test-dir build-release -R '^consensus_capella_managed_node$' --output-on-failure
```

All commands succeeded. The integration test starts the real nano supervisor and
nano-p2p twice, listening only on 127.0.0.1 with no bootstrap peers. Both runs must
emit IPC-ready with profile capella and networkProtocolActive=false, then shut
down normally with Capella receipts. The second run uses persistent witnesses,
without an initial snapshot. Logs show the same peer identity and consensus
identity across restart. Shared recovery/dispatch tests and the fixture-child
failure/key-isolation test also passed.

Scope: real process IPC and restart. No peer-to-peer Capella block exchange,
partition recovery, advancing finality or public deployment is claimed. The
legacy protocol guards are changed in source but adversarial wire probes are not
part of this test. Catalog relay, bounded parent fetch over peers and activation
criteria remain open. One synthetic local validator key is loaded; this short
scenario produces no blocks or attestations.
