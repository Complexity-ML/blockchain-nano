# Common bridge test batch

The runner compiles the synthetic harness with solc 0.8.28, Paris, optimizer 200,
using the repository's OpenZeppelin 5.0.2 sources. It executes the same scenario
against two isolated Anvil chains and two independent Nano `State` instances.
The native helper signs only public fixture keys 1, 2 and 3 and calls the actual
`ExecutionEngine`; it starts no consensus, database, RPC server or P2P network.

Dependencies are the existing `tests/hardhat` packages: ethers 6.16.0 and solc
0.8.28. No package installation is performed by the runner. Required absolute
environment paths:

```
BRIDGE_SOURCE=<repository root>
BRIDGE_DEPENDENCIES=<repository root>/tests/hardhat/node_modules
BRIDGE_OZ=<repository root>/tests/contracts/openzeppelin-v5.0.2/contracts
BRIDGE_OUTPUT=<directory reserved for this run>
BRIDGE_ANVIL=<Anvil executable>
BRIDGE_NATIVE=<nano-bridge-execution-runner executable>
```

Run `node tests/hardhat/bridge/runner.mjs`. Both backends run by default;
`BRIDGE_MODE=anvil-only` or `native-only` selects one explicitly. The output
directory must exist and its named artifacts must not already exist. Transactions
have a fixed 30M gas ceiling; negative cases require actual unsuccessful receipts
and no surviving logs. RPC calls, helper requests, startup and child shutdowns
are bounded. Every child belongs to this invocation. Native completion requires
a clean EOF and exit 0. No real-chain endpoint or signer is accepted.

The native reload is an in-memory JSON roundtrip with full state and root equality.
Anvil's corresponding step only mines a block and is labelled as such. Neither
operation proves RocksDB crash recovery. Event-history reconstruction uses actual
backend logs; the native index itself is memory-only.

The mock verifier's approved digest is synthetic authorization, never a proof of
source finality. These tests cannot qualify an authenticated cross-chain bridge,
Circle's Bridged USDC Standard, a production route, native-token conversion or the
validator network. `contracts/bridge` deliberately supplies only abstract endpoint
contracts; mock gates and token failure controls live under the test namespace.

Build the opt-in native helper with `cmake --build <build-dir> --target nano-bridge-execution-runner` after configuring the ordinary Nano C++ dependencies. `npm run test:bridge` in `tests/hardhat` is an alias for the runner. No CTest registration or default node startup is added by this target.

The [recorded qualification](../../../docs/qualification/bridge-common-20261016/README.md) preserves both successful execution results and the original controller failures. This batch does not establish continuous CI coverage; account billing and remaining CI criteria are still open.
