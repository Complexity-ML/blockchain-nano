# Sorties : panne WAL sur le VPS Linux

Sur `ebe1d16`, arbre Git propre, les deux tests `consensus_capella_queued_exit_sync`
et `consensus_capella_queued_bls_sync` passent : **2/2, 26,48 s**.
Les quatre combinaisons principal/témoin et suffixe conservé/perdu sont vérifiées
pour chaque opération. La sortie restaurée conserve exactement ses octets,
la réadmission est idempotente et l'état EVM reste lié au bon checkpoint.

Le dépôt a été mis à jour par `git pull --ff-only origin main`. Le conteneur
Ubuntu amd64 utilisait deux CPU, 4 Gio, aucun réseau, le dépôt en lecture seule
et le volume existant `nano-linux-sync-build`. Son code final est 0, sans OOM.
Image, limites, état Docker, révision, empreintes du binaire et des sources,
logs et JUnit sont conservés. Le conteneur terminé a été supprimé après collecte ;
le volume de compilation est conservé pour la suite.

Limites : panne fsync/fdatasync injectée, suffixe perdu par troncature, arrêt
`_exit` du processus fautif. Ce n'est pas une coupure physique ni une campagne
WAN. Cette preuve Linux concerne l'admission des sorties, pas tous les points
d'interruption de leur inclusion, finalisation, paiement ou élagage.

```sh
cmake -S /work -B /build -DCMAKE_BUILD_TYPE=Release
cmake --build /build --target nano-capella-transition-tests -j 2
ctest --test-dir /build -R '^consensus_capella_queued_(exit|bls)_sync$' -V
```

Les espaces finaux des journaux texte sont normalisés.
