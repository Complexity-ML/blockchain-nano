# Nano native Beacon devnet in Kurtosis

This package runs **Nano**, not Geth/Lighthouse: two real Docker services with
32 synthetic BLS validator keys each, using the same Bellatrix/Paris state and
profile. All keys are public fixture keys. Never fund these accounts.

Build the current Docker image and native fixture exporter first:

```sh
cmake --build build-release --target nano-bellatrix-transition-tests --parallel 4
docker build --build-arg JOBS=4 -t blockchain-nano:local .
scripts/run-kurtosis.sh
```

Requires Docker and Kurtosis CLI (qualified here with 1.20.0). Override
`KURTOSIS_BIN`, `NANO_KURTOSIS_EXPORTER`, `NANO_KURTOSIS_IMAGE` or
`NANO_KURTOSIS_ENCLAVE` if necessary. Use a fresh enclave per campaign.
The wrapper retains its temporary package for diagnosis; genesis timestamps
are regenerated each time. Do not reuse old generated assets.

Assertions: P2P ports open and loopback RPC reachable from container exec, both peers connected, finality beyond the imported block-1 anchor,
and both nodes expose the same finalized execution hash. Engine API capability query
is printed independently; an unsupported method is not disguised as successful
Ethereum client interoperability.

This is a bounded system integration test on one Docker host, not full consensus
qualification, an adversarial campaign, a load test, or a production approval.
No transactions are spammed. The profile uses the existing native Beacon fixture
limits and is not a production genesis. Retention is deliberately bounded.

Nano does not currently provide the authenticated Ethereum Engine API. Using
ethPandaOps ethereum-package with a reference CL therefore requires an explicit
Nano EL adapter plus Engine API implementation and qualification. Merely setting
`el_client_type: custom` does not establish compatibility. Native consensus
qualification is a distinct task.

After saving evidence, remove only the test enclave:

```sh
.deps/kurtosis/kurtosis enclave rm -f nano-beacon-qualification
```

Observed evidence: `docs/qualification/kurtosis-20260926/summary.json`.
The successful finality check was a continuation after fixing harness assertions;
see both retained logs. This is not claimed as a clean first-attempt end-to-end run.

## Process crash with persistent witnesses

Start an isolated witnessed devnet (same runtime image, additional test config):

```sh
NANO_KURTOSIS_WITNESS_RECOVERY=true NANO_KURTOSIS_ENCLAVE=nano-beacon-crash scripts/run-kurtosis.sh
```

Then pass the two exact test container names to:

```sh
node tests/kurtosis/crash-recovery.mjs nano-0--ID nano-1--ID output-directory
```

The driver rejects unexpected container names, non-Kurtosis containers, other
images, missing recovery configuration and non-fixture chain IDs. It submits
24 signed counter-storage transactions using the public fixture key 777,
SIGKILLs node 0, reopens the same container and volume with its existing recovery
witnesses, and checks the finalized prefix. It rebroadcasts identical transactions
because mempool durability is not assumed, then requires successful receipts,
exact nonce/storage increments, and common finality covering the workload.
This externally timed SIGKILL does **not** establish a crash at an exact disk
write boundary, hardware power-loss durability or arbitrary corruption recovery.
The wrapper chooses recovery if the database directory exists and fails closed
if the required witness is missing; it is a fixture harness, not an operator
bootstrap or recovery policy.
