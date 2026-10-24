# Inventory bridge V2: closed prototype

## Qualification and scope

The reference execution **passed** on source candidate
`e7d1e178868d26f42caa64db492c68103bdb6371`: solc 0.8.28, Shanghai,
optimizer 200, `viaIR:false`, Anvil in an isolated AMD64 Docker container.
It completed **24 assertion transactions, 65 outer transactions, 22 paired
accounting checkpoints and three expected mined status-zero transactions**.
All 65 EOA debits matched actual receipt gas plus the transaction value.
Driver and outer controller exited 0; the owned Anvil group was joined.
The final driver/outer descriptor counts were respectively 33/33 and 38/38.

The two economic suites have runtime sizes 18919 and 16613 bytes; the Fixture
has initcode 44005 and runtime 12169 bytes. EIP-170's 24576-byte runtime and
EIP-3860's 49152-byte initcode limits remained strict, including constructor
arguments in the execution checks. The previous 27918-byte monolithic suite
failure is retained; splitting its dispatcher preserved all 15 case bodies,
IDs and assertions. Seven independent-model and six offline-Doppler test groups
have separate passing qualifications; this execution did not replay them.

Public evidence: [scoped qualification](qualification/closed-inventory-v2-20261025/README.md).
The self-written controller report remains provisional (`passed:false`); the
independent outer result and observed process exit establish this limited result.
These are reference contract tests. Nano-native execution remains open.

## Reproduction

Run the committed driver inside an already isolated Linux Docker container with
Node18.19.1, its shared-library providers and the existing dependency/source roots
mounted read-only. The exact tested invocation is retained in
[reference-argv-public.json](qualification/closed-inventory-v2-20261025/reference-argv-public.json).
The existing image alone does not contain the external Node runtime. No image
build or port publication is required; the recorded limits are2GiB,2CPUs,
networknone, UID1000, read-only root and a bounded writable result directory.
The parent must enforce600s, observe the real exit and verify its exact owned CID
is absent; terminating only the Docker client is insufficient.

Inside that container, the driver interface is:

```sh
node "$CASE_ROOT/tests/hardhat/inventory-v2/qualification/driver.mjs" \
  "$NEW_OUTPUT_DIR" "$NODE_MODULES" "$CASE_ROOT" "$ANVIL_BINARY" \
  b47362d2159aa0f2f575320e5e529bb5a91093cb62dc6bd30c0022018aa9f738
```

CASE_ROOT must contain the source-bound8 Solidity units, scenario/backend,
independent model and `tests/multihost` command/deadline helpers. NODE_MODULES is
the verified solc0.8.28/OpenZeppelin5.0.2/ethers6.16.0 dependency tree. The driver
creates a private Anvil1.5.1 at127.0.0.1:18545, chain31337, with the public fixture
mnemonic and one unlocked test account. Caps are30Mgas/transaction,60M/block;
the driver budget is540s including60s for closure. No real funds are used.

## API and accounting

V1 is unchanged. V2 binds a predetermined preissued Base ERC20 to a proposed
native-currency reserve; it neither remotely mints WrappedNano nor establishes
that a real Base token is the native Nano asset. Its 14 immutable terms are:

```
baseChainId, baseDomain, baseEndpoint, baseToken,
nativeChainId, nativeDomain, nativeEndpoint, nativeAsset,
totalBaseSupply, baseLot, nativeLot,
baseToNativeVerifierDomain, nativeToBaseVerifierDomain, maxProofBytes
```

`nativeAsset == address(0)`. Atomic conversion is exactly
`nativeAmount = (baseAmount / baseLot) * nativeLot`; zero, remainder and uint256
overflow are refused. The contract fixture uses **2:3** lots; independent
model/proposal examples use **2:7**, including explicit 6/18-decimal units.
**Neither ratio, allocations nor any economic relationship is adopted.**

The 12 message kinds, numbered 0 through 11, are:

```
BOOTSTRAP, ACK_BOOTSTRAP, LOCK_BASE, ACK_NATIVE_PAID,
RESERVE_BASE, FUND_NATIVE, ACK_BASE_PAID, REQUEST_CANCEL,
ACK_CANCEL_UNFUNDED, CANCEL_FUNDED, ACK_NATIVE_REFUNDED, FREEZE_SUPPLY
```

Each message binds kind, route/terms hashes, both chain/domain/endpoint triples,
nonce, transfer ID, sender, recipient, both amounts and verifier domain.
The exclusive source key is `(sourceChainId, sourceDomain, sourceEndpoint, nonce)`
across all kinds and payloads. A shared outgoing counter and retained outbox
prevent a second conflicting kind from spending the same source nonce.

Opening is one-shot: Base declares the **entire** fixed supply, Native receives
exactly its full quoted backing, and Base authenticates the ACK before trading.
Unsold, treasury, vesting and LP allocations remain within that full obligation.
No preloaded inventory imports claims; no administrator can withdraw due backing.

Base→Native locks exact ERC20 deltas. Inventory remains quarantined until the
native payment ACK; native recipient rejection rolls back consumption/payment.
Native→Base first reserves actual available Base inventory for its recipient
and named funder. Native funding remains refundable and cannot fund forward
payments; only an authenticated Base payout ACK capitalizes it as reserve.
Two real contract clients exercise ownership, reverse delivery, payout/refund
exclusivity, oversubscription and replay refusals with complete balance readbacks.

Cancellation releases no capacity from a request alone: an authenticated Native
unfunded-cancel ACK is required. If funding wins the race, Base cancels before
payout and Native refunds the original funder. A paid Base transfer cannot also
be refunded. Failed native callbacks preserve the same-ID retry; no timeout-only
refund or administrator rescue path substitutes for authenticated evidence.

Base enforces `accounted = free + quarantined + reserved`; Native enforces
`cash >= accountedReserve + refundableFunds`. The independent journal also
checks `baseLot*R >= nativeLot*(T-B) + baseLot*(PN+F)` for raw inventory B,
native cash R, full supply T, unpaid forward PN and funded reverse obligations F
not yet paid on Base. Donations create no rights or free capacity. Supply/delta
checks surround token IO; observed issuer mismatch freezes new risk while
existing authenticated refund paths remain available.

## Remaining gates

Endpoints are abstract and activation remains closed. Arbitrary digest approval
and permissive drivers exist only in the test namespace. Actual source inclusion,
finality, reorg handling, verifier/issuer compromise and recovery are **not**
authenticated or qualified. Replay tests do not make a dishonest proof authority
safe; asynchronous freeze delivery does not repair a compromised token.
Real token identity, reserve ownership/provenance and onchain collateral coverage,
conversion/redemption terms, production enablement, independent security review,
Nano-native execution and public bridge operation remain open. Offline Doppler
encoding and these synthetic tests do not prove a sale, migration, liquidity,
indexing at T0 or a production-ready Base–Nano relationship.
