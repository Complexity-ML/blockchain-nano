# Variante de sérialisation Bellatrix — non promue

Core historique c10426b2, daemon/règles de consensus inchangés. Le JSON déjà canonique du store est inséré dans le document extérieur plutôt que reparsé puis réencodé. Les clés extérieures format/signed* précèdent store dans l’ordre canonique nlohmann ; la limite finale est vérifiée par soustractions avant allocation, sans overflow. WAL et écritures synchrones inchangés.

## Équivalence ciblée ARM64

Image Docker ARM64 existante, compilation C++ du fragment exact extrait de la variante et comparaison à l’ancien `Json::parse`/`dump`. Source et fragment identifiés par SHA256. Formats durables v2-v5, 256 documents synthétiques par format : 2 048 comparaisons byte à byte identiques (limite exacte et SIZE_MAX), 3 072 rejets aux limites 0/store trop grand/enveloppe trop courte, 1 024 annulations propagées. Caractères Unicode/échappements, entiers 64 bits, tableaux et valeurs imbriquées inclus.

Ce contrôle qualifie uniquement l’assemblage canonique extrait et ses limites : pas l’authentification SSZ, la reprise complète, les archives v1 inchangées, ni la production.

## Partition AMD64

La variante a terminé son exécution dans `nano-bellatrix-snapshot-splice-control-amd64-test`, scénario original 112 slots et timeout 240 secondes. Objet du test et dépendances réutilisés, deux objets de diagnostic recompilés. Résultat final : timeout à 240,022 s, dernier slot de phase 96, zéro skip, sorties CTest/conteneur 8. Rapports VPS `/home/ubuntu/nano-bellatrix-snapshot-splice-control-20261002-reports`.

Aucun runtime principal modifié ou promu. Tous les échecs historiques et critères de production restent conservés.

## Comparaison mesurée et limites

Au même compteur de 1 792 publications, encodage 44,416 s contre 70,948 s (−37,4 %), publication inclusive 109,663 s contre 137,117 s (−20,0 %). Les séquences de scénario sont identiques mais ces valeurs ne constituent pas un benchmark répété ni une généralisation WAN. Le dernier slot observé progresse 88→96, sans atteindre le scénario terminal de 112 slots et ses assertions. Le cas reste donc en échec.

Tous les manifestes sources/programmes/bibliothèques sont identiques avant/après, post-manifest à zéro. Aucune promotion runtime et aucun critère de production clos. La construction JSON du snapshot recopie encore ses grands tableaux locaux lors de la construction de l’objet final ; une variante suivante transfère uniquement ces temporaires, sans déplacer les données du store authentifié.
