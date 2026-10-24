# Inventory bridge V2 — closed source prototype

Generation 02: the complete generation 01 and its manifests remain unchanged.
The four API/shared/abstract endpoint contracts are byte-identical. This
generation adds a separate actual two-client assertion suite, test-only client
drivers, and the corresponding injected runner step. It does not replace the
independent model with an execution claim or alter economic terms.

Status: **SOURCE ONLY / NOT EXECUTED / ECONOMICS NOT ADOPTED**. The author has
not compiled these files, run Node, deployed a contract, produced a signature,
contacted a network or changed the repository. Root must review and qualify this
separate lot on an authorized private backend after the consensus campaign.
Nothing here proves source inclusion/finality, USDC backing, production safety,
a Doppler launch, a Base/Nano token identity or a usable public bridge.

## Scope and composition

The existing V1 bridge is untouched. V1's initial-zero-supply mint/burn gateway
cannot simply import a Doppler preissuance as native Nano claims. Its canonical
escrow can already lock a preissued ERC20 and mint a *different* zero-supply
representation; that is a separate asset and does not convert it into native
Nano. V2 proposes a different, closed protocol: a predetermined, fixed-supply
Base ERC20 inventory and an initially funded native-currency reserve.

The four contracts under `source/contracts/bridge-v2` are an API library, a
shared abstract endpoint, an abstract Base inventory and an abstract native
reserve. `_requireRouteEnabled()` is deliberately unimplemented in the source
endpoints. There is no production verifier implementation. The arbitrary digest
approver, enable switches, token corruption, forced donations and unrestricted
fixture wrappers live **only** in `source/tests/contracts`.

The associated independent journal/model and Doppler proposal sidecar are
separate peer-owned source packages:

- `../inventory-bridge-v2-model-source-01`: economic journal oracle, trace vectors
  and seven new Node test groups; no endpoint-storage implementation imports.
- `../inventory-bridge-v2-doppler-source-01`: offline byte-pinned proposal join
  with the existing unsigned Doppler review and six new test groups. All launch,
  backing, simulation, activation and production claims remain false.

Their manifests belong to those packages. Do not silently copy mutable peer
files or reuse a prior green result. Compose only the exact reviewed pins and
the unchanged legacy dependencies named there.

## Terms and wire binding

`API.sol` defines `Terms` in this ABI order:

```
baseChainId, baseDomain, baseEndpoint, baseToken,
nativeChainId, nativeDomain, nativeEndpoint, nativeAsset,
totalBaseSupply, baseLot, nativeLot,
baseToNativeVerifierDomain, nativeToBaseVerifierDomain, maxProofBytes
```

`nativeAsset` is exactly `address(0)`, meaning native currency, not WrappedNano.
All amounts are atomic integer units. The only conversion is:

```
nativeAmount = (baseAmount / baseLot) * nativeLot
```

Remainders, zero amounts and uint256 overflow are refused. There is no rounding,
implicit decimal scaling, selected default ratio or price oracle. The entire
declared supply must be an exact number of lots and its complete native
obligation must fit uint256. Test fixtures use **2:3 atomic lots** and separate
pure/model examples use **2e6:7e18**; none is a proposed real exchange rate.

`termsHash` is `keccak256(abi.encode(TERMS_TYPEHASH, terms))`. The route commitment
is `keccak256(abi.encode(ROUTE_TYPEHASH, eight flat asset/domain fields))`.
The precise type strings are in `API.sol`; the offline proposal sidecar uses the
same strings and tuple order. Domain and verifier-domain pairs must be distinct
and nonzero; both chain IDs must be nonzero. Equal chain IDs are allowed only
as a wire possibility for separate synthetic domains. Local constructors check
their actual chain, self address, verifier code, and Base token code. This does
not establish the provenance or honesty of that code.

Every message binds its kind, route, terms, both source/destination chain-domain-
endpoint triples, nonce, transfer ID, sender, recipient, both amounts and proof
domain. One increasing outgoing counter covers **all** kinds. Consumption is
exclusive on `(sourceChainId, sourceDomain, sourceEndpoint, nonce)`, deliberately
excluding kind, transfer ID and payload: a contradictory second message for the
same source nonce cannot be paid as a different operation.

Kinds are fixed in this order, 0 through 11:

```
BOOTSTRAP, ACK_BOOTSTRAP, LOCK_BASE, ACK_NATIVE_PAID,
RESERVE_BASE, FUND_NATIVE, ACK_BASE_PAID, REQUEST_CANCEL,
ACK_CANCEL_UNFUNDED, CANCEL_FUNDED, ACK_NATIVE_REFUNDED, FREEZE_SUPPLY
```

This is not V1 encoding. No compatibility fallback accepts a V1 proof or drops
terms/amount/domain fields. `MessageSent` retains digest, source key and the
whole message, and each outbox nonce retains its original digest.

