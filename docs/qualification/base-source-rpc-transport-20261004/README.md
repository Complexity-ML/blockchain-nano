# Transport RPC source borné — 4 octobre 2026

Base main fc15b15f, fichiers et commande exacts dans manifest.json.
Validation ciblée terminale exit 0 : 39/39, aucun échec/skip (tests.log).
15 nouveaux tests exercent de vrais serveurs HTTP loopback : identifiants
concurrents, résultat null, mauvaises enveloppes/version/ID, erreur fournisseur,
limites déclarées et chunked, corps bloqué avec timeout, redirection sans retry,
HTTP 503, JSON/UTF-8 invalide et configuration/méthodes/requête refusées.
24 tests existants vérifient l’observateur source avec RPC injecté.

Aucun fonds, signature, endpoint Base réel, mint, déploiement ou image Docker.
La borne de temps s’applique par requête, pas à toute l’observation.
La taille est bornée sur les bytes décodées par Fetch ; pas sur le trafic
compressé réseau ni une preuve de résistance d’un service public sous charge.
La cohérence RPC n’authentifie pas le consensus Base/L1 ou l’inclusion du reçu.
Finalité cryptographique et éligibilité au mint restent false.
Les preuves interchaînes, journal atomique, couverture, retraits, compromission
et termes Base–Nano restent ouverts. Candidate C++ efbeb9bc inchangée.
