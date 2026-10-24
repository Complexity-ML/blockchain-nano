# Exact Native→Anvil raw differential — SOURCE ONLY

This single driver proposes replaying the **same65 retained raw signed transactions** from Root's already completed Native execution on a new isolated Anvil. It does not recompile Solidity/Core, rerun Native/the scenario, create signatures, instantiate a Wallet, load private keys, contact public RPC, bridge assets or start a consensus campaign. No Node, Python, test, Docker, SSH, Git or actual/private input was executed/read by this author. This source package makes no claim that the replay passes. The reported324gas total difference between the earlier distinct-sender runs remains unexplained until a controlled comparison produces evidence.

```text
node raw-differential.mjs OUT NODE_MODULES CASE_ROOT ANVIL ANVIL_SHA NATIVE_ACCOUNTING NATIVE_ACCOUNTING_SHA
```

All five paths must be absolute; OUT must be new. Root supplies and pins the Anvil binary, exact Native `native-accounting.json`, source/helper files, existing ethers6.16.0 dependency tree and isolated environment. CASE_ROOT supplies only the existing `tests/multihost/campaign-{deadline,commands}.mjs` helpers. No compiler/scenario/artifact input is needed. Root must ensure the dedicated network namespace has no earlier listener on127.0.0.1:18546. The endpoint is hardcoded loopback and accepts no remote URL parameter.

The proposed fixed CLI is:

```text
--host 127.0.0.1 --port 18546 --chain-id 31337 --hardfork shanghai
--gas-limit 60000000 --accounts 0 --disable-default-create2-deployer
--base-fee 0 --gas-price 1 --timestamp 1700000000 --no-mining --silent
```

Root confirmed these options in its retained Anvil1.5.1 help, including `--no-mining`; there is **no `--coinbase` flag**. The driver uses `anvil_setCoinbase` and verifies the standard RPC `block.miner` field after every transaction. It requires block0 and `eth_accounts=[]`, no default CREATE2 deployer code, and funds **only the three public fixture addresses derived from keys1/2/3** to10^24 units through private `anvil_setBalance`. Their code/nonces and the initially empty coinbase balance are checked. Computing these known public addresses is not signing. There is no adaptive fixture adjustment after a mismatch.

Before starting Anvil, all65 input raws are decoded and their exact byte serialization/hash, recovered public pk1 sender, nonce0..64, type0, chainId31337, gasLimit30000000, gasPrice1 and value are checked against the retained Native rows. Native row/root/fee continuity and its final accounting total must be coherent. Each transaction then receives timestamp1700000000+12×step, is submitted with `eth_sendRawTransaction` **unchanged**, and is mined in exactly one explicit `evm_mine` call. No other block is deliberately mined. Every mined receipt/transaction/block must join the retained hash/sender/nonce/status/gas, target/calldata/value/gas fields, single-transaction canonical block, block number/time/gasLimit/baseFee/miner, exact sender balance and cumulative coinbase fees. Its block **stateRoot must equal that row's Native executionStateRoot exactly**.

The genesis header may retain a stateRoot predating `anvil_setBalance`, so no false equivalence is asserted from that stale header. **All65 post-transaction roots remain mandatory.** A first mismatch is retained in the bounded trace/controller evidence and causes nonzero exit; there is no normalization, alternate sender, state patch, root suppression or automatic rerun. A shape/protocol refusal also fails; diagnostics may then contain only the completed prefix. These observations would qualify only this exact retained trace/configuration, not general EVM/RPC/consensus equivalence or production.

The driver reuses the existing one owned process-group cleanup and monotone budget helpers: maximum540000ms including60000ms reserved for closeout; Root must impose a finite outer controller budget and observe the real exit. Source/input/binary FDs and their path/full-nine rows remain held/fenced, all explicitly opened FDs are closed, and an early Anvil exit is both a failure and still subject to owned-group cleanup. Anvil is joined before terminal adoption, and the provider is destroyed. JSON/stdout intentionally report `passed:false` and provisional `terminalEvidenceReady`; Root alone may adopt after its actual outer terminal, elapsed time, group absence, provenance/dependency and closure checks. No SDK closure, hard-wall guarantee, active network fork, real reserve or production readiness is inferred here. Historical Native, earlier Anvil and postvalidator failures stay intact.
