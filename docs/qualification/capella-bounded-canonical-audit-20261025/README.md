# Audit canonique Capella avec fenêtre chaude bornée — 25 octobre 2026

La réussite décrite ici porte sur le relecteur canonique Core à 160 blocs en conteneurs ARM64 et AMD64, ainsi que sur des contrôles locaux de sélection CI. Elle ne clôture pas l'objectif de production. Les échecs antérieurs, le plan SOURCE initial et les qualifications plus étroites restent conservés ; aucun statut d'EIP/fork ni autorisation de campagne n'est promu.

## ARM64 et AMD64 : six groupes réels à 160 blocs

Le [reçu ARM02](proofs/arm-02-result-public.json) rapporte Docker exit 0, aucun échec secondaire et 978 fichiers détenus puis fermés. Le [reçu AMD03](proofs/amd-03-result-public.json) rapporte aussi Docker exit 0, six groupes réels et 977 fichiers détenus/fermés, en 117 446 ms ; il réutilise les Core, CLI et driver déjà produits, sans compilation ni build d'image supplémentaire. La candidate de test est `c0ffcea8326ecb39e3283db9932eb221c1d57232`. Le reçu identifie la source de production `5dfb9ddcaaa5ba1e2a6bedf1d038ba0e388c8b60` comme inchangée entre les deux générations de tests.

| Contrôle réel | Résultat borné |
| --- | --- |
| Ancienne API par défaut | 160 blocs signés après l'ancre, 161 blocs comptés avec celle-ci ; fenêtre déclarée 64 |
| Fenêtre explicite | Même histoire et engagements, fenêtre déclarée 48 |
| Ancienne signature corrompue au slot 2 | Refus au rejeu signé, malgré sa position hors de la dernière fenêtre chaude |
| Fenêtre de deux records | Refus sans agrandissement silencieux |
| Annulation après progrès d'indexation | Refus coopératif ; fichiers des DB source et copie inchangés |
| Namespace et index préexistant | Index dans la DB refusé avant création ; fichier préexistant conservé |

Les deux chemins positifs observent un pic de 33 records, 686 517 octets SSZ, 4 861 032 octets SSZ avec la réserve d'un état suivant, 17 élagages et un index de 17 920 octets (160 × 112). Le checkpoint externe au slot 16 reste prouvé alors que l'ancre chaude finale est au slot 144. Toutes les vérifications physiques full9/SHA restent inchangées ; 118 fichiers de la fixture sont ouverts puis fermés, et l'audit crée zéro signature BLS. La génération de la fixture publique crée séparément 1 611 signatures de test.

Ce lot contrôle les fichiers de ses DB synthétiques source et copie ; il n'ouvre ni ne qualifie les journaux privés anti-double-signature des validateurs.

La [comparaison croisée](proofs/cross-arch-result-public.json) vérifie exactement huit champs canoniques/checkpoints sur les deux modes, sans normalisation des états ou hashes. Elle compare deux audits offline de la même fixture publique ; elle ne démontre pas une finalité réseau commune ni des duties réels sur trois environnements.

Le pic RSS mesuré pour le driver entier est **84 508 672 octets en ARM64 et 81 203 200 octets en AMD64**, incluant génération de fixture, matérialisation RocksDB et audits. Ce n'est ni la RAM isolée de l'audit ni une limite totale garantie par le budget SSZ. Le vecteur d'ancêtres de dépôts atteint 143 entrées ; le compteur d'historique de dépôts de cette fixture reste nul. L'histoire auxiliaire de dépôts, son arbre cumulatif, les états d'exécution, caches et allocations demeurent hors d'une prétendue borne RAM totale.

## Réutilisation et échecs conservés

L'[ARM01 global](proofs/arm-01-result-public.json) reste FAILED (Docker exit 1). Son marqueur terminal lie cependant la [sortie positive du driver legacy O1](proofs/arm-01-legacy-stream-stdout.log) : 1 026 transitions signées, quatre groupes et neuf refus attendus, conservés comme sous-qualification héritée. ARM02 ne l'a pas rejoué ; cela ne démontre pas un nouveau driver legacy O3. Le [stderr ARM01](proofs/arm-01-bounded-audit-stderr.log) refuse le contrôle RSS de la fixture publique (seuil 128 MiB), au stage `fixed-public-unit-fixture` ; aucune cause physique n'en est déduite. L'AMD01 antérieur conserve son refus générique du rejeu 1 026 ; son [dernier stage publié](proofs/amd-01-stages-stdout.log) est `run-canonical_stream`, sans explication de cause.

