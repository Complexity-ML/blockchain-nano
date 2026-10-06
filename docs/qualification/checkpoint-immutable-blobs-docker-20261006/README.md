# Fragments immuables de checkpoint : stockage Docker qualifié localement

Lot C++ issu de main `7c2549fc`, cœur parent `02ec0ab0`. Une seule unité de stockage remplacée dans chaque archive parent ; nouvelle fixture compilée, puis fixture d’exécution actuelle recompilée/reliée. Aucun binaire de nœud remplacé, aucune image reconstruite, aucune campagne réseau relancée. ARM64 utilise `nano-clean-arm64-build-20261002`, AMD64 `nano-clean-rpc-build-20261002` ; images existantes e30ce85e/fd9b0bb9. Hashes des programmes, parents, commandes et sources conservés. Les 1604 fichiers figés correspondent au worktree ; les métadonnées Apple supplémentaires du VPS ne sont pas des entrées compilées.

## Implémentation

`ChainStore` peut publier des fragments opaques identifiés par SHA256 dans le même batch synchrone que le checkpoint/token, avec ou sans liaison à l’exécution. Le contenu existant est comparé et jamais remplacé. Tous les refus de CAS/liaison, de digest ou de budget interviennent avant Write ; une erreur d’écriture rend les lectures/retry de ce handle indisponibles jusqu’à réouverture. Les budgets bornent le nombre et le cumul des octets d’entrée ; avec fragments, le batch agrégé est borné, métadonnées comprises. Les méthodes historiques restent disponibles et préservent leur contrat.

La lecture est liée au chainId/genesis et exige un digest fourni par le futur manifeste authentifié. Le hash ne remplace pas la validation consensus ni l’engagement externe anti-rollback. Les fragments ne sont pas acceptés directement depuis un pair. Aucun garbage collector, manifeste Capella compact ni cache de branches froides n’est encore raccordé.

## Qualification précise

Deux modes nouveaux passent sur chacune des deux architectures :

- Contenu binaire, duplication idempotente, mauvais digest, taille/nombre/batch agrégé, liaison réseau et lecture bornée ; CAS concurrent avec un seul gagnant et absence du fragment perdant ; copie/restauration en lecture seule et hashes physiques inchangés ; altération d’une copie refusée sans remplacement ; checkpoint atomiquement lié à l’exécution et compatibilité des deux anciennes API.
- Erreur fsync réelle injectée sur le WAL : handle incertain bloqué, réouvertures avec suffixe conservé puis suffixe retiré par la fixture ; fragment et checkpoint/token présentent la même disposition. Reprise ensuite possible. Deux dispositions par architecture ; cela ne qualifie pas une panne physique réelle.

La régression existante `atomic-execution-checkpoint` passe aussi sur chaque architecture : 14 refus, CAS concurrent, reorg court, archives conservées, récupération atomique et quatre réouvertures après SIGKILL. Profil de cette fixture Paris (`shanghai:false`) ; aucun fork EVM supplémentaire qualifié.

Originaux terminaux : nouveaux tests ARM64 76510/0 et AMD64 37394/0 ; régression actuelle ARM64 85004/0 et AMD64 30587/0. Le premier lien de régression réutilisait un objet de l’ancienne candidate efbeb9bc sans le paramètre de rétention ajouté depuis : lien refusé sur les deux architectures. La première réparation manquait le fichier fixture dans le sous-ensemble figé : compilation refusée. Les deux erreurs de préparation sont conservées, aucune réussite rétroactive revendiquée. La fixture actuelle et sa hash sont ensuite vérifiées.

## Limite et suite obligatoire

**La saturation du graphe V28 n’est pas corrigée par ce lot.** Il fournit la publication atomique des fragments pour le correctif complet. La candidate runtime reste le cœur `02ec0ab0` et la limite 1024 reste inchangée. À raccorder : manifeste de graphe authentifié, états complets rechargés à la demande dans un cache borné, parcours de fork choice sans suppression arbitraire, copie séparée des fragments nécessaires dans le témoin de reprise et migration des anciens checkpoints. Qualifier ensuite saturation, contexte froid, archives absentes/altérées, pannes, journaux et nouvelle finalité commune à trois nœuds.

Aucune qualification native macOS de ce nouveau lot, aucune qualification de portabilité physique de ces nouveaux fragments entre hôtes, ni garantie de baisse mesurée des IO. Tous les critères de production/EIP, autres échecs historiques, revue indépendante, bridge USDC et lancement Doppler restent ouverts. Production non qualifiée.
