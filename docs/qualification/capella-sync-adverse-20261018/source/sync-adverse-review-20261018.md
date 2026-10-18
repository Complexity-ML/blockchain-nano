# Revue source sync/reconnexion — 18 octobre 2026

Lecture publique seule de main `4ae774f16220d8a9362004b573d83f3c8ed78a97`
(HEAD vérifié). Aucun test, import de module projet, compilation, SDK, autorité
privée, réseau, duty ou campagne04 exécuté. Cette note ne qualifie aucun runtime.
Local couvre séparément runtime Altair et reactor Capella.

## Constats établis

* `network/altair_rpc_service.hpp` est header-only : status50–57,
  receive59–76, submit78–80, block82–87, inventory89–97. Ce service n'expose
  ni tick distant ni fonction de synchronisation arbitraire. Le mode réseau
  interdit receive synchrone60 ; submit passe par le worker et les reçus.
* Le décodeur vérifie chaîne/genesis/fork et schéma exact avant allocation SSZ
  (`network/altair_message.hpp`49–72). Ce filtre de compatibilité ne remplace
  pas la validation du bloc ni l'ancre de confiance.
* Les entrées inconnues exigent parent présent, slot non futur et strictement
  après la finalité, puis ascendance vers la racine finalisée
  (`src/consensus_altair_fork_store.cpp`24–29). La finalité n'est remplacée que
  par une époque supérieure49. La préparation copie le store avant mutation.
  Ce chemin ne montre pas de recul de finalité ou de mutation publiée sur rejet.
* Les jobs ET payloads en cours/en attente sont bornés : compteur commun,
  capacité allouée comptée, réservation conservée pour deferred
  (`network/altair_worker.hpp`42–61,74–81). Le ledger est borné ; rotation ne
  supprime pas ses jobs et annule les anciens, séquences retirées non rejouables
  (`network/altair_receipts.hpp`29–55). Aucune file illimitée établie ici.
* Les pages de relay sont bornées à32 votes/un bloc, connaissances par session
  bornées/évictables et deadline ; les ACK perdus n'avancent pas le curseur
  (`network/altair_archive_relay.hpp`8–27,47–64,85–102). L'inventaire RPC borne
  sa réponse ; son catalogue est néanmoins reconstruit/copied à l'échelle de
  l'archive sous mutex (rpc_service25–37), sans checkpoint d'annulation : coût
  O(N) à mesurer, pas une file infinie ni une exploitation démontrée.

## Particularité causale : doublons

Un root déjà connu retourne l'input AVANT vérification de signature
(`consensus_altair_fork_store.cpp`23). `AltairDurableReactor::block` conserve
l'archive originale mais appelle publish même dans ce cas
(`src/consensus_altair_reactor.cpp`228–238). Le commit de checkpoint incrémente
toujours sa séquence (`src/storage.cpp`565–581). Un doublon peut donc provoquer
une nouvelle écriture/engagement ; « idempotent » ne doit pas signifier token
inchangé. `tests/test_altair_archive.cpp`41–42 accepte expressément une signature
altérée sur un root connu et vérifie que les octets archivés ne sont pas écrasés.
Ce constat n'établit ni admission d'un root invalide inconnu, ni double signature,
ni recul de finalité. La préparation received reconsidère aussi les attestations
embarquées124–145 : comparer les snapshots et le contexte temporel, pas seulement
les compteurs. Aucun correctif n'est justifié sans oracle/scénario causal.

## Couverture source existante, sans nouveau PASS revendiqué

* `test_altair_wire.cpp`113–129 : enveloppes/SSZ invalides, mauvais profil,
  token inchangé au rejet, blocs officiels durables ;130–149 : budget par pair,
  erreur locale, service arrêté sans bloquer le RPC ordinaire.
* `test_altair_relay_progress.cpp`44–68 : rejet différé, ACK perdu, ordre des
  blocs ;69–107 : budget de scan, doublons/oversized, cache/pruning ;108–127 :
  connaissance isolée par admission/session. CTest
  `consensus_relay_rejected_vote_progress` (CMake1346–1349).
* `test_altair_restart_p2p.cpp`45–67 : deux blocs, deux SIGKILL du core,
  checkpoint externe conservé, nouvelle session gossip, rattrapage et octets.
  Ce test requiert un daemon ; ce n'est pas un scénario sans réseau.
* `test_altair_archive.cpp`24–34 et `test_altair_reactor.cpp`19–35 couvrent
  annulation/écriture incertaine/réouverture ; receipts16–59 couvre saturation,
  retraite, réponse perdue et course rotate/submit.
* Aucun sélecteur littéral `service_sync`, `service_malformed`, `service_adverse`
  trouvé dans les sources C++ des tests/CMake lus. Ne pas inventer ces commandes.

## Plus petit nouveau lot significatif

Test C++ séparé, fixtures officielles publiques déjà signées, transport injecté
sans daemon/clé/duty : enfant avant parent (rejet sans changement), parent puis
retransmission de l'enfant (même head/archive que parcours canonique) ; root
inconnu de signature invalide/refus de slot ou d'ancre finalisée ; mélange borné
de doublons connus et messages invalides, annulation puis réouverture.
Mesurer séparément head, finalité, snapshot/hash, archive SSZ, token/écritures,
réservations et temps. Pour les doublons connus, utiliser la sémantique actuelle
ci-dessus, sans imposer artificiellement une séquence inchangée.

Ajouter un cas catalogue local réel : append tardif, page retenue puis pruning,
curseur obsolète, réponse bornée et reprise sans entrée perdue. Les tests relay
actuels simulent les réponses ; ils ne composent pas ce cas avec le vrai reactor.
Qualifie seulement cette API/ces transitions, pas WAN A, partition prolongée,
finalité multiowner, restauration des duties ni cycle complet. Tentative03 reste
FAILED consommée ; aucune04 ni production déduite.

Pins SHA256 de référence : rpc_service9498B
`f663c95b04d8b02371ddf0be56747a7eb7826bd29161575fc088b0f5d51fdcf1`,
worker4042B `3b038a561d9085ba678efd32fe677ba06966907dd3fc3ca4ac565c109d541204`,
relay9472B `703f5584dec8c685d1df391fbbf39e1d082adaef9075f65857a947d927d8c292`.
