# Correction du cas d’ouverture existante

Le premier CTest réel ARM64 et AMD64 a refusé le banc avant sa première ligne PASS : le banc supprimait state/0 bien que le tip couplé soit 1. Blockchain Existing exige state_at(latest.header.number), pas tout état historique. Ces deux échecs et leurs JUnit restent conservés.

Cette version de test seulement dérive la clé canonique depuis meta/tip, exige que chaque clé adverse existe avant Delete, conserve quatre bindings manquants et tous les refus d’initialisation. Un contrôle distinct dans la même famille supprime seulement state/0 historique et exige le vrai plan CLI accepté, avec l’état courant présent et les lignes protocolaires inchangées. Aucun nouveau CTest, aucun changement de CLI/cœur/CMake, aucune réduction de bornes ni de cas.

La reprise interrupted-WAL du wrapper reste ouverte. Les trois migrations hot/cold/fragmented, trois fautes fsync de rétention et 13 lignes sémantiques restent identiques en nombre ; la première ligne ajoute trois discriminants explicites. Proposition non compilée/exécutée par son auteur.
