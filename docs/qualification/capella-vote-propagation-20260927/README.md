# Capella distributed vote propagation — 27 septembre 2026

**Résultat : ÉCHEC**, même critère que la qualification distribuée précédente : finalité commune d’époque >= 2, trois observations, délai de 210 secondes. Deux processus réels, chacun possédant 32 des 64 clés synthétiques, transport libp2p local. Aucun flood ni fonds réels.

Le binaire diagnostique applique `diagnostic.patch` avec `git apply --unidiff-zero` au commit indiqué dans `manifest.json`. La sonde opt-in est limitée à 4096 événements par processus et ne modifie aucune règle de consensus. Elle est conservée ici plutôt qu’intégrée au code de production. Les binaires locaux du test restent instrumentés tant qu’ils ne sont pas recompilés.

## Observations nouvelles

- Les deux workers restent sains, justifiés à l’époque 2 mais finalisés à l’époque 0. Aucun dépassement de slot mesuré dans les polls du validateur.
- 181 et 191 préparations de votes différés : toutes acceptées, chacune modifiant un dernier message. Ce journal précède le commit durable et ne constitue donc pas, seul, un accusé de persistance.
- Par rapprochement du même `wireRoot` avec son admission au slot courant sur l’autre nœud, 38 admissions tardives sont observées de chaque côté. Retard maximal : 12 et 11 slots, avec des slots de six secondes. Cette mesure porte sur l’admission au cœur, pas sur la latence réseau seule.
- Les têtes de devoirs concordent jusqu’au slot 18, divergent aux slots 19–28, concordent aux slots 29–34, puis divergent à nouveau.
- Un vote ancien admis sans changement de dernier message peut déjà avoir été appliqué via un bloc ; `changedMessages=0` ne prouve pas une perte de vote. Les compteurs du relais incluent aussi des racines déjà acquittées.

## Prochaine investigation

Tracer les tentatives de livraison des blocs et votes avec horodatage, racine, dépendances et motif d’échec avant fermeture du catalogue. Relier les périodes de retard aux refus/reprises du relais et à la disponibilité des blocs référencés. Ne pas changer la règle de poids ou prolonger le délai pour masquer cet échec.

Les journaux complets et l’analyse par nœud sont conservés. Les bases temporaires ont été supprimées par le harnais à la sortie : elles ne sont pas disponibles pour rejouer directement l’état. Cette preuve ne qualifie ni les partitions, ni le WAN, ni la production.
