# Inventory V2 on the actual C++ execution engine

**SOURCE ONLY.** No Node, compiler, project binary, Docker, SSH, Git operation,
test, key or private artifact was executed/read by this author. This package
changes no Core, production source, CMake target, frozen GEN03 source, existing
Anvil result or private campaign. Root alone supplies/adopts the real link recipe,
builds the single test translation unit, authenticates artifacts and runs it.

## Exact entry and prerequisites

```
node /proposal/native-driver.mjs OUT NODE_MODULES CASE_ROOT SHIM SHIM_SHA \
  COMPILER_OUTPUT COMPILER_OUTPUT_SHA COMPILER_INPUT_SHA
```

All five path arguments are absolute; all three SHA arguments are externally
retained SHA256 values. `OUT` must be absent, its parent must exist. The other
inputs and dependency tree are read-only. Keep `native-driver.mjs`,
`native-backend.mjs` and `scenario-native.mjs` together under `/proposal`.
The shim accepts **no arguments**, and receives only bounded JSONL on stdin.

`CASE_ROOT` is the unchanged GEN03 source projection: four `contracts/bridge-v2`
units, four `tests/contracts` units, the existing command/deadline helpers under
`tests/multihost`, and `tools/bridge-inventory-v2/inventory-model.mjs`. The driver
verifies all eight frozen Solidity pins and the exact independent oracle pin.
`NODE_MODULES` provides the already-present ethers 6.16.0 ESM tree and the nine
OpenZeppelin source files from the retained hardhat dependency closure. No solc
invocation or install occurs in this driver.

`COMPILER_OUTPUT` is the actual public GEN03 `compiler-output.json` from Root's
passing reference execution. Root supplies its prior external SHA and the prior
Standard JSON input SHA. The driver reconstructs that exact input from all 17
held source files, Shanghai/optimizer200/`viaIR:false`/output selection, compares
both external pins, requires a successful solc 0.8.28 result, and retains strict
49152/24576 initcode/runtime bounds. ABI shape or a freshly observed hash alone
is insufficient adoption authority. Constructor arguments and actual native
runtime are also size-checked by the scenario. All 24 contract assertions and
22 physical/model checkpoints remain; no model/Doppler test replay is added.

## One test TU and the retained Core

Compile `inventory_v2_execution_runner.cpp` only, with genuine retained recipe
flags/includes, then link its new object to the exact admitted G2 archive and
existing providers. No Core/archive/image build, CMake configure or dependency
download is needed. Root supplied this current AMD64 Core locator/binding:

```
/home/ubuntu/cli-readonly-amd-root-20261018-01/outputs/libnano_core.a
a1475733637348e9a22e72d6f8be3d61ebeafb39443cb168110ca353781ae2e6 / 41077050 bytes
```

That is a **parent-supplied** authority, not a historical row recovered or
requalified by this package. Root binds current d234-derived C++ source/headers,
the archive, compiler, actual library/object inputs and new binary provenance.
The driver admits the resulting `SHIM` against the externally supplied SHA and
holds its full-nine metadata; it does not infer its linked Core from a banner.

Root's actual ordered Linux link tail, retained rather than invented, is:

```
/old/third_party/mcl/bint64.o /old/third_party/mcl/base64.o
/outputs/libnano_core.a
/old/third_party/ckzg/libnano_ckzg.a /old/third_party/ckzg/libnano_blst.a
/old/third_party/secp256k1/lib/libsecp256k1.a
/usr/lib/x86_64-linux-gnu/libgmpxx.so /usr/lib/x86_64-linux-gnu/libgmp.so
/usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libcrypto.so
/usr/lib/x86_64-linux-gnu/libicuuc.so -ldl
/usr/lib/x86_64-linux-gnu/librocksdb.so /old/lib/libmcl.a
```

Compile flags supplied by Root are `-O3 -DNDEBUG -std=gnu++20`,
`-DMCL_BINT_ASM_X64=0 -DMCL_DONT_USE_XBYAK -DMCL_FP_BIT=384 -DMCL_FR_BIT=256
-DMCL_MSM=0 -DMCL_PORTABLE=1 -DMCL_USE_LLVM=1`, with the actual retained include
paths `/work/include`, `/work/third_party`, CKZG src/generated include, BLST
bindings, secp256k1 include and MCL include. Use argv, preserve provider order,
capture the real new MD, and authenticate all selected inputs. Root decides the
actual output/bind mappings; no global SDK scan or guessed fallback is supplied.

## Actual execution evidence, not mined receipts

