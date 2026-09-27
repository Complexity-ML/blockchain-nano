# Capella binary serve startup — 2026-09-27

Base revision `ae2d23b` plus accompanying changes.

`nano serve genesis.json database 8545 127.0.0.1 service.json` accepts a `capella`
object containing the runtime configuration previously qualified. The private
NANO_NETWORK_TOKEN environment variable must contain canonical 32-byte hex.
The CLI rejects a public bind, dev_mode and competing consensus configurations.
It starts Capella before Rpc, attaches its Beacon read handler to HttpServer,
then drains HTTP before stopping the runtime and printing its restart receipt.
The private token is not logged. Ordinary serve behavior remains unchanged.

New consensus_capella_cli_serve test launches the real nano binary against
JSON-derived Shanghai genesis and an authenticated synthetic Capella snapshot
aligned to wall time. Two starts with a clean stop between them exercise witness
recovery, HTTP status and empty anchor catalog. Missing-token/public-bind attempts
are refused. The test passed in 1.43 s. Existing rpc_cli regression passed in
0.89 s, covering ordinary and development serve and native client operations.

This CLI scenario has no validator keys and produces no additional blocks; it
qualifies startup/read/recovery, while configured-validator production remains
covered by its separate runtime test. `run` managed P2P dispatch remains Bellatrix;
Capella peer sync, dependency ordering and libp2p protocol activation are still
open. No public activation, external server or funds.
