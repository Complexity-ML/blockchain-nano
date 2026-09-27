# Échantillons de frais réutilisés par hash de bloc

Le cache précédent évitait les calculs sur une tête inchangée, mais relisait
vingt blocs après chaque nouvelle tête. Le cache supplémentaire retient les
pourboires sélectionnés, le parent et la hauteur de chaque bloc de la fenêtre.
Il utilise le hash complet, jamais la seule hauteur.

## Invariants

- La comparaison des adresses du producteur demeure normalisée.
- Une nouvelle tête réutilise uniquement les ancêtres communs par hash.
- La fenêtre retenue remplace la précédente après vérification de la tête.
- Au plus `history_blocks` entrées sont persistées en mémoire, chacune contenant
  au plus `samples_per_block` pourboires ; une seconde fenêtre candidate existe
  temporairement pendant le calcul. Aucun bloc complet n’est conservé.
- Tout rechargement valide de la politique vide les deux caches.
- Échec de lecture ou changement de tête : aucune publication partielle.

Le test fonctionnel ajoute une branche à pourboire 900, revient deux hauteurs
en arrière puis remplace cette branche par un bloc à 300. Les réponses attendues
sont 700 avec l’ancêtre commun, puis 300 après son expiration, puis 0 après
expiration du dernier bloc non vide. Le test de charge compare aussi un nouvel
estimateur sans cache à l’estimateur incrémental sur exactement la même tête.

## Résultats

Cinq tests affectés passent en 9,28 s : `rpc`, `rpc_fee_policy`, `rpc_fee_load`,
`wallet_cli`, `rpc_cli`. La variante de charge à 1 428 transactions par bloc
passe également : vingt blocs initiaux (28 560 transactions) puis un nouveau
bloc (1 428 transactions), tous à 29 988 000 gas utilisés.

Après ce nouveau bloc : **33,518 ms** avec réutilisation, contre **671,225 ms**
avec un nouvel estimateur sur la même tête (environ 20 fois moins de temps).
Les deux retournent 101 wei. Les 1 000 lectures en cache prennent 5,638 ms.
Les trois calculs entièrement sans cache de recommandation restent à
692,745 / 667,644 / 672,027 ms. Le scénario vérifie l’absence de mutation de
la chaîne ou de la mempool par les consultations.

## Portée

Mesures locales macOS, RPC en processus, base RocksDB temporaire ; caches disque
non purgés. Le run volumineux et la suite ciblée ont été lancés avec chevauchement :
ces chiffres ne sont pas des garanties temporelles ni une comparaison entre machines.
La comparaison incrémental/neuf est faite dans le même processus sur la même tête.

Le premier calcul et les nouvelles branches absentes du cache restent coûteux
et détiennent le mutex RPC. Pas de preuve d’équité, de saturation HTTP/P2P,
de charge WAN ou de réseau Beacon ici. La fenêtre maximale de 1 024 blocs et
la course effective avec réorganisation pendant un calcul ne sont pas testées.
Un échantillon déjà en cache peut survivre à l’élagage disque du bloc correspondant,
mais uniquement tant que son hash appartient à la fenêtre retenue.

Aucun changement des frais de consensus et aucune clôture de production.
