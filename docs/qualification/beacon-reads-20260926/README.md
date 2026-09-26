# Lectures Beacon natives — qualification bornée

Source locale non commitée basée sur `30b0de21a1225ba55603de437c1200f3437a1ecb`.
Binaire Nano et runner construits avec CMake sur macOS ARM64.

- `consensus_bellatrix_runtime_restart` : passé en 9,54 s.
- `consensus_bellatrix_beacon_reads` : passé en 0,24 s après correction du montage.
- HTTP réel, runtime Bellatrix réel, checkpoint synthétique vérifié avant RPC.
- Racine SSZ, fork et trois checkpoints comparés à l'état source.
- Annulation et corruption d'état rejetées ; lecture sans mutation.
- Bind public refusé, Origin 403, POST 405, endpoint absent 404,
  service indisponible 503 sans fuite du détail interne.

Le premier montage échouait parce qu'il construisait le RPC avant la vérification
du checkpoint Beacon ; il a été remplacé par le démarrage du runtime réel.
L'essai suivant a rencontré l'interdiction de bind du sandbox ; le test autorisé
sur loopback passe. Le premier journal conserve l'échec et le résultat du test
de redémarrage, sans présenter ce lot initial comme intégralement réussi.

Reproduire :

```sh
cmake -S . -B build-cmake
cmake --build build-cmake --target nano nano-bellatrix-transition-tests -j 4
ctest --test-dir build-cmake -R '^consensus_bellatrix_(beacon_reads|runtime_restart)$' --output-on-failure
```

Seulement trois routes pour `head`. Aucun Assertoor exécuté, aucune preuve de
conformité complète Beacon REST, aucune activation EIP ou approbation production.
