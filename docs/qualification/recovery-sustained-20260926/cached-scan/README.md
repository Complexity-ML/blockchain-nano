# Parcours borné des votes déjà acquittés

Les traces précédentes montrent une stagnation des derniers messages distants
pendant que les votes locaux continuent. Le relais ne retransmettait plus les
votes connus, mais consommait encore un intervalle de planification par vote
connu lors des reprises de catalogue.

Une tentative poursuit maintenant le parcours asynchrone jusqu’à 32 entrées
connues (budget de travail configurable au constructeur, sans rapport avec
le nombre de validateurs). Le délai global reste actif. Au premier vote inconnu,
le chemin normal export/décodage/envoi/acquittement est conservé.

Test ciblé réussi en 0,32 s : préfixe de 70 votes acquittés suivi d’un nouveau
vote atteint en trois tentatives, rendement au budget, absence de renvoi du
préfixe, perte de réponse du nouveau vote et réessai. Les cas précédents de
rejet/session/éviction/dépendance de bloc restent exécutés.

Le binaire P2P est reconstruit. La partition prolongée avec mêmes seuils est
terminée en échec en 373,12 s, dès la première lecture du moniteur.
Les reçus d’arrêt ont pourtant tous deux finalisé 6 et justifié 7.
Sans les valeurs de la lecture fautive, ce run reste un échec conservé.
Une course entre les lectures du checkpoint et de la tête est suspectée ;
elle est reproduite indépendamment dans le test du moniteur.

Les traces du source aux slots 61, 62, 63 contiennent respectivement 20, 23,
27 derniers votes distants de l’époque courante sur la branche signée ;
l’observateur en connaît 20, 25, 29. La stagnation observée précédemment
(19 votes conservés plusieurs slots) n’est plus présente sur cette fenêtre.
Cela prouve une amélioration locale de fraîcheur, pas encore la stabilité
prolongée du réseau.
