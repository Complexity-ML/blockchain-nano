# Independent InventoryBridge V2 conservation model — SOURCE ONLY

This is an unexecuted prototype model, proposed tests and hand-calculated trace
vectors. The author has run no Node, tests, Solidity compiler, Nano runtime,
Docker, SSH or private-data reads. No R files or campaign 04 were changed. No
price, real ratio, cap, inflation policy, launch allocation, fee, bridge trust,
sale or production activation is adopted. All examples are synthetic.

## Files and independence

`inventory-model.mjs` imports no Solidity, SDK, implementation helper, runtime,
filesystem or network code. It accepts explicit atomic-unit parameters and
recomputes an independent economic journal from the initial balances after
every appended fact. Only a fully valid resulting journal is committed; a
refused append cannot change the prior facts, messages, replay set, balances or
rights. It does not copy a Solidity storage snapshot and call that conservation.

The agreed V2 enum and 16 message fields are reproduced as the public interface,
not as a cryptographic implementation. Source references are equivalent to the
fixed domain's `(sourceChainId, sourceDomain, sourceEndpoint, nonce)`, with no
kind/transfer/terms/amount in the deduplication key. A claim must match the
already-recorded source emission exactly. This models an authenticated source
fact; it does **not** implement proof verification or prove source finality.
`proofAccepted:false` is only a negative synthetic fixture input.
The factory is an economic oracle, **not** the shared fourteen-field Terms
admission validator: it does not even receive baseToken/nativeAsset/maxProofBytes
and checks domain hashes for shape, not all nonzero/distinct policy constraints.
Its parameters must already have passed the separate API/sidecar Terms review.
An invalid-domain input producing consistent arithmetic must never be presented
as an admitted route; the supplied vectors use explicit nonzero, distinct domains.

Public draft API/actors were read solely to align the common protocol after the
independent ledger equations were fixed. Base declares supply; Native accepts
exact payable backing and emits ACK; Base confirms it. The Base recipient
requests/cancels a reservation, while its Native funder may be a different
address. A cancel request excludes Base payout even if funding wins the race
before Native receives the request. Cancel requests do not release capacity;
an unfunded Native ACK or irreversible funded Base cancellation does.

`inventory-model.test.mjs` defines seven targeted test groups. It has not been
executed. `trace-vectors.json` contains an explicit two-client interleaving and
manual checkpoint values, including refused operations, receiver failure/retry,
both cancellations, delayed acknowledgements, donations and a final full ledger.
Those values were written as reviewable arithmetic; no test result is supplied.
The separate draft Solidity suite uses two distinct Native funder Actors with
one Base recipient Fixture. That is distinct from this manual vector's two Base
wallets; neither source-only scenario is evidence of two real deployed clients.

## Exact independent equations

Let `p=baseLot`, `q=nativeLot`, `T=entire preissued Base supply`, `B=actual Base
inventory`, and `C=T-B`. `p` and `q` are positive uint256 atomic-unit quantities.
They are not human token prices or decimal-display scalars. A transfer must be
an exact lot; a remainder is rejected, never silently rounded. Division precedes
multiplication, with an explicit final uint256 bound. The whole supply must be
an exact lot, and every initial allocation must sum to `T`.

The journal independently derives:

- `Q`: Base returned but not yet acknowledged as paid on Native, in Base units.
- `PN`: Native still owed for accepted Base returns, in Native units. A Native
  payment removes `PN`; only its later Base ACK removes `Q` and frees inventory.
- `PB`: Base reserved for nonterminal Native purchases, in Base units.
- `F`: Native funded for a Base purchase that has not yet paid Base or refunded
  Native. This is a **global economic** pending amount.
- `pendingFunds`: the **Native-local** funded amount awaiting Base-payout ACK or
  Native refund. It can remain nonzero after Base payout while its ACK is delayed.

At each admitted journal prefix:

```text
B = freeBase + PB + Q + donatedBase
B + sum(Base wallet balances) = T
R = accountedReserve + pendingFunds + donatedNative
R + sum(Native wallet balances) + executionFees = initial Native total
p*R - q*(T-B) - p*(PN+F) = q*donatedBase + p*donatedNative
```

