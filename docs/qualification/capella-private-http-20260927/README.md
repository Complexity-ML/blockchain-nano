# Capella private HTTP qualification — 2026-09-27

Base revision `6dbee1d` plus accompanying test changes.

New CTest consensus_capella_private_http (--http) runs the configured runtime
scenario and then sends its Capella RPC probes through a real HttpServer on an
OS-assigned 127.0.0.1 port using a persistent HTTP client. The public 0.0.0.0 bind
is refused before opening a listener. A browser-origin request with the correct
private token is rejected with HTTP 403.

The test passed in 17.24 seconds. HTTP probes cover wrong-token status/jobs/submit/
catalog-open, correct identity, duplicate-block submission and accepted receipt
without checkpoint mutation, exact archive export, catalog open/page/close, closed
handle rejection and malformed root. Runtime coverage still includes sixteen
blocks, witnessed restart, six bad configurations, receipt and shutdown checks.
The output httpTransport=true distinguishes this new socket path;
networkTransport=false still means there is no inter-node P2P transport.

The test ran with loopback networking permission. No public port or external
service was used. This is one local runtime, not distributed synchronization,
HTTP overload/disconnection qualification or a production rollout. CLI dispatch,
parent dependency scheduling and libp2p remain open. Existing unchanged suites
were not repeated.
