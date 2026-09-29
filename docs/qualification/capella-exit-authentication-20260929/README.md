# Rejets de sorties signées — 29 septembre 2026

Deux tests CTest passent sur Mac (10,92 s) : `consensus_capella_queued_exits`
et `consensus_capella_exit_wire`. Chaque scénario refuse explicitement huit
requêtes : mauvais domaine, fork, genesis, index, clé, époque future, index
inconnu et durée de présence insuffisante. Le checkpoint et la file restent
inchangés après chaque rejet. Les requêtes utilisent de vraies signatures BLS
sur les contextes erronés ; elles ne sont pas seulement des octets aléatoires.

Ces cas complètent les contrôles existants de signature altérée, identité du
service, bornes, doublons et réorganisation. Ils qualifient le réacteur local,
pas huit injections WAN. Le patch exact et les empreintes du banc sont conservés.

```sh
ctest --test-dir build-release -R '^consensus_capella_(queued_exits|exit_wire)$' -V
```
