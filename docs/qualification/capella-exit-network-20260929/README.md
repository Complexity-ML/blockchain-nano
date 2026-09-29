# Sortie de validateur par le réseau réel — 29 septembre 2026

Statut : **scénario local RPC/libp2p/retrait passé**, jalon global ouvert. Base du dépôt : `f1c6c3a`; patch exact et empreintes dans `candidate.patch.gz` et `candidate.json`.

Nouveau scénario `--exit-network-cli <nano> <nano-p2p>` : deux processus Nano
locaux, vrai cpp-libp2p, 64 clés synthétiques sur le producteur, aucune sur le
suiveur. Le suiveur reçoit seul la sortie signée du validateur 0 via le service
RPC privé, puis son enfant de test exécute le vrai daemon. Aucun accès direct
à la base du producteur pour lui transmettre la demande.

Le profil synthétique démarre au slot 8, exige une époque de présence et réduit
le délai de retrait à une époque. Les paramètres du réseau réel restent inchangés.

## Échecs conservés

- `build-initial-failure.log` : erreurs de types GMP et d'API dans le nouveau test.
- `profile-initial-failure.log` : incohérence des délais du profil synthétique.
- `frozen-balance-failure.log.gz` : les deux nœuds convergent ; un retrait complet
  est observé ; le réseau progresse jusqu'à la finalité époque 8. Le test échoue
  sur son hypothèse de solde inchangé entre l'observation du retrait et le bloc
  finalisé. Cela ne constitue pas un succès du scénario.

Le contrôle corrigé distingue le retrait du capital des paiements ultérieurs
(par exemple récompenses de comité sync encore applicables). Il compare les
sommes exactes à hauteur finalisée et en tête, compte les paiements du capital,
relit les bases avec leurs témoins et les dernières réservations de signatures.
Ce contrôle corrigé a passé ; le premier log seul ne permettait pas de conclure sur la cause.

## Limites restant ouvertes

Deux processus sur un Mac ne prouvent pas un consensus entre opérateurs
indépendants. Mac/VPS, réorg/réindexation de cette sortie, crash/WAL et élagage
adversarial restent à qualifier pour clôturer le jalon. Aucun fonds réel utilisé.

Commande :

```sh
build-release/nano-capella-transition-tests --exit-network-cli "$PWD/build-release/nano" "$PWD/build-network/nano-p2p"
```

CTest expose `consensus_capella_exit_network` si `NANO_NETWORK_EXECUTABLE` est configuré.

## Résultat local corrigé

`network-pass.log.gz` : code 0. Le catalogue a exporté et livré une sortie
`voluntary_exit` au producteur via le vrai daemon. Le retrait est finalisé au
bloc d'exécution 56, hash
`0x5e6b7d3679d7d63d10a5cb3196d4f140858f2ed1e6ce59478c679d8e6605a861`.
Le solde au même bloc est identique sur les deux nœuds :
`0x1ca0acb6b916ed400` wei.

Après réouverture, chacun retrouve 16 paiements au bon destinataire, dont un
seul paiement du capital (> 30 milliards de gwei, seuil propre à cette fixture
initialisée à 32 milliards). Les autres paiements sont des retraits partiels ou
résiduels. Leur somme exacte correspond au solde EVM. Le journal du validateur 0
n'a aucune réservation de proposition ou d'attestation à partir de son époque
de sortie 6. Les signatures sync encore autorisées ne sont pas interdites.

Le solde consensus résiduel n'est pas nécessairement nul : la logique de comité
sync continue d'accumuler des récompenses tant que l'affectation reste valable
(`src/consensus_altair_sync_aggregate.cpp`), puis les balayages les retirent.
Les deux processus ont été arrêtés successivement et leurs têtes finales ne
sont pas nécessairement au même slot ; la comparaison de finalité porte bien
sur le même bloc 56.

Attention : le champ historique `finalizedExitRetired` du log vérifie ici
`pending_exits().empty()`, pas directement l'absence de l'archive durable.
Cette preuve ne clôture donc pas à elle seule le critère d'élagage/retrait de
l'archive. Les contrôles de conservation portent sur les paiements observés,
ils ne constituent pas une preuve exhaustive de toute l'émission monétaire.

Les espaces finaux des journaux texte de compilation/CTest sont normalisés.
