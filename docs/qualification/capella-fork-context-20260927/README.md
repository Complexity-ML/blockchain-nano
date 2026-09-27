# Contextes de fork-choice Capella

Les helpers Capella authentifient le post-état contre le header du bloc,
son slot, le latest_block_header et le marqueur d'époque. Ils extraient le
registre, RANDAO et les domaines de fork ; calculent les checkpoints réalisés
et unrealized (pull-up FFG) ; avancent l'état cible avec les slots Capella ;
dérivent le proposant à partir du parent authentifié. Le pull-up préserve
header d'exécution, curseurs de retrait et résumés historiques.

La projection suit explicitement le schéma Capella. Ces helpers ne prouvent
pas l'origine d'un header arbitraire : ils reçoivent un bloc déjà authentifié.
Ils ne publient aucun état et n'admettent pas un snapshot réseau.

Tests sur les trois chaînes signées existantes : continuation, migration à
la frontière exacte, migration avec trois slots sautés. Pour chacun des 27
blocs, le registre correspond au solde post-retrait et aux credentials mis à
jour ; les checkpoints réalisés concordent avec SSZ et un mauvais header
est refusé. Six contextes cibles et prochains proposants sont contrôlés.
Trois cas refusent le proposant sélectionné slashed sans reroll ; trois
annulations sont propagées. Les anciennes validations de blocs restent actives.

Le test initial lisait un offset de balance conservé avant l'ajout de votes
Eth1 : ce pointeur de test était périmé. Il lit maintenant l'offset dynamique
du post-état SSZ. Aucun changement consensus n'a été fait pour masquer ce cas.

Régression Bellatrix signé, Capella signé/contextes et migration officielle :
3/3. Le Store complet, son snapshot, le profil durable, le réacteur et la
qualification réseau Capella restent ouverts. Aucun statut production acquis.
