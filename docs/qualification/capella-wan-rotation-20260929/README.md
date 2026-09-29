# Rotation de comité avec partition Mac/VPS — 29 septembre 2026

**Scénario borné qualifié après correction de l'auditeur hors ligne.**
Le premier driver a terminé avec le code **1** ; son erreur et ses journaux
restent inchangés. La campagne réseau n'a pas été rejouée. Le cœur exécuté
`940736f` est inchangé entre le banc `830f0ec` et l'auditeur corrigé `bb3c9b7`.
Ce résultat ne clôt pas l'objectif global de production.

## Réseau réellement observé

- Mac natif, Docker ARM64 et VPS Docker x86-64 ; **deux hôtes physiques**.
  64 clés publiques synthétiques réparties 22/21/21 ; vrais démons cpp-libp2p
  dans un tunnel SSH privé, RPC non exposés publiquement.
- Partition effective à un head commun **122**, avant la modification du comité
  au slot **128**. Les trois branches franchissent cette borne pendant la coupure.
  Finalité inchangée à l'époque 13 et aucun conflit de finalité observé.
- Coupure **97 460 ms** pour une durée demandée de 96 s. Reconnexion aux heads
  137/136/136 ; seuls les agrégats inclus **après 137** reçoivent du crédit.
- Reprise commune à l'époque **18 en 131 132 ms**, sous la limite de 180 s.
  Fenêtre suivante **18 → 19 → 20 → 21 en 143 855 ms** ; liaison entre les
  checkpoints Beacon et les blocs d'exécution vérifiée par le moniteur.
- Trois validateurs sains à l'arrêt : 486/466/468 attestations admises.
  Deux conteneurs arrêtés avec code 0, sans OOM.

## Vérification après arrêt

Les trois propriétaires RocksDB ont été rouverts avec leur témoin et leur token
terminal exact. Les audits corrigés indépendants donnent le même état finalisé
au slot **168**, le même hash SSZ, la même racine d'état et le même comité :

- comité initial `0x65958d612a4fe6a024e4efb1be78b0cebf8b1796dc5b94fb3e3d4038a2641934` ;
- comité final `0xdf6b4506b6079a1d8bc7eb4577c604266a4d533d818d0fa0cf5f44eaad7e581f` ;
- state root `0x2a19dd6cb6050388a06b838d1672b5dbbb38fde0e0335c5a9413034bed2bb781`.

Les octets SSZ de l'ascendance ont été capturés sur les trois instances avant
élagage et sont identiques. L'auditeur recalcule leurs engagements, vérifie
les signatures des proposants, les domaines et les agrégats du comité authentifié.
**19 blocs, slots 150 à 168**, contiennent des contributions distantes : 17 à
32 participants, dont 6 à 26 positions détenues par une autre instance que le
proposant. Les répétitions de membres du comité sont conservées dans le calcul.
Trois autres agrégats ne contiennent que des participants locaux ; deux sont vides.
Ils ne comptent pas dans le minimum de trois blocs avec contributions distantes.
Aucune exigence de quorum, de délai ou de quantité minimale n'a été abaissée.

## Échec initial et correction exacte

L'auditeur initial exigeait `!keys.empty()` pour chaque bloc de l'ascendance.
Il rejetait ainsi les agrégats vides des slots 147 et 149. Leur signature est
l'infini G2 canonique : `c0` puis 95 octets nuls. Cet encodage existe dans le
fixture Ethereum `sync_committee_rewards_empty_participants/sync_aggregate.ssz`
et est déjà traité dans `src/consensus_altair_sync_aggregate.cpp`.

Le correctif ne change que l'auditeur : encodage canonique strict pour zéro
participant, FastAggregateVerify pour toute liste non vide, zéro crédit distant
pour un agrégat vide. Les six contrôles de ce cas passent sur Mac et les deux
Linux. Les bases et preuves initiales ont été réutilisées avec le cutoff **137**.
Les trois résultats complets se trouvent dans `rotation-audits-corrected.json`.

Cinq contrôles adverses rejettent un faux token, un propriétaire de clé altéré,
un corps altéré, une signature de proposant falsifiée et du SSZ mal formé.
Le premier essai de mutation du corps touchait une terminaison SSZ : il a été
rejeté au décodage avant le contrôle d'engagement attendu. Cet essai est conservé
séparément ; une mutation d'un champ fixe vérifie ensuite le rejet d'engagement.

## Ressources observées, sans extrapolation de capacité

46 relevés utilisables par instance, démarrés environ 120 s après le lancement ;
le relevé terminal indisponible après arrêt reste signalé. Maxima **échantillonnés** :

| Instance | Mémoire observée | CPU observé |
| --- | ---: | ---: |
| Mac natif, processus principal seul | 461,06 Mio RSS | 77,9 % (`ps`) |
| Docker Mac, principal et P2P | 413,6 Mio | 54,78 % |
| Docker VPS, principal et P2P | 364,4 Mio | 104,97 % |

Les métriques `ps` et Docker n'ont pas la même fenêtre de mesure. Chaque
conteneur dispose de deux CPU et 3 Gio ; 100 % Docker correspond à environ un
CPU. Aucun pic entre deux prélèvements n'est connu. Les valeurs dérivent des
sorties arrondies des outils, pas d'une mesure exacte octet par octet.
Les écritures cumulées Docker atteignent environ 4,70/4,72 Go, tandis que
l'occupation constatée **après arrêt et audit** est 6 512/6 520 Kio (Mac/VPS),
et 6 876 Kio pour les données natives. Écritures cumulées et occupation disque
ne doivent pas être confondues ; l'amplification des écritures reste un sujet
pour la campagne de charge et d'endurance.

## Limites et reproduction

Fixture Capella/Shanghai fixe : 8 slots par époque, 8 époques par période de
comité, 6 secondes par slot. Pas de charge applicative injectée, de milliers de
nœuds, de participants indépendants administrativement, de changement de registre
pendant la coupure, ni de coupure électrique. Les témoins sont séparés logiquement,
pas placés sur des disques physiquement indépendants. Le réseau privé n'est pas
un déploiement public. Sanctions/cycle complet, charge, exploitation, EVM/RPC,
bridge et revue indépendante restent dans l'objectif global.

Commande de la campagne initiale (répertoire de sortie obligatoirement neuf) :

```sh
NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1 \
NANO_TEST_WAN_ROTATION=1 NANO_TEST_WAN_PARTITION=1 NANO_TEST_NETWORK_TRACE=1 \
node tests/multihost/run.mjs ubuntu@145.239.199.166 \
940736fd2078eb058289434cfefa2c2147df690d /tmp/nano-production-wan-rotation-20260929
```

Le banc exécuté, ses empreintes, les deux versions de l'auditeur, les SSZ capturés,
les configurations, tokens, états d'arrêt, contrôles négatifs et données brutes
sont archivés ici. `manifest.json` conserve séparément l'échec du driver et la
qualification complémentaire corrigée ; `sha256.json` indexe les fichiers.
Les bases natives et volumes Docker restent conservés hors Git. Les images et
volumes de compilation ont été réutilisés, sans nouvelle image.
