# Panne WAL pendant une réorganisation — VPS Linux x86-64

## Résultat

Sur le VPS Ubuntu 24.04, `consensus_capella_deposit_reorg_sync` passe en
**99,87 s** (CTest total 99,88 s), au commit `c379e7d` avec arbre propre.
Deux branches à dépôts inversés font passer le validateur A de l'index 64 à 65.
Une erreur EIO est injectée sur la synchronisation du WAL au vote de bascule :

- Les cinq accès à l'état incertain sont refusés après l'erreur.
- WAL conservé : état nouveau complet, index 65 et archive du vote présents.
- Suffixe perdu : état ancien complet, index 64 et archive du vote absente.
- Dans les deux cas, rejeu vers l'index 65, enveloppe durable, tête et état EVM vérifiés.

## Environnement et reproduction

Image de dépendances Ubuntu 24.04, architecture amd64, digest dans `image.txt`.
Deux CPU, 4 Gio, réseau désactivé, dépôt du VPS monté en lecture seule en `/work`,
volume réutilisable `nano-linux-sync-build` en `/build`. Le dépôt a été mis à
jour par `git pull --ff-only origin main`. Aucun nœud public démarré.

```sh
cmake -S /work -B /build -DCMAKE_BUILD_TYPE=Release
cmake --build /build --target nano-capella-transition-tests -j 2
ctest --test-dir /build -R '^consensus_capella_deposit_reorg_sync$' -V --output-junit /build/reorg-sync.xml
```

Le conteneur de qualification termine avec code 0, sans OOM. Journaux, JUnit,
état final Docker et empreintes des sources/binaires conservés. Les espaces
finaux des journaux sont normalisés. `environment.txt` provient d'un conteneur
éphémère de collecte utilisant la même image et le même volume en lecture seule ;
son hostname diffère donc du conteneur de test. Ce dernier est supprimé après
collecte, le volume de compilation est conservé.

## Limites

Cette preuve complète macOS et Linux ARM64 pour ce scénario précis. Le VPS
héberge un test local isolé : aucune communication WAN entre nœuds, panne
physique, charge concurrente ou perte d'un témoin indépendant n'est simulée.
L'injecteur vise fsync/fdatasync ; le suffixe WAL est tronqué par le banc de test.
Le résultat ne qualifie ni tous les points d'interruption, ni tout le cycle des
validateurs, ni l'activation d'un fork public. Aucun EIP ou objectif production
n'est clôturé. Le contrôle indépendant de l'injecteur reste documenté dans la
[preuve Linux ARM64](../linux-deposit-reorg-sync-20260927/README.md).
