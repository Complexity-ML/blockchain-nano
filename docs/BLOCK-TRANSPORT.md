# Représentation interne des blocs

Le format `signed-rlp-v1` ajoute `transactionEncoding` au JSON interne du bloc.
Son tableau `transactions` contient les octets signés de chaque transaction,
encodés en hexadécimal, au lieu de dupliquer le calldata dans `data` et `input`.
Les en-têtes, preuves du producteur et certificats BFT restent inchangés.
Le hash des transactions, leur racine et le hash du bloc ne dépendent pas de
cette représentation JSON. Les réponses Ethereum publiques conservent leur
représentation actuelle.

Les propositions BFT, les nouvelles propositions du journal de vote et les
blocs exportés par `nano_networkBlocks`/`nano_networkEvents` utilisent ce format.
Le lecteur accepte aussi les anciens blocs contenant des objets transaction.
Une version inconnue, un mélange d'objets et d'octets ou des octets signés non
canoniques sont refusés. Un tableau de simples hashes reste insuffisant pour
importer un bloc.

Un ancien journal n'est pas converti silencieusement : la réservation vérifie
la représentation développée exacte du même bloc et conserve les octets déjà
enregistrés. Le test `block_transport_codec` reprend une réservation au format
précédent, vérifie la signature identique, finalise le bloc et audite l'historique.
Les tests des types signés 0 à 4 portent sur le codec ; ils n'activent pas les
types 3 et 4 sur le réseau Nano.

La pagination vise 8 Mio et au plus 50 entrées. Un premier bloc ou événement
plus gros peut occuper seul une page, dans une enveloppe de 16 Mio avec une
marge de 1024 octets. Un objet dépassant cette enveloppe produit une erreur
explicite au lieu d'une page vide qui empêcherait le rattrapage. Le client RPC
natif accepte 16 Mio, comme le client du relais. Le journal de vote garde sa
limite de 16 Mio pour les anciennes valeurs monolithiques ; les nouvelles
propositions plus grandes utilisent le stockage fragmenté décrit ci-dessous.
Le plafond WebSocket de 8 Mio n'est pas modifié ; les relais
utilisent HTTP pour leurs échanges avec le cœur.

Les anciens exécutables qui ne décodent que des objets transaction ne peuvent
pas consommer des transactions `signed-rlp-v1`. Cette étape ne qualifie pas une
mise à jour avec plusieurs versions de cœur coexistantes. Aucun déploiement
public ou changement automatique de base existante n'est effectué.

Les cas locaux de 9 et 18 Mo dans l'ancienne représentation passent désormais
le vote, l'export, l'annonce et l'import audité. Cela ne ferme pas la question
des blocs dépassant 16 Mio après encodage compact. Les qualifications réseau
et le traitement des échanges BFT cumulés sont décrits ci-dessous ; ils ne
constituent pas une preuve de préparation production globale.

## Local libp2p large-block qualification (2026-09-24)

`bft_large_block_paris_network` and `bft_large_block_shanghai_network` pass with four real core processes and four libp2p relays. Five signed transactions with 900,000 zero calldata bytes each are admitted to three peers before transport connection. The fourth peer stays isolated and has neither the transactions nor finality. The majority certifies all five in one block with three independently verified signatures. After reconnection the isolated validator imports the history through libp2p. The 9,004,650-byte compact RPC page exceeds the former 8 MiB limit. All four peers agree on receipts, block hash, state root and exactly-once transferred balance; all four persisted histories pass audit after shutdown and reopening.

Evidence: `build-cmake/qualification/large-block-network-20260924`. Both profiles pass (48.64 seconds total). This is one bounded local campaign, not multi-host endurance or a new full-suite result. It does not qualify compact blocks above 16 MiB, simultaneous large current/previous BFT proposals, mixed-version nodes, or post-recovery quorum participation requiring the returnee.

## Single-round BFT pages