The last identity is checked after Native bootstrap. It uses arbitrary-precision
integer intermediates with bounded stored amounts and no floating point. A
fractional donation does not invalidate it. `PN` must not be confused with the
Base contract's `pendingNativeBase` counter: the latter corresponds to `Q`.
Likewise `F` must not be substituted for Native-local `pendingFunds` before ACK.

Bootstrap accounts for **all** `T`, not only sold tokens or tokens deposited in
the bridge. No initial inventory is imported as available credit. Donations
create neither a message, payout entitlement, reservation credit nor accounted
Native reserve. The model conservatively excludes donated Base from free stock.
It capitalizes Native funding only after irreversible Base payout ACK, so a
refundable deposit can never be spent to pay somebody else's Native redemption.

Execution gas is separate from bridge fees: an optional `chargeGas` fact debits
the observed payer and records an external fee sink. It never changes backing
or pending bridge claims. A real adapter must derive it from actual receipts;
a reverted transaction may charge gas while its bridge effects fully roll back.
If a fee recipient is among the observed wallets, the adapter must account for
that receipt-backed credit too before claiming exact wallet reconciliation;
the current source model does not invent those credits or validator issuance.
The manual economic trace has zero execution fees and is not a raw funded-Anvil
transaction trace.
The proposed backend harness keeps modeled funds in Fixture/Actor contracts,
whose balances do not pay the outer EOA's gas. Its controller must verify that
boundary and report receipt-calculated gas separately; this does not by itself
prove the outer wallet's physical debit.

## Scenario coverage proposed, not executed

The fixtures exercise a non-1:1 atomic lot pair `2:7`, total supply `1000`, and
full backing `3500`. Base/native display metadata is explicitly `6/18`. A second
quote case scales the lots explicitly to `2,000,000 : 7,000,000,000,000,000,000`,
so `1,000,000,000` Base atoms quote to `3,500,000,000,000,000,000,000` Native atoms.
Neither example is a selected Nanobees conversion or a price against USDC.

New properties include full-supply opening rejection, uint256 intermediate and
result bounds, all 16 wire fields, contradictory source nonce payloads, source
replay, two independent deliveries out of order, no early stock credit, exact
reserved funding, inventory exhaustion, concurrent reservations, both cancel
races, no payout/refund double settlement, refundable-fund isolation, donation
non-entitlement, distinct recipient/funder, native-call refusal and retry.
Receiver `revert`, `reenter-revert`, and `reenter-caught` labels describe expected
economic outcomes for later backend scenarios. They do not run a callback or
prove a reentrancy guard; an actual backend must demonstrate the nested refusal,
outer receipt status, exact transfers and rollback separately.

Supply mismatch/freeze is a control-plane observation, not legitimate inflation
in this ledger. New Base risk stops when Base is frozen; Native risk stops after
the authenticated freeze message is accepted. Existing cancellations, refunds
and ACKs remain possible. The snapshot sets
`physicalBaseSupplyConservationClaimed:false` after an outside mismatch: it then
tracks only the original admitted claims and legitimate cash flows, not the
counterfeit token distribution. No new backing is inferred and no supply is
rebased. Frozen unpaid Native rights/liveness remain unqualified.

## Pairing with a later real backend

The source suite imports no backend and is not an Anvil/Nano qualification. A
later adapter must obtain real committed receipts and paired Base/Native
checkpoints, pin block/transaction identities, account for actual gas and decode
events. It must feed only their observed effects to the independent journal and
compare the complete `financial` object through `assertObserved`, including
wallet maps and all counters. It must not pass the model's own snapshot back as
if it were external evidence. Refused operations require before/after bridge
readbacks, real receipt status and surviving-log checks, with gas charged
separately. No in-memory reload is a WAL or process recovery test.

The opaque route ID, terms hash and transfer IDs in these synthetic vectors are
placeholders. For a backend run they must be bound to independently reviewed
terms and actual emitted identifiers before appending facts, with a consistent
bijection if the manual vector IDs are relabelled. Canonical decimal strings
encode ABI uint values, enum kinds are numbers, and addresses/bytes are lower
case. No ABI/digest authenticity follows from the model's string comparison.

Every snapshot and comparison explicitly leaves source finality and production
qualification false. Neither deployed bridges, a launch, backing funds, pricing,
USDC compliance, an audit, nor activity on Nano has been demonstrated here.
