# Capella managed node lifecycle — 2026-09-27

Base revision `ba54a09` plus accompanying changes.

`nano run genesis.json database node.json` now accepts a Capella runtime section,
selects its private RPC and Beacon read handler, and stops its workers before
emitting a restart receipt. Public RPC and mixed consensus configurations are
rejected. Configured Capella validator key environment variables are excluded
from the spawned child, like the existing Bellatrix keys. The supervisor generates
its own private IPC token.

The new consensus_capella_managed_node test runs the real nano executable with
an explicit p2p_binary override pointing to a test process. The child authenticates
against the real loopback HTTP server, checks the Capella profile and execution
binding, verifies private key exclusion and supervisor credentials, then waits.
Two starts stop normally; the third child exits with code 7 and the supervisor
must stop the runtime, emit its receipt and exit with failure. Restarts use the
persistent chain and signing-journal witnesses with initial snapshot disabled.
Public bind and conflicting Bellatrix configuration are negative controls.

Results:
- consensus_capella_managed_node: passed, 1.45 s.
- consensus_capella_cli_serve: passed, 0.85 s.
- rpc_cli: passed, 1.30 s.

This is a process-supervision qualification, not a libp2p network test. The child
is deliberately a control fixture. It checks that networkAdmissionEnabled remains
false. The current nano-p2p daemon still needs Capella profile selection, private
IPC recovery integration and catalog/archive relay; this commit is not a usable
Capella distributed deployment. No public network or funds were used.