The retained-lock round-transition regression reproduces a transport failure on Paris and Shanghai: the aggregate snapshot is 18,009,077 bytes, above the 16,777,216-byte frame ceiling, although each proposal is individually valid. Its failed baseline remains in `build-cmake/qualification/large-bft-envelope-20260924`.

`nano_bftExchangePage` accepts the same optional inbound envelope as `nano_bftExchange`, followed by an optional boolean page selector. False returns the current round; true returns the retained previous round, falling back to current when none exists. Every page is a complete version-1 envelope with `previous: null`; votes and proposal authentication remain unchanged. The original aggregate diagnostic endpoint is retained and may exceed transport limits.

Relays use the paged endpoint for requests and replies. Local polling alternates page selectors, while each peer independently remembers its next page so that a slow exchange cannot repeatedly skip the same round. There remains at most one BFT exchange in flight per peer. Previous-round messages are validated with their original height and round; pagination neither authorizes stale votes nor removes the retained lock.

This separates two distinct proposals as well as two copies of a locked proposal; it does not fragment a single oversized proposal. A single proposal plus its votes can still exceed 16 MiB. Mixed core/relay versions and end-to-end delayed large-round transitions require further qualification.

## Single oversized proposal baseline

A 40-million-gas genesis with ten admitted 900,000-byte zero-calldata transactions produces a valid 36,210,000-gas block whose compact JSON is 18,003,843 bytes. Both Paris/Shanghai tests fail at the journal, export page, event page and JSON frame boundaries. Canonical storage accepts and audits it only after import with a synthetic externally supplied certificate. See `oversized-block-20260924`; this does not qualify local finality or default-genesis block sizes.

The next transport design must retain bounded per-message work while identifying immutable proposal/block content, enforcing fragment offsets and total sizes, rejecting mixed or corrupted assemblies, and bounding aggregate in-flight memory per peer and globally. No partial assembly may authorize a vote, publish a block or advance an event cursor. Proposal persistence must be atomic with its vote reservation and readable after restart. Recovery must retain existing journals. Qualification must cover interruption, duplicate/reordered fragments, bad content, size and timeout exhaustion, reopened state, finality and lagging catch-up through real libp2p. This list describes the original gaps; the following sections record the implemented subsets and their remaining limits.

## Fragmented voting-proposal storage

A proposal whose compact JSON exceeds 16 MiB is now stored as 1 MiB values under `<proposal-key>/chunks/<index>`. The proposal key holds the version tag `nano/proposal/chunks-v1\n` and a bounded manifest containing total bytes, chunk count and Keccak digest. The manifest, every chunk, reserved vote, lock and watermark are written in one synchronous RocksDB WriteBatch. Smaller proposals retain their existing inline representation. Existing compact and expanded inline proposal values are accepted and preserved byte-for-byte.

Recovery validates manifest types, version, arithmetic bounds, exact chunk sizes and the complete digest before parsing or signing. It never reserves memory from an untrusted manifest length. A missing fragment/manifest, modified bytes, truncated tail, invalid count/digest, unsupported version or impossible total size prevents resumed signing. This does not bound total reconstruction memory: valid block serialization, RocksDB batching and recovery still hold the complete proposal.

The reserved signature is reproduced exactly after reopening, and the reconstructed large block reaches local BFT finality and canonical audit. The standalone large-block lifecycle tests now pass the journal stage without an external fixture certificate, but remain red at export, event and libp2p framing. This journal milestone implements fragmented **storage**. Certified-history network transfer is covered by the later sections; BFT proposal transfer remains outstanding. Older executables cannot read this new manifest and mixed-version upgrade operation remains unqualified.

## Read-only fragment export RPC

`nano_networkBlockManifest([blockHash])` returns `{version: 1, block: ...}`. The block carries the authenticated header, producer proof, uncle headers, certificate, and ordered transaction hashes with `transactionEncoding: "signed-rlp-hashes-v1"`. This marker is deliberately not accepted by `Block::from_json`: a manifest is not an executable block. The manifest response is limited to 8 MiB; larger transaction inventories still require pagination.

