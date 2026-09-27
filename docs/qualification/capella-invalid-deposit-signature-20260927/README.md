# Wrong-key native deposit signature — 2026-09-27

## Scenario

A signed execution transaction pays the configured full activation stake (32
native units in this fixture) to the real native deposit contract for a new BLS
public key. A positive control verifies the original deposit signature. It is
then replaced with a well-formed signature over the same message by another key,
and the deposit data root is recomputed before submitting the transaction.
Thus the execution contract, deposit event/history and Merkle proof are valid;
the proof of possession alone is invalid. The wrong-key control must fail BLS
verification before the integrated run.

After pruning and reopening one owner, the two native reactors continue voting
until the deposit is consumed. The cursor must advance once, but the registry
must stay at 64 and exclude the requested new index. At every continuation slot,
both reactors must refuse signing for the unregistered key, without calling its
signer or changing the checkpoint. Both Beacon and EVM states must remain equal.
Replaying blocks is idempotent; two database reopens preserve the consumed cursor
and the unchanged registry. There are no public funds or WAN traffic.

The expected cursor/admission distinction follows [Ethereum Phase0 deposit
processing](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#process_deposit):
a valid deposit proof advances the cursor; a new key with an invalid signature
is not added to the validator registry. Native deposit selection is not itself
Ethereum EIP-6110 conformance.

## Scope and limits

Only the new integrated adversarial case and the official deposit-transition
fixture runner are executed in this tranche. The immediately preceding top-up
and valid activation evidence remains separately recorded; the production
transition code is unchanged. This is one wrong-key signature on a new key,
not all malformed encodings, invalid signatures for existing-key top-ups,
subgroup attacks, malicious inbound votes, reorgs or crash-at-consumption cases.
It does not prove production readiness or protect a validator whose private key
has actually been stolen.

## Result

Both tests passed in 87.60 s combined. The integrated case took 87.21 s:
consumption once at slot 80, continuation through slot 89, finality epoch 9,
unchanged 64-entry registry, no signer invocation for index 64 and two database
reopens. The official runner took 0.38 s (15 valid fixtures, two invalid cases;
six admissions, five top-ups, four ignored invalid signatures, plus its recorded
mutation/overflow checks). These counts describe that runner, not global EEST.
Source/binary hashes were verified unchanged afterward. Logs normalize trailing
whitespace only; the build retains an existing macOS helper deprecation warning.
