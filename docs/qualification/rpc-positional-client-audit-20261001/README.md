# Positional JSON-RPC argument defect: client and test harness

Live complete-regression campaign at the prior runtime reports `events`,
`validator`, `network`, `signed_network`, `bft_network` failures with -32602
Expected positional parameters. Other local BFT scenarios still pass.
The full campaign remains running; this snapshot is not terminal evidence.

`RpcClient::call(method, Json params)` permits a braced single JSON object to
select JSON copy construction instead of building an array. The production
legacy validator path in src/client.cpp calls validator_submitBlock with
`{block}`; the same defect therefore affects this client path, not just tests.
HTTP server validation correctly rejects the object. Do not normalize arbitrary
invalid JSON objects into accepted params or weaken the positional-array rule.

The prepared patch adds initializer-list overloads which explicitly construct
arrays while preserving the existing explicit-Json path. The local helper,
persistent test clients and events builder are also covered. An added validator
negative test sends explicit object params and requires -32602 with unchanged
state/head/pending/events; valid list submission already has positive checks.

Patch is checked with git apply --check but not applied/compiled, preserving
source freeze for the live complete regression. Apply only after its terminal
state, inspect other collected failures for the same cause, qualify actual
positive legacy CLI and HTTP/network paths, then qualify the changed candidate.
This prospective change affects runtime client code, so existing unchanged
binary hashes cannot qualify the patched candidate. Remaining production
criteria, optional Linux registrations, WAN/lifecycle and review stay open.