`nano_networkTransactionFragment([blockHash, transactionHash, offset])` reads the canonical transaction record and checks its stored block membership and signed-byte hash. It returns version, both hashes, the byte offset, total signed-byte length, the next offset (or null), and at most 256 KiB of signed transaction bytes encoded as hexadecimal. Offsets must be aligned to 256 KiB and lie within the transaction. Each response therefore stays under 512 KiB plus small metadata. The server retains no partial assembly or transfer session; repeated requests produce the same bytes while the referenced canonical records remain available.

A receiver must collect the exact signed bytes, verify every transaction hash, preserve manifest order, validate the transaction root and authenticated block identity, and pass the complete block and certificate through ordinary validation. The new RPCs only export data: they do not reserve a vote, import an incomplete block, change finality, or authorize minting. Transaction records are read individually, avoiding a full-block read for every fragment.

Managed relays now use the fragmented catch-up path described below; standalone relays without a local token retain the aggregate block path. Block event references and BFT proposal fragments remain to be implemented and qualified. The original export-only test submits its reconstruction in process; the private HTTP ingress described below now provides a separate fragmented receiving path.

## Private fragmented ingress into the core

Four RPC methods require the existing local relay token as their first parameter. With an empty configured token they are disabled. `nano_beginBlockImport([token, manifest])` checks that the header identifies the next block over the current canonical parent and creates an opaque session id. `nano_appendBlockImport([token, id, transactionIndex, fragment])` accepts one exported fragment. `nano_finishBlockImport([token, id])` consumes a complete assembly and calls the ordinary block submission validation. `nano_abortBlockImport([token, id])` discards it. Managed `nano run` already generates and passes this token to its local relay; the catch-up forwarding path is described below.

There are at most four sessions and 64 MiB of retained manifest/raw-byte payload across them. This is local admission policy, not a new consensus block-size rule. It is not a process RSS bound: container overhead, JSON decoding, hexadecimal reconstruction and execution allocate additional memory. Sessions have an absolute 60-second lifetime, checked on every import operation and the periodic RPC tick. Duplicate fragments do not renew that lifetime or charge bytes twice. An aborted, expired, completed or hash-corrupt assembly releases its payload budget. Capacity refusal leaves existing accepted data available for retry. No staging survives a process restart; a relay must begin again.

Fragments must match the manifest block/transaction identity, exact transaction order, aligned offsets, fixed total and next-offset semantics. A repeated accepted range is acknowledged only when byte-identical. Gaps, conflicting duplicates and oversized input are refused. Each reconstructed transaction must match its signed-byte hash. A bad final digest discards the entire session. A complete payload does not itself commit: explicit finish still verifies transaction roots, producer authentication, execution and the BFT certificate through `validator_submitBlock`.

Paris/Shanghai HTTP tests transfer the 18 MB compact fixture block between two real local HTTP servers, verify that no partial/complete staging commits before finish, reject an otherwise complete block without its BFT certificate, finalize the valid one and audit again after reopening its database. Unit tests exercise aggregate byte exhaustion/reuse, duplicate idempotence, expiry, abort, corruption cleanup and incomplete completion. Libp2p forwarding, proposal fragments, oversized manifests, sustained transfer-load qualification and power-loss behavior remain outside this evidence.

An assembly whose retained manifest and signed bytes exceed 64 MiB is refused by the current local admission budget. Such a block may still be valid under consensus rules; supporting it requires a further bounded streaming/staging design. This milestone does not close the general oversized-block readiness gate.

## Managed libp2p catch-up by fragments

Managed relays with a local core token now request `/nano/blockparts/1.0.0` when a peer advertises a higher canonical height. A height request returns `nano_networkBlockManifestAt`; subsequent requests carry only the block hash, transaction hash and offset. The relay forwards each response to the private core assembler and retains only the manifest and current response. The local token is never sent to a peer. Standalone relays without that token retain their previous aggregate synchronization path.