L'archive Core identifiée par ARM02 comporte 144 membres : 141 payloads conservés byte pour byte, deux objets remplacés (`consensus_capella_audit.cpp.o`, `storage.cpp.o`) et un ajouté (`consensus_capella_canonical_stream.cpp.o`). Les objets retenus ont leurs headers historiques : **aucun rebuild propre ni fermeture MMD complète avec les headers courants n'est revendiqué**. Le nouvel ELF `nano` est lié et son `--help` annonce `audit-capella` ; ce smoke test ne constitue pas un audit CLI complet d'une base opérateur.

## Contrôles de sélection CI, hors job CI

Le [premier reçu CI](proofs/ci-01-result-public.json) reste FAILED : [TAP01](proofs/ci-01-stdout.log) observe 15 PASS et un échec. Le mock `cmake` sous `/tmp` n'a pas été utilisé ; le vrai CMake échoue avant d'atteindre la sélection et le contrôle JUnit. Cela concorde avec le montage `/tmp noexec` signalé par l'opérateur. Le reçu conserve `runtimeAndSourceFilesStrictlyFenced:false`.

La [reprise ciblée CI02](proofs/ci-02-result-public.json), avec un `/tmp` possédé et exécutable dans le conteneur, observe [un PASS sélectionné et 15 SKIP](proofs/ci-02-stdout.log), Docker exit 0, 138 fichiers ouverts/fermés et fences strictes vraies. Les trois sources CI sont identiques. Les 16 cas distincts ont donc chacun un succès observé sur les deux lancements ; ce n'est pas un nouveau batch 16/16, et le premier reçu FAILED n'est pas requalifié. Aucun job GitHub CI n'a été exécuté et aucune limite de compte/facturation n'est levée par ces contrôles locaux.

## AMD02 long : échec conservé, périmètre distinct d'AMD03

Le [reçu AMD02](proofs/amd-02-result-public.json) reste FAILED : Docker exit 1 après 339 066 ms et 976 fichiers détenus/fermés, sans marqueur runtime positif. Son [stderr public](proofs/amd-02-canonical-stream-stderr.log) donne `elapsedMs:300007`, `publicFixtureReason:"canonical-stream fixture wall bound"`, `wholeDriverPeakRssKiB:26824` et le marqueur `testStage:"initialization"`. Ces valeurs constatent un refus de la borne de 300 secondes ; elles n'établissent pas la cause du coût CPU ni une absence de travail derrière ce marqueur.

Le défaut agrégé `testDriverOptimization:"O1"` des reçus ne décrit pas le choix individuel du legacy AMD02 : les [commandes réelles](proofs/amd-02-steps-public.json) compilent `canonical_stream` avec l'override O3, tandis que le driver borné est compilé en O1. La [note de clarification](proofs/amd-02-optimization-clarification-public.json) signale cette imprécision du champ global sans réécrire les anciens reçus.

AMD03 ne remplace pas cet échec : il qualifie le scénario borné de 160 blocs et le smoke test `nano --help`, sans rejouer le legacy long 1 026. Le parcours long legacy AMD64 reste donc **non qualifié dans sa borne de 300 secondes**, même si le scénario de fenêtre Core passe sur les deux architectures. Le legacy long ARM64 demeure le résultat O1 hérité déjà décrit. Aucun budget n'est augmenté, aucun test 1 026 supprimé et aucun critère nouveau n'est inventé pour transformer l'échec en succès.

## Critères restants inchangés

Les index non vides sont conservés ; leurs locateurs opérateur restent privés et leur propriété n'est pas déclarée suffisante pour une suppression automatique. Le nettoyage opérationnel, la mémoire auxiliaire des dépôts, le coût CPU du parcours long, la fermeture complète des headers courants, l'audit de sécurité indépendant, la qualification CI effective et la reprise réelle des duties/finalité entre Mac natif, Docker Mac et Docker VPS restent ouverts. Les autres critères des quatre documents de production sont intégralement conservés. La campagne 04 demeure terminale FAILED ; aucune campagne 05, signature réelle, fonds, lancement public ou condition économique n'est autorisé par ce lot.

Les [identités logicielles](software-identities-public.json) lient sources, archives, ELF et six fournisseurs par architecture. Les images sont les outils de compilation déjà présents ; aucun nœud ni image de production n’est déployé par ce lot.
