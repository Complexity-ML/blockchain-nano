# One local Anvil integration of the current V2 migration verifier

Source-only review of Root-reported R revision f851c3bd. No Node/Anvil/RPC, tests, Git, Docker, SDK/module installation, private inputs or R writes were performed. Independent peer reviewed the verifier and migration units too. No certain product defect was found, so no patch or extra runner is proposed. Campaign03 approval remains unrelated and pending.

The useful missing proof is narrow: `migration.test.mjs:13–19` fabricates Transfer/Migrate logs, and `:27–32` reencodes mocked state responses from the historical report. Those 29 units do not prove that the historical deployed V2 migrator really emits one ordered zero → migrator → beneficiary route before Migrate, or that a real Anvil accepts all six canonical-hash state calls. The mock dispatch primarily distinguishes selectors, so it is not an independent check of every encoded token/beneficiary argument. Current verifier source encodes the right arguments; this is a coverage limit, not an observed bug.

## Minimal Root preflight

Use one **fresh owned** Anvil, loopback 127.0.0.1:19546, chain31337, new output directory and no other writer. The existing README pins fork height52173454; retain its actual Base fork block hash, Anvil identity/version and source pins. Driver lines23–25 check chain/version and record `anvil_nodeInfo`, but do not themselves assert the upstream fork identity. Confirm the intended historical module code exists at that fork, including Airlock, the selected factories/initializer/migrator, Permit2, router and quoter, plus Base USDC with decimals6 and its master minter. In particular the chosen V2 migrator must be `0x5F3bA43D44375286296Cb85F1EA2EBfa25dde731`; otherwise refuse before impersonation/writes rather than weaken verifier line35. Root must also check Anvil supports EIP1898 at the canonical hash; no height/latest fallback. Offline preview code is reusable for parameters, but its on-chain preview deliberately requires chain8453 and is not the chain31337 fork driver.

Reuse the already pinned package-lock dependencies, including explicit router .js entrypoints (driver5–7). Ensure gas/block limits permit the existing 15M-gas transactions, use a whole-child deadline and cleanup only that newly owned Anvil. The script has per-read limits but no whole-run process bound. Do not import `fork-lifecycle.mjs` as a harmless library: it is a top-level executable. It writes only its fixed loopback URL, impersonates synthetic fixture accounts/master-minter, uses no real private key and performs no native Nano operation.

## One run and the exact assertions

Run the existing driver once in that namespace. It already performs create → premature migration refusal → attacker ownership refusal → synthetic USDC funding → buy400USDC → sell10% of acquired tokens → proceeds check → time advancement → migrate → current verifier → double migration refusal → current verifier again (driver78–125). No separate deployment script is needed. Keep the actual mined receipt/logs and real RPC replies; never replace their LP logs with the unit fixture. Driver120/123 already pass the actual migration receipt hash to the new verifier.

Require terminal exit0 **and** `result.complete===true`, one successful create/buy/sell/migrate with the reported transaction hashes and status, exact input deltas, positive outputs and output delta at least the recorded99% quote minimum. Require nonzero reserves/supply/beneficiaryLP and one real LP attribution with `mintLogIndex < creditLogIndex < migrateLogIndex`, credit≤mint and attributed net credit>0. Pair and beneficiary must match actual hash-pinned getAssetData; currencies must be token+USDC and noOp beneficiary the dead address. The verifier enforces receipt/log bindings, six exact-hash eth_calls and before/after canonical headers (verify15–60). A bounded Root RPC observation can count these calls without replacing responses; if there are two verifier invocations there should be twelve such state calls in total. Record the selectors as well as the success booleans.

For all three adversarial **transactions**, require the expected explicit eth_call revert selector, actual mined receipt reverted, gasUsed<15M, and exact equality of the existing before/after contract/token/USDC/reserve/LP snapshots (driver62–75). This does not assert unchanged account ETH fees/nonces or every EVM storage slot. Require all four adversarial report flags true and exact equality of the migration proof before/after replay refusal. Optional read-only source replays of the actual receipt may test missing/ambiguous/cancelled attribution through the same verifier; label such modified-receipt replays as verifier refusals, not transactions executed by Anvil. Existing 29 units remain the deterministic malformed/reorg mock coverage.

A successful run would qualify this synthetic historical V2 integration on the owned fork only. It would not prove a live Base migration/reorg/finality, arbitrary module semantics, audited bytecode trust, launch terms, native Nano, a bridge or campaign03 permission. Unsupported EIP1898 or a different real LP route is an actual refusal to preserve, not a reason to relax the verifier.

## Reviewed public source pins

- `fork-lifecycle.mjs`: `f34aa7689ad4401402e052b157dd4369e9403830bfa2315597c9bec9d2906a62` / 11475 bytes.
- `verify-migration.mjs`: `fe362e75e277ef4bb9d7162db866400624dfd4f6658f4ec9e6332e0bd2b0ab13` / 7533 bytes.
- `migration.test.mjs`: `cf7fbbeb5e1b9d9fb0baaf19a92bd839b206ea19fd41e5f8ab4fc3787036f9ae` / 8401 bytes.
- `preview.mjs`: `4799ace32712390254066366acf6c287ab029bd354869afd122cae1eda58ab2c` / 10746 bytes.
- `example.synthetic.json`: `f240e2822cbea0aea5e5b95cef1bd47814c2f4c73011864be6eeb446ef347bb7` / 965 bytes.
- `package.json`: `c64df2ff667febf5e3370544f6b47a5645cfe738a3f7a5d46d1a02a585c64ebd` / 341 bytes.
- `package-lock.json`: `3c24823625c8ca5ff1b205c00f13c7e7309c8c1ffd45b45172a924e4428f102c` / 48068 bytes.