Only one download is active per relay. Fragment requests are sequential and paced at 350 ms to share the existing per-peer bulk admission budget with normal inventory traffic. A transfer has a 55-second deadline; failure aborts local staging, clears the sync flag, and applies a one-second peer cooldown. Updating an existing handshake preserves its active sync and BFT-page state. After finish, the relay queries its local canonical block hash, so a missing finish reply need not imply that an already committed block must be imported again. The lost-reply path and an interrupted fragment transfer are now qualified by the local fault campaign below.

This connects certified-history catch-up, not oversized BFT proposal distribution. At that milestone, block announcements still used the aggregate event route; the reference route below replaces it for managed relays. The local 64 MiB staging budget, 60-second assembler lifetime, 55-second relay deadline, and 8 MiB manifest ceiling can still exclude larger or slower valid transfers. Disk-backed/streaming assembly, concurrency/endurance, adversarial peers and mixed-version operation remain to be qualified.


## Interrupted transfer and lost commit acknowledgement

The Paris/Shanghai `libp2p_fragment_faults` scenarios use two managed nodes and an 18,003,843-byte compact certified block. The source resets the real libp2p stream before replying to the second 256 KiB fragment. The follower records exactly one failed attempt after accepting one fragment, then downloads all forty fragments again. After successful validation and commit, its HTTP server resets the real local TCP connection before sending the finish response. The relay observes a missing reply, verifies the canonical hash and reports exactly one successful import.

Each scenario compares all ten receipts, the block hash and state root, checks that the recipient received exactly ten units, stops/reaps both nodes and audits both reopened databases. Five suites pass in 77.55 seconds including RPC, HTTP and supervisor regressions. Evidence: `libp2p-fragment-faults-20260924`. Fault hooks require explicit test environment variables; the HTTP hook additionally requires a loopback listener and consumes only a successful private import.

This proves local certified-history recovery for those two connection resets. It does not prove recovery from process death during assembly, distributed oversized proposal finality, multihost partitions, adversarial concurrency or sustained load.


## Unfinalized BFT proposal export

`nano_bftManifest([previous])` returns a single current/previous round envelope with ordered signed-transaction hashes instead of transaction bodies (`signed-rlp-hashes-v1`). The boolean selector is optional; the manifest is capped at 8 MiB. An inactive reactor returns null. This is read-only and does not receive votes or tick consensus. The ordinary BFT receiver rejects this incomplete proposal encoding.

`nano_bftTransactionFragment([blockHash, transactionHash, offset])` serves up to 256 KiB of signed bytes from a proposal retained by the active reactor. The response uses the same version/identity/offset/total/next/data shape as certified-block fragments. It rejects unknown proposal/transaction identities, unaligned/out-of-range offsets and proposals from a height already committed. Unlike certified history, this source is transient: current/previous round retention is not a durable transfer lease. Fetchers must handle a proposal disappearing while downloading.

The previous round retains a structured Block separately from its vote metadata; legacy full snapshots are reconstructed with unchanged contents. Previous-round replacement occurs only after the round transition succeeds. Per-fragment lookup currently scans transaction hashes; response-size bounds are not CPU or process-memory bounds.

Five final native suites pass on Paris/Shanghai in 30.01 seconds. Each oversized profile reconstructs both round envelopes using eighty total fragments, preserves every proposal field and vote, rejects seven incomplete/invalid/stale cases, and finalizes/audits the locked proposal in the next simulated round. RPC dispatch is checked for exact export and invalid arguments. Sources, binaries and all three test phases are retained in `bft-proposal-fragment-export-20260924`; final input hashes identify the final build.

This export-only milestone did not connect live BFT transport. The private ingress and local libp2p integration described below cover subsequent subsets; broader distributed finality remains unqualified.


