# WAN Capella asymétrique : lecture par état et reprise stable — 29 septembre 2026

**PASS**, exit 0. Noyau `940736fd2078eb058289434cfefa2c2147df690d`.
Trois instances réelles sur deux machines : Mac natif, Docker Linux ARM64 Mac,
Docker Linux x86-64 VPS. 64 clés synthétiques distinctes réparties 22/21/21.
Images et volumes existants réutilisés ; aucune nouvelle image ou dépense.

## Résultat mesuré

- Départ : checkpoint finalisé commun 2, lié au même bloc d'exécution.
- VPS : retard sortant `netem` de 500 ms dans son seul espace réseau ; pas de
  modification du réseau hôte. Finalité commune **2 → 4 en 146,980 s** sous
  défaut, 2 149 paquets mesurés, zéro drop dans ce relevé. Défaut retiré ensuite.
- Reprise : rattrapages **4 → 6 puis 6 → 8**, chacun lié à l'exécution, sans
  crédit d'avancée stable. La fenêtre est remise à zéro, pas le délai global.
- Puis **8 → 9 → 10 → 11**, trois avancées consécutives communes avec égalité
  des blocs d'exécution sur les trois RPC. Observation totale après restauration
  **271,743 s**, incluant le rattrapage, sous la borne inchangée de 288 s.
- **114 lectures** attachées à une racine d'état. Aucun mélange de snapshots
  head/finalité, aucune régression ou racine finalisée contradictoire observée.
- Les trois validateurs terminent sains, avec respectivement **270/256/255**
  attestations locales admises. Deux conteneurs arrêtés code 0, sans OOM.

Le correctif C++ [par state root](../capella-pinned-beacon-20260929/README.md)
est passé sur Mac (5,42 s), Linux ARM64 (3,72 s) et Linux x86-64 (15,61 s),
avec 24 tests du moniteur. La campagne ci-dessus qualifie en plus le raccordement
HTTP/SSH effectif de cette lecture pendant une reprise de réseau réelle.

## Historique et critères

L'[échec initial sur le saut 4→6](../capella-wan-asymmetric-20260929/README.md)
et l'[échec suivant de lecture cohérente](../capella-wan-asymmetric-settle-20260929/README.md)
restent conservés comme échecs. L'[analyse du checkpoint](../capella-wan-ffg-analysis-20260929/README.md)
explique pourquoi FFG n'impose pas une succession stricte de toutes les époques.
Le contrôle exige désormais une vraie fenêtre consécutive **après rattrapage**,
sans prolonger le délai, perdre l'historique des racines ou ignorer une erreur.
Ce nouveau résultat ne requalifie pas rétroactivement les anciens runs.

## Reproduction et provenance

```sh
NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1 \
NANO_TEST_WAN_ASYMMETRIC=1 NANO_TEST_NETWORK_TRACE=1 \
node tests/multihost/run.mjs ubuntu@145.239.199.166 \
940736fd2078eb058289434cfefa2c2147df690d /tmp/NEW-OUTPUT
```

Volumes de compilation estampillés avec cette révision, source du cœur identique
sur les deux hôtes et architectures contrôlées avant export d'un genesis frais.
`manifest.json` et `run.jsonl` enregistrent images et hashes des binaires ;
`executed-harness/` inclut aussi le moniteur importé. Logs bruts comprimés,
configurations et clés de test publiques, statuts Docker et observations conservés.
Les volumes de données et la base native restent disponibles après suppression
des seuls conteneurs de la campagne et de son réseau dédié.

## Limites

P2P libp2p en étoile passant par un tunnel SSH privé ; les ACK TCP sortants sont
aussi retardés. Ce n'est pas un réseau public ni une mesure unidirectionnelle
pure de la latence applicative. Aucun trafic utilisateur ajouté, perte aléatoire,
attaquant Byzantine, panne physique ou rotation de comité authentifiée ici.
Deux hôtes et 64 clés synthétiques ne prouvent pas la grande population ou
l'indépendance de 64 opérateurs. La marge temporelle observée ne prédit pas celle
sous charge. Ce résultat ferme ce scénario borné, pas la production globale,
la conformité EIP générale, l'exploitation, le bridge ou le lancement économique.
