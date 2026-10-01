# RPC client et isolation du budget d'admission — 2026-10-01

Correctifs dans checkout isolé ; branche privée qualification/rpc-admission-20261001,
révision 9861d94c4379f67eea989dcc166840fd3a6faf9c. Sources main de la campagne
400 non modifiées ; originaux nano bbe09460…/nano-p2p ae567df0… préservés.

## Paramètres positionnels et commandes réelles

Source 97f4a19d, nano d5c40202… reconstruit après retrait de l'expérience GMP.
Supervisor, RPC CLI, staking CLI et validateur signé passent en 11.43 s.
Les contrôles incluent identité chain/genesis, refus de templates corrompus,
SIGKILL/récupération de la même preuve de signature, audit et progression.
Le premier lancement avait fini exit 128 avant compilation : le common-dir
Git du worktree n'était pas monté. La révision propre est ensuite vérifiée
sur l'hôte ; hashes source constatés dans le conteneur. Montages non élargis.

## Cause et correctif d'admission directe

eth_sendRawTransaction chargeait le budget global avant le rejet wrong chain ID.
Nouveau test déterministe avec horloge gelée : l'ancien runtime échoue exit 8.
La prélecture RLP canonique identifie le chainId avant consommation du budget.
Taille d'entrée (1 Mio) et profondeur RLP (64) restent bornées. La prélecture
ne peut jamais admettre une transaction ; le décodeur et l'ECDSA complets restent
obligatoires après débit du budget pour une enveloppe locale. Pas de changement
aux règles consensus, aux signatures, au gas ni aux limites 128/64 par seconde.

384 enveloppes étrangères de types 0/1/2, nonces variés, sont rejetées sans
mutation d'état/événements/pool ; une transaction saine reste admissible.
127 signatures locales high-s après cette admission épuisent réellement le
budget ; refus -32005 puis récupération après remplissage vérifiés.
Le test RPC existant et ce nouveau test passent (1.17/0.04 s).

## Scénarios réseau bornés

Le scénario legacy BFT NanoExperimental avec restart/partition/charge passe
22.70 s ; 149062 rejets étrangers, 149066 rejets BFT malformés, 298128 lectures
saines, zéro réponse inattendue. Les contrôles exigent la transaction saine
finalisée sur les quatre validateurs pendant la charge et la même réserve de
vote après restart. Tous les workers/processus s'arrêtent à la fin.
Deux nouveaux enregistrements explicites Paris/Shanghai passent 22.56/22.60 s,
mêmes assertions et timeout 120 s. Ils ne qualifient pas le consensus Beacon
Capella du réseau production ni une attaque WAN générale.

Le candidat ARM64 nano c6197c3e… a également ses quatre CLI requalifiées après
la modification d'admission : 6.29/0.43/2.37/2.19 s. Le transport reste ae567df0….
Les sélections exactes, assertions et absence de skip sont vérifiées.

## Limites et suite

La file de gossip nano_receiveTransactions n'est pas modifiée et reste à
examiner ; parsing hostile, transactions malveillantes de même chaîne, fairness
sous charge générale et RPC d'estimation restent ouverts. Ce correctif limité
n'est pas une qualification DoS générale, EVM complète ou production.
La régression 400 originale continue avec tous ses échecs historiques.

Qualification AMD64 démarrée sur VPS depuis Git, checkout distinct
/home/ubuntu/nano-rpc-admission-20261001, builder existant 0fa8a608… et volume
nano-linux-sync-build ; build distinct /build/rpc-admission-amd64-20261001.
Huit contrôles exacts prévus. Aucun résultat terminal encore revendiqué,
aucune nouvelle image construite ni nœud public déployé. Journal accessible :
/home/ubuntu/nano-rpc-admission-amd64-20261001.log.
