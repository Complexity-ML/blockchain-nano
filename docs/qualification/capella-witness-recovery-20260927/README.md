# Capella separate-witness recovery — 2026-09-27

Base revision `6e8f2b9`, plus the accompanying source diff.

## Implementation

Optional independently trusted ChainStore witness for a bound Capella execution
owner. Before each primary checkpoint write, synchronously record an intent:
previous token, next sequence/digest, complete durable snapshot and the new signed
block archive entry (if any). The witness has its own chain/domain binding and CAS
token. Any publication failure closes the coordinator and owner execution reads.

On restart, the caller supplies the trusted witness, not a guessed primary token.
Only the exact previous or next primary token is accepted. The next outcome is
verified against execution state, receipts, finalized ancestry and the intended
archive bytes. The previous outcome is rolled forward by replaying the exact
snapshot and archive in one canonical publication. The signed archive is checked
against SSZ header/body, proposer signature, root and slot before replay. Empty,
malformed and older-than-previous recovery states are rejected.

## Qualification

`consensus_capella_execution_owner_witness` injects a primary WAL fsync error and
exits the child without destructors. Both retained and truncated primary WAL suffixes
recover to the exact intended next state, including the signed block archive and
transaction indexes. Owner state and RPC reopen successfully. A third case injects
the error into the witness WAL: the primary stays at its previous checkpoint,
owner access closes, and the retained witness intent completes the publication on
restart. Empty witness, corrupted BLS signature in its archive, and older primary
rollback are rejected without changing the primary checkpoint. Repeated restart
preserves the primary token.

## Trust and limits

The witness is explicitly trusted local storage, not a remote consensus certificate.
Separate directories on one disk do not protect against joint rollback or disk loss.
The tests inject I/O failures and controlled suffix truncation, not physical power
loss. Loss of the witness WAL suffix itself is not qualified here. Runtime wiring,
operational placement/backups and signed-journal recovery must preserve this trust
boundary. No public activation, P2P or mixed-fork Store migration is included.

## Results

Five suites passed in 62.04 seconds: witness recovery, bound-owner sync failure,
owner reorg, owner finality/restart and signed transitions. Logs retain the three
witness outcomes and negative-control markers.
