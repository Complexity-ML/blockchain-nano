# Sortie admise, réorganisation et pruning — 17 octobre 2026

**La perte est reproduite sur l’ancien Core, puis les deux nouveaux scénarios passent dans trois environnements : Native macOS ARM64, Docker Linux ARM64 et AMD64.** Les deux régressions existantes passent Native seulement. Les exécutables/images runtime et l’index 24 ne sont pas actualisés par ce lot ; la tentative 03 est autorisée mais inactive, aucune production qualifiée.

## Résultats effectivement exécutés

| Lot et reçu public | Résultat | Construction ciblée |
| --- | --- | --- |
| [D07 Native](evidence/D07/result-public.json) | `LOST`, test 1 attendu, 14,270 s | Un test compilé/lié contre l’ancien Core 7052b226… ; aucun Core reconstruit. |
| [D10 Native](evidence/D10/result-public.json) | Conservation 14,602 s ; différé 15,001 s ; deux PASS | Builds D09 repris, zéro CXX nouveau, un lien. |
| [D13 Native](evidence/D13/result-public.json) | `--queued-exits` 3,269 s ; `--exit-pending-pruning` 17,548 s ; deux PASS | TU D12 repris, zéro CXX, un lien, M/MD concordants. |
| [ARM02 hôte](evidence/ARM02/result-public.json), [worker](evidence/ARM02/outputs/result-public.json) | Conservation 9,223 s ; différé 9,502 s ; deux PASS | 2 M/2 CXX (reactor 5,540 s, test 4,999 s), un ar/un lien, zéro image. |
| [AMD01 hôte](evidence/AMD01/result-public.json), [worker](evidence/AMD01/outputs/result-public.json) | Conservation 36,090 s ; différé 36,808 s ; deux PASS | 2 M/2 CXX (reactor 22,235 s, test 20,431 s), un ar/un lien, zéro image. |

D07 importe 16 blocs A après admission sur B (2 blocs) : sortie d’époque 4, nouvelle finalité 2 et tête 4, même clé, aucune inclusion, zéro vote retenu. Le vrai pruning retire B sans changer la tête ; la file passe 1→0 ([stdout](evidence/D07/causal.stdout.log), [assertion](evidence/D07/causal.stderr.log)). Pic RSS 81472 KiB et 287 signatures synthétiques. Le retour 1 prouve le défaut, pas un PASS produit.

Après correction, `--exit-reorg-prune-self-test` conserve les mêmes octets 1→1, puis vérifie réouverture primaire/témoin, doublon sans nouveau token et réinclusion normale ; l’archive reste présente avant finalisation. `--exit-reorg-prune-deferred-self-test` admet une sortie d’époque 5 alors que tous les états retenus restent antérieurs : pruning manuel différé sans mutation, tick automatique publié sans pruning et réouverture exacte. Cette sortie n’est pas présentée comme déjà incluse dans la tête. Les [sorties ARM](evidence/ARM02/outputs/causal.stdout.log) et [AMD](evidence/AMD01/outputs/causal.stdout.log) confirment les mêmes oracles ; le cas conservation utilise 289 signatures, le différé 287. Profil synthétique à tenure 1, limites 180 s/128 MiB/1024 signatures, sans modifier les règles de production.

Les régressions Native [queued](evidence/D13/queued.stdout.log) et [pending-pruning](evidence/D13/pending-pruning.stdout.log) couvrent admission/refus/doublon/capacité, reorg/réinclusion et réancrage admissible avec continuation identique après reopen. Elles n’ont pas été exécutées sous Linux (`transitionSkipped:true`). « Lot12 » était le numéro du lot préparatoire, pas douze régressions ; les propositions V1–V3 restent intactes.

## Sources, archives et contrôles

Sources corrigées : reactor 38649222… et test a5c6c42c…, depuis la baseline 00154f57…. AMD enregistre la révision 1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352 et réemploie le même worker db3f32a2 qu’ARM. Aucune opération Git n’est effectuée par l’auteur.

Les preuves [Native](evidence/D10/archive-one-member-proof.json), [ARM](evidence/ARM02/outputs/core-member-proof.json) et [AMD](evidence/AMD01/outputs/core-member-proof.json) comparent 143 membres objets ordonnés, dont 142 inchangés ; seul `consensus_capella_reactor.cpp.o` est remplacé. Nouveaux Core : Native 3401bf77…/29252992 B, ARM 36f77c8b…/41045640 B, AMD a3fdf87a71e05264058fb8bbb7894082330ef6042dd23f68007cf9d782ae2c48/40811064 B. Ces fonctions ne vérifient pas elles-mêmes les neuf champs du fichier archive ni la sémantique des tables de symboles.

Les reçus hôte attestent séparément les originaux strictement inchangés et cleanup réussi, conteneurs possédés retirés, sans purge d’images/volumes. Les 583 entrées guest ARM et 581 AMD sont suivies par stat/SHA/résolution ; `guestFdIdentityVerified:false` et l’autorité hôte externe au worker restent explicites. Pas de configuration CMake globale. D13 réutilise le template CMake et la bibliothèque de support ancienne sous autorité retenue, selon Root ; aucune recompilation de cette bibliothèque n’est revendiquée.

## Historique et limites conservés

01 refuse la matérialité de copie avant CXX ; 02 échoue sur types sync/API RocksDB ; 03 dépasse le plafond RSS 134512>131072 KiB avant oracle, avec refus final distinct du ctime du nouvel exécutable. 04–06 compilent/lient mais refusent sans observation causale, cause fine non publiée. 07 reproduit `LOST`. 08 refuse un header relatif absent au scan. 09 achève les builds mais refuse le compte 144 incluant l’index contre 143 objets. 11 échoue sur SyntaxError du contrôleur avant travail ; 12 compile le TU (47,126 s, exit 0), puis le lien échoue sur deux symboles de support. D10/D13 réemploient ces builds sans transformer D09/D12 en réussite. Ces faits préparatoires sont les observations publiques de Root, sans lecture de leurs diagnostics privés. [ARM01](evidence/ARM01/result-public.json) reste FAILED avant worker : CID préparé absent, zéro build/test.

La [CI 1925](evidence/ci-1925eeea.json), run 39054451069, conserve `operations`/`changes` FAILED pour facturation avant toute étape ; `core`/`network` skipped, aucune relance. Ce n’est ni un échec C++ de ces tests ni un PASS CI.

Les consommateurs du nouveau Core et leurs images runtime restent à relier/qualifier ; l’index 24 et les admissions antérieures ne sont pas retargetés. Aucun ancien six-groupes proposer-slashing, replay global Core/EEST/Doppler, service runtime ou campagne n’est lancé. Les qualifications antérieures gardent leur périmètre. Les deux régressions Linux, la reprise/duties/finalité commune, la panne physique et la production ne sont pas qualifiées ici. La tentative 03 dispose d’un accord distinct mais reste non activée : aucune identité opérationnelle, fonds, vente ou ouverture publique créée par ce lot. Copies exactes et origines publiques : [MANIFEST.json](MANIFEST.json).
