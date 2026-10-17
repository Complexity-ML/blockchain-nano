# Statuts P2P bornés — 17 octobre 2026

Un pair pouvait fournir un `best_hash` arbitraire et faire conserver son statut JSON entier. Le lecteur générique autorisait jusqu’à 16 MiB, ensuite recopiables dans le rapport interne. Le correctif borne handshake/chainsync à **4 KiB avant accumulation et analyse JSON**, valide le hash canonique s’il est fourni, puis conserve uniquement les champs de statut utiles. Les extensions racine inconnues sont ignorées ; les annonces imbriquées restent intactes pour ne pas transformer une annonce ambiguë en annonce compatible. Les messages de blocs/données gardent leur plafond indépendant de **16 MiB**.

| Environnement | Compilation/lien P2P | Deux tests concernés | Clôture |
|---|---|---|---|
| Mac natif ARM64 | PASS | PASS | PASS ; 902 entrées du producteur retenu identiques avant/après |
| Docker Mac, Linux ARM64 | PASS | PASS | PASS ; nouvelle configuration admise après stabilité initiale |
| Docker VPS, Linux AMD64 | PASS | PASS | PASS ; reçu exact rapatrié, contrôle resté sur le VPS |

Les deux nouveaux groupes couvrent neuf hashes malformés, l’ancien statut sans `best_hash`, les annonces Altair/Bellatrix/Capella et leur projection, puis cinq cas de frontière : exactement 4096 octets, 4097 fragmentés, 4097 en une lecture, maintien du lecteur générique au-delà de 4 KiB et refus du writer avant émission. Les anciens cas des **deux fichiers modifiés** passent aussi ; les autres suites ne sont pas rejouées. Chaque binaire P2P exécute seulement sa sortie d’usage attendue (code 1), sans ouvrir de service ni générer d’identité.

La base est `6139576befa194cb251c87f21eff406d0c7f43fd`, avec les cinq fichiers de la proposition revue appliqués. Le champ `sourceRevision` des reçus bruts désigne cette **base**, pas un commit prétendument inchangé : [le manifeste commun](source-manifest.json) fixe les 81 fichiers exacts des exports parent/candidat. Le candidat correspond aux sources du dépôt. [Les neuf composants](qualified-components.json) portent leurs SHA-256 et tailles. Les nouveaux tests restent dans les deux cibles CTest existantes ; aucun CMake, Core ou EVM modifié.

Une configure réseau neuve par contexte sélectionne le SDK existant ; seuls P2P et les deux tests sont construits (la baseline ne construit que les tests). Les véritables dépendances `-M` concordent avec les `.d` produits. Le SDK Linux est celui des images immuables existantes, monté en lecture seule et sans réseau. Les observations GNU stat/SHA invitées restent distinctes des FD/neuf champs hôte ; aucune identité physique n’est assimilée entre les deux. La chaîne complète de fabrication du SDK n’est pas requalifiée. Onze unités de traduction applicatives/de test et leurs liens (hors sondes CMake), zéro build d’image ou de dépendance, zéro recompilation Core. Les contrôleurs effectivement utilisés et les logs sont conservés ici. Les reçus privés détaillés restent chez leurs propriétaires.

## Échecs conservés et causalité

1. La première enveloppe Docker refuse `/usr/bin/python3` absent de l’image, **avant toute compilation ou exécution C++**. Le contrôleur est déplacé sur l’hôte ; l’image n’est pas modifiée.
2. Les deux nouveaux selectors compilent contre les anciens headers et échouent réellement avec `malformed best_hash accepted` et `oversized status accepted or stream not reset` (codes 1 attendus). Leur enveloppe termine néanmoins **FAILED** : seul le `ctime` de sa configuration hôte neuve change de 44,7 ms ; huit autres champs et SHA restent identiques. Le diagnostic localise cette différence, sans en inventer la cause. Ce résultat brut n’est pas promu en qualification complète et les selectors ne sont pas rejoués.
3. L’enveloppe suivante attend, avant la première exécution de son **nouveau** contrôle, trois observations FD/neuf champs/SHA identiques couvrant au moins deux secondes, face aux bytes JSON canoniques connus. L’ancien contrôle et ses engagements restent inchangés. Le candidat ARM termine PASS. Les clôtures native et AMD passent également. Les seules enveloppes de ce lot ont été retirées ; des listings Docker réussis confirment leur absence.

## Portée restante

Les trois P2P sont des composants qualifiés dans ce périmètre, **pas encore des images runtime réassemblées ni des admissions de campagne mises à jour**. L’index antérieur de 24 actifs et ses trois admissions n’est pas retargeté. Aucune duty, signature de validateur, reprise réseau ou finalité commune n’est démontrée ici. La tentative 02 reste FAILED et son domaine signé ne doit pas être réutilisé ; la tentative 03 attend l’accord distinct déjà demandé. Aucun fonds réel, vente ni déploiement public.

Le [risque de sortie perdue après reorg/pruning](consensus-source-gap-open.md) est un **constat source**, avec reproduction ciblée préparée séparément ; il n’est ni corrigé ni démontré par un test exécuté dans ce lot. Le cycle des validateurs, les autres cas consensus/WAN/charge et de restauration restent ouverts. Les preuves EVM/RPC/EIP antérieures restent dans leur périmètre, sans nouvelle promotion. CI/facturation, revue de sécurité externe et termes/vérificateur Base–Nano/USDC/Doppler restent ouverts. La revue source interne jointe ne remplace pas cette revue de sécurité externe. **Objectif actif ; production non qualifiée.**