## Opening and full initial coverage

1. Base checks actual `totalSupply == totalBaseSupply` and emits one BOOTSTRAP
   for **the full supply**, including sold tokens, LP allocation, treasury,
   vesting and unsold balances. A donated/preloaded inventory balance creates no
   rights and does not reduce the initial obligation.
2. Native authenticates that message and accepts exactly the complete quoted
   native value in one payable bootstrap. Both insufficient and excess value
   fail. It records the reserve and emits ACK_BOOTSTRAP once.
3. Base authenticates that ACK before enabling locking or reservations.

There is no supply import/reset function, later unbacked mint or admin removal
of due collateral. A public production implementation would still need to prove
the source token, supply/issuer policy, finality and exact bootstrap event. A
digest approval in a test cannot do that. Preissuing a Base token never creates
the native reserve by itself.

## Transfers and cancellation states

Base→Native locks an exact debit and credit of existing ERC20 units. The Base
amount remains quarantined until an authenticated ACK_NATIVE_PAID. Native
consumes one source message and transfer ID, reduces only spendable accounted
reserve, then calls the recipient. A failed/reentrant receiver cannot commit
the debit, consumed bit, payment marker, ACK or outgoing nonce. Successful ACK
then makes the existing Base tokens available for reverse reservations.

Native→Base starts at Base: the future Base recipient reserves **available**
inventory and names the native funder (these may be different addresses).
There is no native deposit before a source reservation. On Native, exact funding
is held as refundable `pendingFunds`, not spendable collateral. Base pays only
the reserved existing ERC20 amount; only ACK_BASE_PAID capitalizes that funding
on Native. Remote balance snapshots or timeouts are never reservations.

| Branch | Base state | Native state | Capacity / funds |
|---|---|---|---|
| reserve | Reserved | None | Base stock reserved; no native debit |
| request cancel | CancelRequested | None or Funded | No capacity released; payout now refused |
| Native confirms no fund | CancelRequested | CancelledUnfunded | Future fund refused permanently |
| Base consumes unfunded ACK | CancelledUnfunded | CancelledUnfunded | Capacity released |
| Native funds before cancel arrives | Reserved or CancelRequested | Funded | Refundable funds held |
| Base pays before cancellation | Paid | Funded, then Paid on ACK | Refund/cancel refused |
| Base cancels funded reservation | CancelledFunded | Funded | Inventory released; payout refused |
| Native consumes cancellation | CancelledFunded | Refunded | Original funder refunded once |
| Base consumes refund ACK | Refunded | Refunded | Trace completion only |

There is no timeout-only refund. A request to cancel does not itself prove that
funding never happened. If funding wins that race, the unfunded ACK fails and
the funded-cancellation branch is required. The same FUND message cannot be
both settled and cancelled. If a native refund callback fails, its original
funding and unconsumed cancellation remain retryable. It cannot be both paid
on Base and refunded on Native.

## Accounting, donations and issuer mismatch

Base enforces `accountedBase = freeBase + pendingNativeBase + reservedBase` and
actual balance >= accounted inventory. Native enforces actual native balance >=
`accountedReserve + pendingFunds`. Donations remain surplus; they never create
available Base capacity, minting rights, conversion rights or spendable reserve.

The independent model additionally derives global obligations from its event
journal. For raw inventory B, raw native cash R, full supply T, unpaid forward
native PN and funded reverse obligations F not already paid on Base:

```
baseLot * R >= nativeLot * (T - B) + baseLot * (PN + F)
```

It accounts separately for donated surplus and local pending funding that is
not yet capitalized. A successful Base payout removes the corresponding global
F even while the Native ACK is delayed. This is not permission to spend locally
refundable `pendingFunds` before that ACK.

New Base locks/reservations/payouts check the fixed total supply **before and
after token IO**, as well as exact balance deltas. Detecting a mismatch can emit
an authenticated permanent FREEZE_SUPPLY to Native; new risk-taking then stops.
Existing ACKs and authenticated cancellation/refund paths remain possible.
This cannot make a dishonest, upgradeable, rebasing, temporarily corrupted or
lying token trustworthy. In particular, remote freeze delivery is asynchronous,
already-locked forward transfers may remain blocked, and no complete incident
recovery or liveness guarantee is claimed after issuer corruption.

## New tests and private execution adapter

`InventoryV2Cases` has 15 fresh-fixture cases: closed gate; full one-shot backing;
round trip and delayed capitalization; pre-fund cancellation; cancel/fund race;
payout-first exclusivity; native rejection/retry; exact guarded reentry; refund
rejection/retry; donations; supply freeze with surviving refunds; wire/domain/
source-nonce conflicts; pending funding excluded from collateral; token no-op,
tax/extra-mint/post-IO mismatch; and concurrent reservations funded in reverse
order. The last case uses two distinct native funder actors and **one** Base
recipient Fixture, not two independent Base clients. Failed calls require the
specific selector and unchanged global accounting/funds/nonces; key rollback
cases also inspect the individual consumed/payment markers and retry once.

