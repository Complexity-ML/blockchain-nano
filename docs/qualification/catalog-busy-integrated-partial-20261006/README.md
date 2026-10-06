# Reprise catalogue : qualification partielle conservée — 2026-10-06

Base main eb38f4c8 ; modifications P2P non commitées, sources exactes figées dans integrated/source. Cœur ee5e278b inchangé. Aucune image reconstruite ; images existantes, réseaux Docker isolés, aucun nœud public ni fonds.

Les sessions originales sont terminales : natif 2780/0 (25/25), ARM64 3458/8 (24/25), AMD64 1514/0 (25/25). Les 76 sources sont vérifiées avant sur les trois chemins et après sur les deux réussites. Le contrôleur ARM s'arrête sur CTest : empreintes binaires et contrôle après manquent dans cette campagne, explicitement non qualifiée. Les diagnostics séparés ne remplacent pas cette campagne.

Le test HTTP neuf couvre neuf scénarios avec Core et le wrapper head/catalogue : capture occupée puis disponible, erreur inconnue, occupation persistante bornée, réponse perdue/tardive et handle tardif nettoyé. Même identité de requête ; aucune reprise des réponses ambiguës. Backend simulé/cataloque vide : pas une qualification de capture moteur ni de finalité WAN.

L'échec original capella_dependency_preparation_slow admet 4 blocs après 20 tours/50 exports, sans ACK de la tête. Le diagnostic ARM 4442/0 trace l'ancien et le nouveau code à 40 ms : les quatre runs passent ; l'ancien mode duplicate atteint exactement 20 tours. Cela ne répare ni n'explique l'échec original.

Le contre-exemple ARM 97772 termine son collecteur à 0 mais ses DEUX tests retournent 1 : ancien et nouveau à 60 ms, mêmes 100 ms/tour, 20 tours, 3 exports et 2 admissions maximum, tous deux 38 exports/2 admissions, tête non acquittée. Aucun plafond augmenté. Il démontre que la borne de tours dépend du délai et de la répétition du parcours ; il ne prouve pas la cause précise du premier échec à 40 ms. Sources, traces, commandes et hashes binaires conservés ; chemin Docker initial absent (127) et montage JSON omis (compilation 1) conservés ci-dessous.

Prochaine action : caractériser la progression et la réutilisation du parcours local authentifié sous délais, conserver les règles d'ACK et les réponses tardives, puis qualifier le correctif concerné. Lot NON entièrement qualifié ; aucune campagne WAN relancée, V26 FAILED et revue indépendante ouverts.