## Private BFT proposal assembly

`nano_beginBftImport([localToken, envelope])` stages a proposal manifest for the live next height and current round. The initial checks cover envelope/header/proof encoding, parent linkage, vote sizes and reactor availability. Nano producer signatures cover the full signed transactions as well as the header; cryptographic authentication therefore remains at complete-proposal reception. No partial manifest is authenticated as a complete block.

The existing `nano_appendBlockImport` and `nano_abortBlockImport` carry or discard the session bytes. `nano_finishBftImport([localToken, id])` consumes a complete proposal session and passes its reconstructed envelope through ordinary `BftReactor::receive`, then tick. This rechecks the live height/round, producer signature, execution, roots and votes. Its bounded `received` response acknowledges processing; it does not promise finality or adoption of a competing proposal. Only ordinary quorum logic can commit.

Block/proposal sessions share the existing four slots, 64 MiB retained-payload budget and absolute 60-second lifetime. Envelope metadata is charged too. Their finish operations reject the wrong session purpose without consuming it. Both new RPCs require the configured local relay token and active BFT reactor; empty tokens disable them. Completed sessions are consumed even if later consensus validation rejects their contents.

Two final native RPC scenarios pass on Paris/Shanghai (32.76 seconds), alongside six same-core regressions including HTTP certified-block imports. Each forwards forty fragments to three assemblies: valid, correctly signed with false state root, and valid but completed after canonical commit. Staging produces no proposal/vote; invalid execution cannot produce a vote; the valid proposal obtains a local prevote, commits only after real signed quorum votes, and audits after reopening. The late finish and twelve invalid requests per profile are rejected. These are in-process BFT RPC tests, not live BFT HTTP/libp2p transfer tests.

The first implementation incorrectly tried to authenticate a header with an empty transaction body and failed both ingress profiles. After moving authentication to ordinary complete reception, both fixtures failed an address-case comparison; normalizing the decoded/checksummed validator addresses fixed the fixture without changing signature checks. All failures and source phases are retained in `bft-proposal-ingress-20260924`. The subsequent local libp2p campaign below covers one live transfer/round scenario; broader retention, fault and timing qualification remains open.


## Local libp2p BFT manifest and fragment exchange

Relays with a local core token now exchange `/nano/bft/2.0.0` manifests and votes through `nano_bftExchangeManifest`. The receiver validates the envelope context and processes signed votes separately from the proposal body. Votes alone cannot authorize a block signature: the existing coordinator still requires a fully verified matching proposal. Missing bodies are fetched through `/nano/bftparts/1.0.0` and staged via private BFT ingress. Standalone relays without a token retain the old full-page path; mixed-version negotiation is not qualified.

Each relay permits one proposal download alongside its existing single certified-block download. Both share the core assembly budget. Fragments retain 256 KiB size, 350 ms pacing, the existing bulk admission limits and a 55-second transfer deadline. Votes/manifests retain the control traffic budget. After finish, the downloader checks canonical history or the retained current/previous proposal hash. This checks processing, not a promise of finality. The local token never crosses libp2p.

The first live Paris scenario failed after 100 seconds without finality. Eight partial transfers stopped after 12–28 fragments while round progression discarded their source/context. The emitted admission traces showed no budget refusal. The reactor's proposal wait now increases by round: 5, 10, 20, 40, 80, 160 seconds, then a 300-second cap. The same rule applies to recovered rounds. Prevote/precommit waits remain five seconds; quorum, signature domains, block validity and journal safety checks are unchanged. The coordinator rejects invalid retry durations before replacing round state. This bounded backoff is not a guarantee under arbitrarily delayed networks.

The corrected Paris/Shanghai scenarios each certify an 18,003,850-byte compact proposal at round two, with all ten transactions in the same block, matching receipts and exactly-once balances. The strict final replay requires forty fragments at each of at least two distinct receiving validators and audits four reopened databases per profile. It passes both profiles in 157.46 seconds combined. Six other same-core regression suites pass; the initial failure remains indexed. A verifier rejects traces containing only later empty proposals or repeated receiver identities.

