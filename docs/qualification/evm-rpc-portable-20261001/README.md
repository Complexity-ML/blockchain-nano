# Candidat EVM/RPC/crypto — qualification partielle

Candidat ea9667f638facbc7f7427c949660a2cf0b310abd, ARM64 Docker : 14 tests exactement sélectionnés, 14 réussis, zéro ignoré. Résultats XML et sélection identiques ; neuf empreintes exécutables identiques avant/après. Durée 570,97 s. Le profil couvre cœur/RPC/HTTP/admission, BN128 à froid, huit scénarios de charge RPC Paris/Shanghai et le sous-ensemble EEST enregistré. Il ne prouve pas une conformité EEST universelle ou l’activation réseau Cancun/Prague.

AMD64 : campagne du même candidat en cours au moment de cette archive. Ne pas promouvoir le résultat ARM64 en preuve AMD64 ou production.

## Nouveau parcours volumineux local

Branche qualification/oversized-lifecycle-20261001, commit b39a0122 : dix transactions, bloc compact de 18 003 843 octets, finalisation avec journal réel sans certificat substitué, annonce par référence, reconstruction en 40 fragments, import authentifié et racine d’état exacte. Paris et Shanghai réussissent ; limite des trames conservée à 16 777 216 octets, ancienne exportation agrégée refusée. Compilation isolée contre les bibliothèques du candidat montées en lecture seule, empreintes avant/après égales. Le binaire est identifié séparément. Aucun réseau libp2p n’est utilisé par ces deux tests.

Première préparation arrêtée avant exécution : Python absent de l’image. Script/log conservés dans preparation-a ; préparation corrigée avec les outils existants, sans installation. Les anciens échecs des tests volumineux restent dans la campagne complète aacc56da ; cette preuve ne les réécrit pas. Le transfert P2P distribué et la régression complète restent à qualifier sur le candidat final.

Le nouveau parcours volumineux local b39a0122 passe aussi sur AMD64 : Paris/Shanghai exit 0, 40 fragments, état audité exact, empreintes bibliothèques avant/après identiques. Sources obtenues par Git. Cela ne ferme pas le transport P2P distribué.

## AMD64 terminal — ea9667f6

14 tests exactement sélectionnés, 6 réussis / 8 échecs / zéro ignoré, exit CTest 8, empreintes des neuf exécutables avant/après identiques. Tous les huit scénarios RPC de charge échouent ; cœur/RPC/HTTP/admission, BN128 à froid et EEST réussissent. EEST : 317 fichiers, 5 387 correspondances au total, dont 4 882 exécutions et 505 validations, zéro divergence/non supporté/erreur ; forkConformanceProven=false. Les trois divergences BN128 de l’ancienne révision 8a8b080e sont absentes de ce sous-ensemble sur le VPS Haswell sans ADX. Cela ne ferme ni le débit AMD64 ni la conformité universelle. Journaux/XML/sélection disponibles dans amd64-terminal.
