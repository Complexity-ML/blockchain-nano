# Pair de qualification Altair : réponse aux sondes de statut

Un cas original `consensus_altair_libp2p` passe sur AMD64 en 4.36831 secondes, zéro skip, sorties CTest/conteneur à zéro. Le daemon original, les règles d’admission, la limite de bloc et le délai de 120 secondes restent inchangés. Core c10426b2 et binaires/bibliothèques historiques réutilisés en lecture seule, aucune image reconstruite.

Le pair de qualification ne répondait pas à `/nanopy/chainsync/1.0.0`. La trace de refus seulement montre `unknown-peer` sur le flux plein. `poll_status` supprime un pair dont le statut est invalide/indisponible ; le grand message rend cette lacune du pair observable. La correction lui fait renvoyer exactement son annonce de handshake, y compris les annonces incompatibles des scénarios négatifs. Aucun statut valide fabriqué pour contourner une incompatibilité.

Le contrôle utilise le probe de transport instrumenté et le test diagnostic déjà compilé, avec le daemon original non instrumenté. Le changement fonctionnel du probe est intégré dans `network/altair_probe.cpp`, sans les traces de transport. Les manifestes sources, programmes et bibliothèques sont identiques avant/après. Le contrôle précédent en échec reste dans `../daemon-comparison/`.

Portée : correction du pair de test et passage du scénario Altair borné sur AMD64. Pas de campagne complète rejouée, pas de version candidate cohérente promue, pas de qualification générale du consensus ou de la production. Les autres huit échecs historiques AMD64, WAN, bridge et revue indépendante restent ouverts.
