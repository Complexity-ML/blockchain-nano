# Qualification Linux de la panne WAL pendant une réorganisation

## Changement

Ajout de `tests/support/sync_fault_linux.cpp`, bibliothèque ELF exclusivement
liée aux tests. Elle interpose fsync et fdatasync, résout le chemin du descripteur
via `/proc/self/fd`, et renvoie EIO uniquement pour le fichier temporaire armé.
Les symboles de contrôle restent compatibles avec le banc macOS. Aucun lien
vers cette bibliothèque n'est ajouté au binaire de production `nano`.

CMake rend les tests Capella protégés par `NANO_SYNC_FAULT` disponibles lorsque
cette bibliothèque existe. Cela ne qualifie pas automatiquement tous ces tests.
Seul le scénario de réorganisation ci-dessous a été exécuté ici.

## Environnement et résultat

Ubuntu 24.04 de l'image de dépendances déjà présente, Linux ARM64 dans Docker
Desktop sur Mac, deux CPU, mémoire limitée à 4 Go, aucun réseau, dépôt monté en
lecture seule. Cache de build dans le volume `nano-linux-sync-build`.
Aucune image supplémentaire construite et aucun VPS modifié.

`consensus_capella_deposit_reorg_sync` passe en **24,63 s**. Les deux branches à
dépôts inversés sont admises, puis EIO est injectée au vote de réorganisation :

- Cinq accès échouent après l'erreur d'écriture.
- WAL conservé : index 65, enveloppe complète et archive du vote cohérentes.
- Suffixe perdu : ancien état complet à l'index 64, sans archive du vote.
- Rejeu : état attendu à l'index 65 dans les deux cas, racines EVM/têtes vérifiées.

Un contrôle indépendant de l'injecteur vérifie que fsync/fdatasync du fichier
ciblé échouent, que ceux d'un autre fichier passent et qu'après désactivation
les deux opérations du fichier initial réussissent. Source et sortie jointes.

## Reproduction

Utiliser l'image dont le digest est dans `image.txt`, monter le dépôt en `/work`
en lecture seule et un volume de build en `/build`, puis :

```sh
cmake -S /work -B /build -DCMAKE_BUILD_TYPE=Release
cmake --build /build --target nano-capella-transition-tests -j 2
ctest --test-dir /build -R '^consensus_capella_deposit_reorg_sync$' -V
```

Empreintes des sources et binaires dans `manifest.json`, logs de configuration,
compilation et test conservés ; espaces finaux normalisés. Le conteneur de
qualification est supprimé après collecte ; le volume reste pour réutilisation.

## Limites

Linux ARM64 local n'est pas le VPS Ubuntu x86-64. Pas de panne physique, de
réseau WAN, de témoins indépendants ni de charge. Ce nouveau run ne qualifie
pas toute la suite de pannes Linux ou le cycle complet des validateurs.
Aucun EIP, profil ou objectif production clôturé.
