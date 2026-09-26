# Révision des propositions validateurs

Vérification du 27 septembre 2026, dépôt officiel Ethereum/EIPs, commit
`0b8184b1d6ed9fba836222684fd082b32782d4ef`. `sources.json` conserve les URL
immuables, statuts, dépendances déclarées et SHA-256 des fichiers consultés.
Ces sources épinglées ne constituent ni une implémentation ni une activation.

| EIP | Statut source | Dépendance déclarée | Critères spécifiques avant clôture Nano |
|---|---|---|---|
| [8045](https://eips.ethereum.org/EIPS/eip-8045) | Review | 7917 | Prévision des proposeurs intégrée à l’état et qualifiée ; filtrage des slashés avant sélection ; liste future complète ; un slashing ultérieur ne réécrit pas la prévision déjà stockée ; transition de fork explicite. |
| [8068](https://eips.ethereum.org/EIPS/eip-8068) | Draft | 7251 | Prérequis Electra/MaxEB qualifiés ; nouveaux champs temporaires initialisés à la migration, persistés et hachés ; hystérésis, arrondis et réinitialisation après retraits comparés à des vecteurs de référence ; neutralité économique évaluée sur séquences, pas supposée à partir du nom. |
| [8363](https://eips.ethereum.org/EIPS/eip-8363) | Draft | Aucune dans l’en-tête | Déductions par devoir assigné, règle d’inactivity leak et transition du facteur de récompense ; calcul entier et bornes vérifiés ; comptabilité émission/destruction et conservation des autres pénalités ; paramètres de supply/epochs Nano explicitement spécifiés avant activation. |

Pour 8363, l’absence de `requires` ne supprime pas les dépendances fonctionnelles
au traitement des récompenses, aux devoirs et aux états du consensus. Les
constantes exprimées pour ETH ne doivent pas être copiées comme paramètres
économiques NANO sans spécification. Une adaptation Nano serait documentée
comme telle, distincte d’une conformité exacte à la proposition Ethereum.

Les trois restent **non commencées** dans la matrice d’implémentation.
L’intégration se fera dans un profil expérimental distinct ; aucun changement
du réseau Bellatrix actuellement qualifié. Les critères ci-dessus sont un
plan de vérification, pas des tests déjà exécutés.