`InventoryV2TermsCases` adds eight groups for 6/18 atomic lots, dust/zero/caps,
whole-supply remainder, uint256 overflow/boundary, domains/proof bound, separate
route/terms hashes, exclusive nonce across kinds, and native-asset/zero-lot
refusals. Fresh fixtures are deployed separately from the assertion contract to
keep creation/runtime size visible. No oversized bytecode is exempted.

`InventoryV2MultiClientCases` is a separate one-shot suite for the missing Base
ownership scope. Two distinct `InventoryV2BaseClient` contracts are both the
Base recipients and native funders. Fixture is their explicit test driver, but
reserve/requestCancel/cancelFunded/fundNative reach the endpoints with the actual
actor address as `msg.sender`. Driver methods are only in the test namespace;
they are not production account or authorization machinery.

Starting from 100 available Base units, A and B each reserve 40. Another 22
cannot oversubscribe the remaining 20; B cannot cancel A's reservation; A cannot
fund B's message. Funding is delivered in reverse order B then A. B receives 40
actual ERC20 units; A cancels its funded transfer and receives exactly 60 native
units back. A cannot cancel B's funded message. Settlement/refund/ACK replays
are refused with the exact state error, while actor balances, both reservation
and fund records, inventory, outgoing nonces/outboxes and source consumed bits
remain unchanged after every refusal. Final Base free/accounted inventory is
60, native accounted reserve is 1410, pending funding is zero, A has 60 native
and zero Base units, B has zero native and 40 Base units. Final outgoing nonces
are Base 7 and Native 6. These are hard-coded synthetic assertions for future
execution, **not results obtained here**.

`scenario.mjs` performs those **24** assertion transactions (15 economic + 8
terms + 1 real multi-client) and a **separate paired
trace**, using the independent model, actual message logs, separately encoded
digests/source keys, outboxes, exclusive consumed bits, contiguous nextNonce,
bootstrap/gate states and physical balances after each transaction. Its EOA
gas payer is outside the Fixture economic ledger: the Fixture/Actor contract
balances do not pay outer transaction gas. Every receipt still records actual
gasUsed and effectiveGasPrice; the independent oracle supports explicit receipt-
based gas charges when an adapter models EOAs directly. No balances are silently
normalized to hide a failed rollback.

The optional `ethers-backend.mjs` accepts an **already-created** private provider
and signer plus an explicit chain ID, gas limit and parent deadline check. It
creates no RPC URL, wallet, keys or backend process. It refuses common Ethereum/
Base public chain IDs, but a chain ID alone is not proof of isolation; Root must
establish that separately. Status-0 receipts must be actual mined receipts,
not estimate/RPC/import exceptions. Caller checks are cooperative; external
process cancellation and cleanup remain the parent controller's responsibility.

Suggested qualification composition, not executed here:

1. Compile only this closure with the already-present pinned solc 0.8.28 and
   OpenZeppelin dependencies. Record optimizer, fork, source hashes and output
   artifacts. Resolve import roots explicitly; install no new dependency.
2. Run the seven independent model groups and six offline proposal groups on
   their exact composed pins. Keep the previous failed Base simulation intact.
3. On one approved, isolated private chain, inject its provider/signer and run
   `runInventoryV2Scenario`; use a bounded parent deadline and retain actual
   receipts and failure. The runner checks EIP-3860 initcode <=49152 bytes and
   EIP-170 runtime <=24576 bytes. Do not raise limits to disguise a failure.
4. Root may repeat the new contract scenario on Nano's actually activated EVM
   profile and a reference backend, clearly labeling each scope. This adds no
   consensus, restart, network, public-finality or production qualification.

No compile, scenario, adapter, model or proposal test in this lot has been
executed by this author. The complete result fields keep production/finality/
real-assets/economic-adoption false even if a future synthetic execution passes.

## Decisions and remaining implementation outside this lot

After an executable prototype is reviewed, humans must choose whether the Base
asset is an independent canonical ERC20 or a claim on native Nano; total supply,
mint/upgrade policy, allocations, atomic lots, reserve provenance/ownership,
funding, redemption rights and issuer obligations must agree. Neither 1:1 nor
any of the test ratios is adopted. Proof authority/governance, finalized source
inclusion, reorg handling, compromise/recovery and production gate design remain
open. So do actual token deployment and identity binding, real onchain supply
and reserve verification, Doppler economics/timing/graduation, visibility at T0,
third-party indexing, bridge operational monitoring and independent security
review. No funds, sale, deployment or public activation is authorized by this
source proposal.
