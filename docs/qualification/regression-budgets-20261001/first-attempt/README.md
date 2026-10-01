# Initial explicit-budget verification — retained failure

Existing ARM64 Docker builder/volume reused, network none, 2 CPU/3 GiB,
read-only source/root, no capabilities, no-new-privileges, tmpfs 128 MiB.
Twelve affected core executable targets rebuilt/checked, existing bootstrap
executable reused. Exact 15-test selection checked for positive timeouts and
absence of skip properties. No runtime source change.

CTest exit 8: 14 pass, `rpc` fails after 0.02 seconds with JSON-RPC -32602,
"Expected positional parameters". Total 3.86 seconds. This failure is not a
budget expiration and must not be hidden by disabling the test or relaxing RPC
validation. A method/type diagnostic is being added to locate the call.
The complete C++ regression and all production criteria remain unqualified.
