# Audit canonique hors ligne Capella — qualification native

La nouvelle commande `nano audit-capella genesis.json base-arretee request.json`
ouvre RocksDB en lecture seule. Le fichier de requête fournit le profil et son
SHA-256, l'ancre et son SHA-256, les limites de snapshot, un engagement de checkpoint
conservé indépendamment (`sequence`, `digest`), ainsi que `maximumBlocks` (hors ancre)
et `maximumArchiveBytes`. Les fichiers doivent provenir d'une source de confiance ;
calculer les engagements depuis une base suspecte ne l'authentifie pas.

Le rejeu remonte les archives signées jusqu'à l'ancre, puis vérifie les signatures,
transitions Beacon/EVM, états canoniques, index par hauteur/hash, reçus et index des
transactions. Les états retenus du checkpoint sont confrontés au rejeu. Le budget
d'archives et le nombre de blocs sont bornés ; la base doit être arrêtée ou être
un snapshot cohérent. Aucun réseau, horloge active ou signature de validateur.

## Résultats

Le scénario de finalisation natif avec audit passe en 33,76 s. Il refuse aussi
un budget de blocs insuffisant et un engagement de checkpoint incorrect.
Sur la base native arrêtée de la tentative WAN C : 99 blocs (ancre incluse),
97 transactions, hauteurs 1 à 99, 283315 octets d'archives signées rejoués.
L'engagement provient du reçu d'arrêt de C, pas d'une lecture de la base auditée.
Les SHA-256 de tous les fichiers restent identiques avant/après.

Quatre copies corrompues sont refusées : archive absente, signature altérée avec
checksum de stockage recalculé, reçu historique modifié, index de transaction
historique modifié. Chaque copie reste inchangée après son audit. L'outil
`nano-capella-audit-corruption-tests genesis.json base-arretee request.json`
reproduit ces cas sur une fixture contenant une transaction historique hors ancre/tip.

## Limites ouvertes

Le rapport affirme `canonicalHistoryVerified`, pas un audit de l'historique
complet du consensus : `historicalForkChoiceVerified` et
`historicalStandaloneVotesReplayed` restent faux. Les branches non canoniques,
les votes autonomes historiques, la provenance de l'ancre et l'historique avant
l'ancre ne sont pas rejoués. Les règles utilisées sont celles du profil fourni.
L'audit peut traverser un checkpoint élagué tant que ses archives canoniques
signées et l'ancre de départ sont disponibles. Il refuse les archives manquantes.
Qualification Linux ARM64/VPS, audit des branches/votes historiques et intégration
à la procédure de sauvegarde/restauration restent requis. L'arrêt A reste inexpliqué.

Le premier build a échoué sur le passage d'un stockage const à l'auditeur ;
le test utilise désormais une ouverture ReadOnly distincte. Ce log est conservé.
Une préparation de requête a aussi rencontré une ligne non JSON dans le log :
la sélection du reçu d'arrêt ignore maintenant les lignes texte. Aucun test réussi
n'est attribué à ces tentatives de préparation.
