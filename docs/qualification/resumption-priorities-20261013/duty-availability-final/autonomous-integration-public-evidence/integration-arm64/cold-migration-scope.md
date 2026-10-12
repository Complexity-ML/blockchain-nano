# Variante de stockage froid du test synthétique

La variante utilise uniquement `migrate_reference_checkpoint({2, 1048576})`, API existante du cœur, sur un répertoire synthétique neuf avant le premier tick. Aucun profil de consensus, état économique, budget, règle de signature ou limite n'a été assoupli.

Le test exige une égalité byte-for-byte du Store canonique avant/après migration, ainsi que des mêmes profil, identité réseau, bloc et comptes d'exécution. Il vérifie séparément la publication primaire et celle du témoin et le lien exact `previous`/`next`. Le format froid porte `cache=["0x2","0x100000"]` et les lignes publiées sont des références froides. Ces chiffres sont des bornes configurées sur les enregistrements encodés ; l'API ne permet pas d'observer directement la consommation du cache LRU privé. La consommation RSS est mesurée séparément et reste plafonnée à 131072 Kio.

L'échec antérieur en format plat demeure conservé et ouvert dans `legacy-flat-rss-refusal` : 137524 Kio, 117 signatures synthétiques et 39 blocs construits avant le refus de ressource. Une éventuelle réussite froide ne qualifie pas ce chemin plat.

La nouvelle exécution cible conserve 64 validateurs actifs initiaux, 48 blocs pairs validés par l'admission ordinaire, un plafond de 160 signatures synthétiques, 60 secondes et 128 Mio. Elle n'utilise ni clés de campagne/V37, ni réseau, ni runtime de nœud et ne constitue pas une qualification de production.
