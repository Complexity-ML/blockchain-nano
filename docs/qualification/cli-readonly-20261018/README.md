# C8 — status/audit en lecture seule et régressions ciblées

Le correctif C8 et ses preuves partent de main `467485c596679d80a6e47991053174fde1ac2942`. Les qualifications ci-dessous sont terminales et bornées ; le refus du premier contrôleur ARM reste conservé. Les sources de ce lot et les rapports publics sont archivés ensemble. Aucun clean build ni déploiement du nœud réseau n’est revendiqué.

## Changement et périmètre logiciel

`status` et `audit` demandent désormais `Blockchain::ReadOnly`. Le constructeur exige une liaison de chaîne et un tip déjà présents ; il ne crée ni DB manquante, ni liaison ou commit initial. Les chemins ReadWrite et ReadWriteExisting restent distincts. L’audit legacy conserve son scratch RW séparé puis nettoyé. Les écritures sur le propriétaire RO sont refusées ; tip, état canonique, compteurs, difficulté, mempool et curseur observés restent inchangés. Les caches internes ne sont pas présentés comme une façade entièrement immuable. ReadOnly ne certifie aucun checkpoint Beacon et ne contourne pas la garde de readiness.

L’intégration proposée contient exactement six destinations : `src/main.cpp` (`b4433387… /8164`), `src/chain.cpp` (`bc0a1922… /44702`), `include/nano/chain.hpp` (`d19297f3… /8250`), les nouveaux TU API (`6431cdca… /11239`) et probe CLI (`84a17dad… /19202`), puis CMake (`4e97be20… /179962`). Son manifeste source parent reste SOURCE_ONLY `e92df59f… /6057` ; il n’est pas reclassé par cette note.

Le réemploi est **sémantique et borné** : un nouveau `chain.cpp.o`, archive de 143 membres dont 142 payloads d’objets byte-exacts, main et feuilles de tests recompilés. Le header ne change que son commentaire : l’ancien header C7 est effectivement compilé, le nouveau documentaire est tenu mais non consommé par M/MD. Ce n’est ni le sélecteur e9 strict des 143 M, ni un clean build, une qualification ABI universelle, une preuve de reproductibilité ou un PASS CMake/CI généré. Aucune image n’a été reconstruite.

## Résultats distincts

| Étape | Résultat transmis par Root | Portée |
| --- | --- | --- |
| ARM compilation 64770 | **EXIT 1 / FAILED conservé** | 36 produits, neuf M/MD, huit liens et huit chargeurs achevés ; zéro RUN. Les modes des anciens host/helper ont été observés 0400 → 0644 après admission, contenu identique. Leurs lignes historiques ne sont pas réparées. |
| ARM reprise runtime31991 | **EXIT 0 / PASS** | Huit cas exécutés sur les mêmes36 produits conservés ; aucune nouvelle compilation, archive, lien ou vérification de chargeur. |
| Audit passif ARM | **PASS communiqué** | 36 lignes produits originales,19 fichiers du case,96 sources consommées et neuf logiciels vérifiés ;69 entrées de la cohorte publique intactes. Zéro C++ ou runtime supplémentaire. Pin complet : **4421e66514fb98a5fe1e2334a8be04c87d069683d7a50498c78609709e1087c9 /10218**. |
| AMD 9878 | **EXIT 0 / PASS : Core1 M/CXX, archive143/1/142, huit M/CXX/liens/chargeurs et huit RUN ;36 produits,19 fichiers du case,96 sources consommées et10 logiciels relus inchangés ; conteneur supprimé, image existante conservée. Sources et résultats : [AMD64](amd64/result-public.json), [audit passif](amd64/root-passive-audit-public.json).** | Qualification Docker Linux AMD64 bornée aux huit cas indiqués ; aucun résultat Native/macOS ou CMake n’en est déduit. |

Sur ARM, l’API RO passe six groupes, onze refus attendus, deux vraies barrières d’écriture et cinq ouvertures RO. Le probe passe dix-huit vraies commandes CLI : huit succès, dix refus et quinze contrôles de la source. Les régressions ciblées passent :185 checks chain, block-auth, les sept records bootstrap, Beacon gate et recovery-audit Paris/Shanghai. API et probe créent zéro signature ; ce chiffre ne doit pas être étendu aux autres fixtures BLS du lot.

Le runtime ARM est une qualification séparée ; il ne transforme pas 64770 en réussite ni ne rafraîchit une ancienne autorité. Les69 entrées du volume public sont contrôlées passivement, pas ouvertes comme DB anciennes par les nouveaux tests. Le runtime AMD utilise un état Linux synthétique neuf, sans ancien volume nommé ni ancienne DB montée. Les preuves de stockage/journaux antérieures gardent leur propre portée.

## Critères qui restent ouverts

Paris/Shanghai demeure le profil EVM/RPC activé ; aucune activation Cancun, compatibilité EIP globale ou nouveau fork n’est déduite de C8. Génération CMake, CI, reproductibilité et revue de sécurité indépendante restent non qualifiées par ce lot. Il ne remplace pas le runtime réseau 1925, ne reprend aucune duty et ne prouve ni restauration opérateur, progression de finalité commune, authentification indépendante d’un checkpoint, full Beacon ABA ou robustesse WAN globale. La campagne privée 03 FAILED est consommée ; aucune 04 n’est autorisée. Tous les anciens critères et échecs restent intacts. Production non qualifiée, objectif ACTIF.

La réponse utilisateur « je sais pas » ne fixe pas l’économie Base–Nano. L’hypothèse de travail est un actif économique et une représentation Base couverte par une réserve NANO vérifiable ; offre S, ratio, allocations, conversion et retraits ne sont pas décidés. Le gateway actuel à encours initial nul n’accepte pas directement une préémission Doppler : l’adaptateur reste à construire et qualifier. Réserve NANO, produit USDC de vente et liquidité Doppler sont distincts. Bridge/finalité source/reorgs, termes réels et visibilité T0/indexation externe restent ouverts ; aucun financement, liquidité, vente, fonds réel ou déploiement public n’est présumé.

## Conservation des preuves

Les quatre ajouts normatifs sont exclusivement des suffixes : conserver chacun des quatre corps normatifs comme préfixe byte-exact, ajouter une seule fois le marqueur `nano-cli-readonly-c8-20261018`, et recalculer les pins avant/après sur le texte final. Les baselines du manifeste sont des lectures de sources publiques, pas une autorité opérationnelle. Si le dépôt reçoit entre-temps un autre append, préserver son corps actuel intégral ; ne jamais remplacer celui-ci par la baseline du draft. Les résultats AMD et le pin complet de l’audit ARM figurent ci-dessus ; ils conservent leur portée propre. Les reçus privés, DB, clés et autorités complètes sont exclus de la livraison publique.

Le comparateur pur d’archives est déjà conservé dans [la qualification C7](../rpc-state-proof-amd-20261018/source/closure/archive_object_replacements.py). Les programmes archivés gardent leurs contrats de contrôle ; leurs autorités privées ne sont pas publiées. La qualification Mac natif et la génération CMake sont désormais documentées dans [le lot Native](../native-c8-cmake-20261018/README.md), avec leurs refus et reprises distincts. L’image de diffusion C8 reste à produire et qualifier.
