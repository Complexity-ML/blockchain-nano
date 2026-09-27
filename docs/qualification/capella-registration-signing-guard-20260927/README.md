# Vérification atomique des index automatiques avant signature

## Défaut traité

La boucle résolvait les clés puis relâchait le verrou du reactor avant de
signer. Si une branche réattribuait un index entre ces deux opérations, le
contrôle complet d'identité empêchait une signature pour une autre clé, mais
son erreur générique pouvait rendre la boucle indisponible.

Les trois entrées `propose_current`, `attest_current` et `sync_current` reçoivent
maintenant la clé d'inscription automatique. Sous leur mutex de signature,
elles comparent le journal à cette clé puis la paire clé/index à la tête
admise courante. Le cache lié à la racine est réutilisé ou recalculé avant
vérification. Une absence ou un index différent lève `StaleCapellaDuty`, déjà
traité par la boucle comme abandon du poll puis nouvelle résolution au suivant.
Une mauvaise clé de journal reste une erreur de configuration bloquante.

Le chemin à index fixe reste strict. Ni l'identité complète du devoir, ni la
vérification BLS, ni le journal anti-slashing ne sont contournés. Le verrou est
conservé entre la nouvelle vérification et le chemin de signature existant.

## Nouvelle couverture et limites

Le nouveau scénario `consensus_capella_registration_signing_guard` injecte un
index incorrect pour une clé connue sur chacun des trois chemins. Il vérifie
le type d'erreur retryable, zéro appel au signataire et l'absence de mutation
du checkpoint/journal. Trois associations incorrectes clé/journal sont testées
séparément : elles doivent rester des erreurs de configuration.

Le scénario poursuit ensuite la boucle réelle avec 64 clés à index automatique,
propositions, attestations et sync, finalité et reprise des journaux.
Ce n'est pas une injection concurrente par thread ni un scénario de deux dépôts
concurrents sur des branches distinctes : ces qualifications restent ouvertes.
La reprise du poll utilise le handler `StaleCapellaDuty` existant ; ce test
injecte l'index erroné directement aux entrées de signature, pas dans le scheduler.

Les simulations locales ne qualifient pas la disponibilité WAN sous reorgs
soutenues. Aucun profil réseau, EIP ou objectif production clôturé.

## Résultat final

**3/3 tests passent en 160,16 s**, deux processus concurrents : nouveau guard,
régression `consensus_capella_scheduled_finality` à index fixe et
`consensus_capella_deposit_auto_registration`.
Le nouveau scénario produit ensuite 40 blocs, finalise époque 3, rouvre les
64 journaux et admet 272 votes/1 280 messages sync. Le dépôt est consommé une
fois au slot 80, la nouvelle clé active époque 17 produit trois attestations,
et les deux propriétaires sont rouverts après poursuite au slot 161.
Empreintes dans `manifest.json`, logs et JUnit archivés. Seuls les espaces de
fin de ligne des logs ont été normalisés. Pas de déploiement VPS.