The shim derives from the existing bounded bridge execution runner and invokes
real `ExecutionEngine::apply_transaction` with **Shanghai** explicitly selected.
Read simulations select Shanghai too and run on a discarded isolated `State`
copy. Accounts use the const State getter; absent reads do not insert entries.
Only fixed public fixture keys 1/2/3 exist in this test process. No host key,
validator, DB, RPC server, P2P, real chain or capsule is opened. One fresh in-memory
State slot, chain 31337, hosts the same synthetic route and two Base client actors.

The envelope is `nano-in-process-shanghai-v1`, `mined:false`,
`syntheticContext:true`, `consensusValidated:false`. Actual signed raw transaction,
its hash, EVM success/logs/gas, sender balance/nonce and MPT State roots are
returned. The supplied header's number/time are synthetic execution context;
they are not mined block evidence. Neither `Receipt.block_hash` nor
`Receipt::to_json()` is exported. There is no manufactured blockHash/blockNumber.

The injected adapter takes only `request`, ethers/artifacts and a deadline
callback. It creates no process/provider/socket/wallet. For every native send it:

- recomputes the raw transaction hash and verifies recovered public sender,
  chain/type/nonce/value/to/data/gas fields against the actual request;
- joins actual pre/post roots and execution step to independent root/account
  queries, requires one consumed EOA nonce, and verifies the CREATE address;
- verifies exact actual EOA debit = gas*price + successful transferred value;
  these fixtures return no payment to the outer EOA;
- observes the real coinbase balance increase at price1/baseFee0, then reconciles
  all 65 gas charges at completion. No invented fee sink or wallet normalization.

The scenario overlay requires explicit `evidenceMode:"nano-in-process-shanghai-v1"`
and a matching native backend. Only receipt validation/report metadata differ;
all GEN03 assertion/trace bodies are preserved. Its default remains the original
strict mined/Anvil branch. `scenario-native.patch` records the whole finite diff
from frozen GEN03 (`ef273dc8…`); the acquired original file stays unchanged.
No new qualification of the default overlay branch is claimed without execution.
Gas equality with Anvil is not expected; synthetic headers/addresses differ.

## Bounds, terminal evidence and failure

The driver runs one registered detached child, one matched JSONL request at a
time, <=5000 requests, <=2 MiB request/response lines, <=32 MiB stdout and
<=16 KiB stderr. Every `ok:false` protocol/admission error throws. Only an actual
`success:false` engine result can satisfy one of the three expected EVM refusals.
Those failures must use less than their full 30M gas allowance: an exceptional
full-frame OOG/invalid-opcode failure cannot substitute for an expected revert.
Malformed IDs/JSON/results, extra replies, timeouts, early EOF/exit, unexpected
stderr, root/fee mismatches and partial case lists fail qualification.

Budget is cooperative 540s with 60s closeout; requests have <=30s timeouts and
final EOF/close <=5s. Root must also enforce an independent outer 600s bound.
Successful completion requires all 24 original IDs, exactly 65 actual sends and
22 paired checkpoints, clean EOF with actual exit0/close0/null signals, drained
protocol, full-nine input fences, all explicitly opened descriptors closed and
absence of the owned process group. Cleanup uses only that registered child.
Outputs are exclusive, fsynced and retained on failure. The self-written report
always says `passed:false` and requires Root's independently observed real exit.

Outputs include artifact reuse binding, native scenario receipts, raw signed
transaction/account/fee/root joins, bounded transport metadata and provisional
terminal evidence. `coreAndLinkProvenanceVerifiedByDriver:false` distinguishes
Root's separate build provenance from in-process behavioral observations.
The prior driver draft `7fe17fab…` is preserved in `rejected-source/draft01`: its
source review found pending cleared before result validation could orphan the
awaiting Promise. The current parser validates before clearing that request.
`rejected-source/draft02` preserves backend `5e614127…` and driver `8eeaf0e0…`
before the additional strict failed-frame gas guard; no version was executed by
this author. The current driver changes only that backend pin from `8eeaf0e0…`.

## Scope remains open

Passing would qualify these contracts only on the actual in-process C++ Shanghai
path linked by Root. It would not activate a Nano network fork or qualify RPC,
mined blocks, consensus, P2P, RocksDB/WAL, restart/portability, cross-host finality,
source inclusion/finality authentication, reorgs, compromised proof authority,
real collateral or production. Full preissued supply/lots remain synthetic: the
2:3 contract fixture and 2:7 model/proposal examples adopt no economic ratio.
Real bridge activation, token identity/reserve ownership, funding/redemption,
Doppler sale and independent security review remain separate. No actual result
is claimed by this source package; Root alone will compile and execute it.