Evidence: `bft-fragment-network-20260924`. Four cores run inside one test process, with four actual HTTP listeners and four separate libp2p processes on loopback. Four independent core processes, multi-host partitions, BFT-specific lost acknowledgements/crashes, larger/slower valid proposals, repeated proposer cycles and block-event references remain open. Existing certified-block connection-reset tests also pass on this integrated relay.


## Bounded block announcement references

Managed relays poll `nano_networkEventRefs([cursor, epoch])` and publish `/nano/blockannounce/2.0.0` hints containing exactly version 1, a canonical block hash and a canonical uint64 height. Each event page contains at most fifty block references, without loading transaction bodies. The cursor advances across pending events as well. An expired event history, changed epoch or future cursor explicitly resets to the current canonical tip captured under the same chain lock. Reopening the database changes the epoch.

A relay checks the complete page before advancing its cursor; an unsuccessful local RPC leaves the cursor unchanged. A received hint only requests ordinary peer-status reconciliation. Canonical import still requires full assembly, authentication, execution and certificate validation. Periodic status synchronization remains the recovery mechanism for missed announcements; these are best-effort hints, not an acknowledged delivery log. Standalone relays without the local token keep the legacy aggregate event/announcement path. Mixed-version interoperability remains unqualified.

The local Paris/Shanghai scenarios each announce and receive the actual 18,003,843-byte compact block, import its forty fragments, compare ten receipts and audit both reopened databases. Fifty-two blocks paginate as fifty then two references; the maximum observed page is 6,630 bytes. Seven malformed references are refused. Expiring the event ring with 10,000 native submissions across 100 accounts explicitly resets to the canonical tip. No network runs during that event-ring phase. These scenarios have two chain instances in one test process and two real libp2p child processes. Periodic synchronization can also initiate the transfer; this is not proof that the notification was its sole trigger.

Evidence: `block-event-references-20260924`. Four initial regression suites pass; the corrected eleven-suite phase passes in 158.74 seconds, including nine estimation/queue/full-pool suites. Fifteen distinct suites pass across these phases on the same core and relay binaries. The initial new fixture failures remain preserved. Broader multi-host, mixed-version, loss/endurance, and production qualification remain open.


## Independent core process fixture

`bft_fragment_processes_paris` and `bft_fragment_processes_shanghai` run the same oversized-proposal scenario with four separately spawned core processes, each linked to `nano_core`, plus four real libp2p relay processes. Each core has its own HTTP listener, state database and signing journal. The fixture verifies four distinct process identifiers and clean shutdown before reopening storage. Child cores preload identical signed transactions before enabling their BFT timers; they are fixture executables rather than the production `nano run` supervisor. The original single-process scenario remains separately registered. This separation exercises process-local synchronization and memory isolation, not different physical hosts or operating systems.

Paris and Shanghai both pass (81.54 and 78.89 seconds including fixture setup/shutdown). Both certify an 18,003,850-byte compact block at round two; three Paris receivers and two Shanghai receivers download all forty fragments of that exact block. Four/five failed intermediate downloads remain recorded. Ten receipts agree across the four peers in each profile, transfers apply exactly once and all eight reopened databases audit successfully. Twelve captured input fingerprints remain unchanged during the run. CMake regenerated/recompiled KZG and relinked the core archive during setup, so this campaign does not claim byte identity with the preceding archive. The relay binary and compared runtime sources are unchanged.

Evidence: `bft-fragment-processes-20260924`. The verifier rejects duplicate/missing core process identities and duplicate target receiver identities. This closes the shared-process limitation for this one large-proposal lifecycle on loopback. It does not qualify multi-host timing, deployment through the production supervisor, extended proposer cycles, partitions, process crashes during proposal assembly, or larger/slower transfers.
