# Candidate Docker C8 à cinq programmes

Le lot du 18 octobre livre une image locale ARM64 sur le Mac et une image locale AMD64 sur le VPS. Les cinq exécutables sont présents, leurs hashes et dépendances dynamiques sont vérifiés, et leurs chemins d’arguments sont exercés dans les images finales. **Production non qualifiée ; aucun nœud réseau n’est démarré ou remplacé par ce lot.**

La recette et le code d’assemblage sont identifiés par `27b529410d26645d023a6c82c25fa7999972d9af`. Le correctif ultérieur du vérificateur JavaScript ne modifie ni recette ni programme C++ ; les mêmes images sont conservées. Leurs identités complètes, bases, cinq hashes et résultats se trouvent dans [release-integrity.json](release-integrity.json).

| Périmètre | ARM64 / Mac Docker | AMD64 / VPS Docker |
| --- | --- | --- |
| Trois nouveaux liens au Core C8 conservé | PASS, zéro rebuild du Core | PASS, zéro rebuild du Core |
| Image `complete`, cinq hashes, cinq ldd, onze notices | PASS | PASS |
| Cinq commandes d’arguments, UID10001:10001 | PASS | PASS |
| Cinq conteneurs possédés absents après contrôle | PASS | PASS |

## Sources et composants réutilisés

`nano` et le Core sont ceux de la qualification [CLI C8](../cli-readonly-20261018/). Le code C++ correspond à `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`; les trois outils de maintenance sont compilés et liés séparément au même Core par architecture. Les identités originales des Core/nano sont comparées à leurs engagements conservés. Les scans `-M` et dépendances `-MD` des trois unités concordent ; les 425 lignes de headers/dépendances sélectionnées et les bibliothèques conservées sont vérifiées avant/après. Le header chain ne diffère que par ses commentaires. Le lot ne revendique pas un rebuild reproductible de tout le Core ou de tous les headers externes.

`nano-p2p` est le composant de [bornage du statut P2P](../p2p-status-bounds-20261017/), et ses 81 sources candidates correspondent encore au dépôt. Les cinq fichiers sont gardés par leur identité physique et SHA pendant l’assemblage et le smoke. Le packager vérifie l’intégrité d’une preuve opaque ; la relation sémantique aux qualifications antérieures reste explicitement documentée ici et dans les reçus. Ce mécanisme n’est pas une attestation tierce.

## Assemblage et commandes exécutées

Une seule recette multi-stage conserve le profil v1 à deux programmes dans `node` et ajoute v2 à cinq programmes dans `complete`. Une seule image effective par architecture est assemblée à partir des bases runtime déjà présentes, sans compiler le Core ni télécharger des dépendances. Les trois nouveaux outils ont été compilés dans les builders Docker déjà conservés. Les contextes temporaires d’assemblage sont retirés.

Le contrôle final appelle `nano --help` (sortie0), puis les quatre autres programmes sans arguments (sortie1 et refus attendu). Pour les outils JSON, types et indicateurs d’absence de runtime/clés sont vérifiés. Ces refus prouvent le chemin d’arguments installé, pas le fonctionnement de maintenance ou la reprise d’un validateur. Réseau `none`, racine RO, UID10001, capacités retirées, no-new-privileges, pas de healthcheck, limite de temps/ressources et aucun montage ni chemin de données, identité, clé ou requête sont utilisés. Le nettoyage est limité aux IDs/noms/labels possédés. L’arrêt forcé du contrôleur entier n’est pas une garantie de nettoyage automatique.

[ARM64](arm64/runtime-smoke.json), [AMD64](amd64/runtime-smoke.json) et leurs `runtime-terminals.json` conservent les cinq commandes, sorties et clôtures. Les `package-result.json`, `package-terminals.json`, logs de build et bibliothèques documentent séparément l’assemblage. Les 33 tests de packaging et cinq microtests du vérificateur passent sur Node Mac ; ce sont des tests injectés, distincts des dix commandes réelles dans Docker.

## Échecs préservés

- Admission initiale des outils ARM refusée sur le ctime de treize documents/dotfiles non consommés, hashes inchangés ; les 425 headers retenus n’ont pas changé. L’acteur n’est pas attribué et les 1047 anciennes lignes ne sont pas déclarées inchangées.
- Premier lot Node lancé depuis le mauvais cwd : FAILED. Le lot suivant corrige ce cwd, sans requalifier le premier.
- Premier assemblage Mac refusé par le client Docker avant toute création d’image : Buildx existant non découvert. Un nouveau client isolé référence ce plugin ; les anciens fichiers/configurations restent intacts.
- Les premiers lots combinés assemblage/smoke ARM et AMD restent FAILED : Docker renvoie `\n` pour un conteneur absent, et le vérificateur exigeait `stdout === ""`. Aucun RUN de smoke n’avait commencé. La correction accepte seulement un stdout blanc avec le code1 et l’erreur/nom attendus ; JSON ou texte non blanc restent refusés. Un cas de régression est ajouté ; le smoke seul est repris, zéro nouvelle image.

Le premier brouillon de copie documentaire s’est arrêté sur le chemin de la preuve ARM conservée dans le dossier initial ; aucun binaire, image ou document normatif n’a changé. Son brouillon partiel est conservé séparément.

Les reçus de ces échecs sont conservés dans `history/`, `unit/` et les fichiers `historical-package-smoke-failure.json` par architecture. Les échecs Native et des campagnes précédentes restent dans leurs dossiers historiques.

## Critères encore ouverts

La candidate n’est ni publiée en registry ni déployée à la place du réseau existant. Paris/Shanghai reste le profil activé. Ce lot ne démontre pas portabilité physique complète du stockage, restauration opérateur avec journaux/témoins, reprise de duties, finalité commune qui avance, clôture des écarts WAN/partitions/cycle validateur, reproductibilité/CI globale ou revue indépendante. Campagne03 FAILED et autorisation consommée ; aucune nouvelle campagne n’est autorisée ici.

La relation Base–Nanobees reste une [proposition à deux séquestres](../../BASE-NATIVE-REPRESENTATION-DRAFT.md) : offre, ratio, couverture, finalité/bridge réel, retraits, termes Doppler et visibilité T0 restent à décider/qualifier. Aucun fonds réel, vente, dépense ou déploiement public. Objectif ACTIF.
