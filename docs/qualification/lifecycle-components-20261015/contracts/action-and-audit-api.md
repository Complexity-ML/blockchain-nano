# Fresh lifecycle tool: draft interface

SOURCE ONLY. No action, network, key access or test has been executed. The tool role is `lifecycleTool`, existing target `nano-capella-deposit-audit`. Legacy `--audit` remains separate and unchanged.

Both modes have argc 11:

```
--fresh-action MANIFEST CONFIG STOP REQUEST REQUEST_SHA256 DATA_ROOT WITNESS_ROOT OWNER NEW_OUTPUT
--fresh-audit  MANIFEST CONFIG STOP REQUEST REQUEST_SHA256 DATA_ROOT WITNESS_ROOT OWNER NEW_OUTPUT
```

REQUEST_SHA256 is lowercase SHA256 without `0x`, supplied independently before reading REQUEST. DATA_ROOT/chain and WITNESS_ROOT/chain-witness are independent private stopped physical copies, never live runtime paths. STOP includes the external primary `restart` token and separately retained `witnessCommitments.core` token from the prior authenticated stopped capture. Each request is specific to an exact authenticated HEAD, including its state root and slot. No key environment is read until both full closures, external tokens, domain, identity, credentials, eligibility, input fences and closed-copy inventories have passed.

The exact request has these keys:

```
format: nano-fresh-lifecycle-request-v1
owner: mac-native | mac-docker | vps-docker
manifestSha256, configurationSha256, stopSha256: lowercase SHA256
expectedHead: {root, stateRoot, slot}                 # canonical hex/quantity
identities: {
  voluntaryExit: {owner, keyEnv, publicKey},
  slashing: {owner, keyEnv, publicKey}
}
resources: {
  maximumWallSeconds, maximumHistoryBlocks, maximumArchiveBytes,
  maximumIndexBytes, maximumInventoryFiles, maximumInventoryBytes
}                                                   # canonical quantities
stage: observe | prepare_action | terminal
action: null | voluntary_exit | proposer_slashing
syntheticActionAuthorized: boolean
intentionallyAuthorConflictingHeaders: boolean
crossOwnerEvidenceTransferAuthorized: boolean
evidence: null | terminal-evidence
```

`observe` is a stopped RO registry probe, with no signer, journal or Reactor. It returns the two declared public identities resolved by actual public key, including absence (`resolvedIndex:null`). Null config indices never imply index 64. Activation, exit and withdrawal epochs/balances/credentials come from authenticated HEAD state, not a nonexistent validators RPC route. Observation alone does not claim replay, inclusion, duties, restart or payout.

`prepare_action` requires the explicit fresh synthetic authorization, one selected local configured key, two disjoint declared public identities and execution withdrawal credentials. Exit is signed only after actual activation plus the unchanged profile tenure. Slashing is an intentional conflicting-header fixture, separately authorized, on the other declared initial-registry key. It is not ordinary validator signing or evidence of two real proposals. Before output it is checked by the existing consensus operation validator. The action path creates exactly one exit signature or two header signatures and never opens a journal. It writes only NEW private `action.private.json` (0600) under NEW output (0700). The six-field network envelope is private. Stdout says `PREPARED`, not accepted or included, and exposes only fixed public commitments/counters. The orchestration resumes the stopped owner before local `nano_capellaSubmit`/`nano_capellaJob` calls; VM envelopes stay in situ.

`terminal` is a separate no-key/no-signature canonical streaming audit, **VPS in situ only**. The two full pairs of source/restored stopped captures remain on that owner. It does not claim three identical terminal replays. The other owners' stopped captures and common-finality comparison are separate observations made by the orchestration.

Terminal evidence is exactly `{requiredFinalized,deposit,actions,restarts}`. `deposit` contains `{transactionHash,rawTransaction,amountGwei,withdrawalCredentials}`. The current fresh exporter already produces a new signed native deposit using its fresh transaction sender; no third signing mode or old sender is reused here. `actions` has `voluntaryExit` and `slashing`, each `{action:Artifact,acceptedJob:Artifact,inclusionRoot}`. An Artifact is `{path,sha256,bytes}`. The job wrapper is `{format:'nano-fresh-lifecycle-accepted-job-v1',campaignId,owner,kind,actionSha256,session,sequence,job}` and the existing job has `{sequence,payloadDigest,cancelRequested,state,decision}`. The tool binds its content digest to the exact operation and requires completed/accepted, then separately finds the exact operation in the externally finalized canonical prefix.

`restarts` has exactly two `{cutoffSlot,beforeProposalRoot,afterProposalRoot,dutyPublicKey,sourceCapture,restoredCapture}` records. Complete capture artifacts must show unchanged source/restored core and signer snapshots/tokens, and the duty public key must occur in their local captured signer inventory. Restart one proves the newly registered validator's signed proposals before actions. Restart two uses an explicitly selected third, still active fresh key; the exited/slashed keys cannot stand in for healthy duty resumption. These records bind cryptographic evidence; the tool does not infer process restart execution from four block roots.

Cross-owner action and accepted-job artifacts are refused **before reading them** unless the externally SHA-pinned request explicitly carries `crossOwnerEvidenceTransferAuthorized:true`. This flag represents the separate human ROOT approval gate, must default false, and does not authorize a transfer by itself. The Native→VPS transfer of nonpublic artifacts remains open until that permission exists. Observe/action require false. No VM capsule, session, raw wire or filename is exported by this interface.

Recipient accounting replays the block's transactions independently on a bounded private execution state, checks their exact receipts, then adds every authenticated withdrawal and checks the final execution root. It records the recipient balance immediately before and after withdrawal credits, the selected credit, all credits to that recipient, and the distinct parent→post and other-execution deltas. Signed deltas are decimal strings; other integers use canonical quantities. Later withdrawals of new residual rewards remain valid; strictly increasing withdrawal indices prevent replay of an old credit. Consensus replay validates slashing penalties and rewards; the report never invents a fixed correlation debit or equates a raw validator balance difference with that debit. Standalone historical fork choice remains unqualified.

The initial trusted snapshot and profile/genesis files are bound by the fresh public manifest, not supplied by an untrusted database. The streamer uses native-vote/deposit-aware tick and fork-block admission and a bounded live state window. It refuses when no safe pruning point exists before its original limits. Neither the tool nor this proposal authorizes an actual campaign.

Phase 1 freeze boundary: managed-child RPC submission is NOT implemented in this component. The private supervisor token is delivered only to the P2P child (src/node.cpp); an external JS Submit/Job call cannot acquire it. The existing public test shim pattern is reserved for a separately reviewed and qualified next lot. Orchestration must refuse before runtime launch until that adapter is available. The action and audit modes, action policy test and bounded stream test remain independently compileable; no live lifecycle result is claimed.

V2 compilation correction: ROOT’s first ARM compilation of immutable Phase1 failed before tests because Int is GMP mpz_class. This successor changes exactly two signed-decimal conversions from convert_to<std::string>() to get_str(). The initial source/test failure history remains retained. No other implementation, test, policy, scope, budget or profile changes. The V2 author ran no compiler or test.
