État courant : raccordement runtime livré sur main, parent1c84e7b3 ; ARM648281 et AMD6485922 terminaux0, quatre scénarios chacun,17 TU core reconstruits ; hashes parent/programmes inchangés,1616 sources publiques vérifiées identiques à main. Aucune nouvelle candidate déployée. Les paragraphes suivants conservent l’historique isolé.

État courant vérifié : ARM64 quatre scénarios terminaux0 ; AMD64 trois scénarios terminaux0, dernier scénario BLS actif85922. Source publique1616 fichiers vérifiés, exactement cinq écarts C++ attendus avec main1c84e7b3. Aucun code runtime intégré au main.

État courant : ARM64 runtime intégré isolé8281 terminal0 ; AMD6485922 fixture en compilation, aucune qualification combinée/main.

# Prototype isolé : mémo du writer raccordé au réacteur — 6 octobre 2026

Statut : non appliqué au main, compilations/tests ARM648281 et AMD6485922 actifs. Parent archive reference-memo qualifiée en API isolée ; ce raccordement n'est pas qualifié par ces preuves.

Nouveau membre reference_cache_ du réacteur, donc17 unités core reconstruites (16 dépendants du layout + serializer). Writer cache prochain préparé localement, installé seulement après contrôles d'idempotence réussis ou commit durable réussi ; erreurs/annulation ne publient pas les updates du mémo. Les hot rows restent pleinement validées et peuvent produire le certificat du futur handle froid. Toutes les anciennes validations de snapshot/pruning, signer, witness et restore_durable préflight sont conservées. Pas de bypass des blobs absents/corrompus dans la récupération.

Scénarios requis séparés : cold-reactor/corruptions3, writer-memo/refus5 et restorer/refus7, runtime cold-automatic32/cache8 et cold-bls32. La fixture exige referenceMemoReuses>0 et cache metadata dans le budget local. Mémo ouvert en stats locale uniquement, pas de nouvel endpoint RPC ; aucun état SSZ/comptes complet retenu par ces certificats. referenceMemoEncodedBytes mesure uniquement les metadata du mémo, bornées par limits.envelope_bytes ; coldCheckpoint.cacheEncodedBytes reste le budget encodé du cache des rows, aucun des deux ne mesure RSS.

Restent ouverts : qualification terminale de ce raccordement, publication incertaine/SIGKILL cold, restauration réelle, intégration candidate cohérente, optimisation des préflights complets, graphe1024 et finalité WAN ; EVM/bridge/Doppler et production non promus. Images/volumes réutilisés, pas de nouvelles machines/fonds ni déploiement public.

ARM648281 contrôleur0 :17 core TU compilés,4 scénarios séparés terminaux0,cold-automatic et cold-bls32 blocs/cache8 avec referenceMemoReuses>0 et metadata bornée. Signer/witness/engagements,pruning et sorties workers conservés ; hashes programmes/parent avant/après OK. Mémo installé après succès uniquement ; préflight durable reste complet. Aucune source runtime appliquée au main,et capacité1024/reprise physique/WAN/production restent ouvertes.

Intégration après qualification des deux architectures : mémo local borné, aucun état SSZ/comptes dans les certificats, installation après succès de publication uniquement ; préflight restore_durable complet conservé. Runtime32 blocs/cache8, pruning, engagements externes9, journaux et témoin/reprise. Aucun chiffre RSS ou amélioration WAN revendiqué. Nouvelle couverture WAL ARM64 qualifiée séparément ; AMD6468450 active et non couverte par cette clôture.
