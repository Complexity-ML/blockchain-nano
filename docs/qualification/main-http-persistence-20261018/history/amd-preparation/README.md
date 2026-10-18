# Préparations AMD avant le batch main HTTP

Ces reçus sont des préalables. Aucun d’eux ne démarre Nano ou ne fait de requête
HTTP. Ils ne constituent pas un PASS du batch main/serve. Chaque prédécesseur
FAILED et son successeur sont conservés séparément, avec stdout/stderr/exit et
source publique exacte. Aucun contrôle/autorité complet ni provider binaire n’est
copié : les pins de contenu peuvent être cités sans exporter leurs tables.

* **Préflight01, Root70652 exit1** : KeyError docker après lecture du contrôle
  privé qui n’avait pas ce champ, avant toute commande Docker. Pas de runtime.
* **Préflight02, Root43355 exit0** : Nano C6 correspond à sa ligne produit
  originale ; les outils Git/Docker/Sudo correspondent aux lignes originales,
  mainbc99 reste propre et l’imagefd9 AMD64 est inspectée. Aucun nouveau Nano.
  Le binaire retenu est cc0eed49d62d9decccf128b773241191d9ef39abc4cb2c90f876a4a8f9ece94b,
  13997336octets. Aucune ancienne FD9 propre du JSON866 n’est revendiquée.
* **Probe imagePython, Root39928** : le script d’observation termine exit0,
  mais le consommateur/conteneur termine exit1 et consumerAvailable=false :
  /usr/bin/python3 n’est pas disponible comme exécutable dans cette image.
  Le conteneur possédé est absent ; Nano0, HTTP0, buildimage0. Ce probe n’est
  donc pas converti en succès Python.
* **Producteur provider01, Root39240 exit1** : la lecture exigeait une taille
  strictement positive et a refusé un fichier vide de la bibliothèque standard.
  Il reste FAILED ; aucun runtimePython/Nano exécuté, aucune dépendance installée.
* **Producteur provider02, Root71158 exit0** : les fichiers vides sont pris en
  charge ;628fichiers, dont cinq vides,31232658octets,51répertoires et trois
  aliases source matérialisés. Les octets et trois observations d’admission sont
  contrôlés, le bytecodecache est exclu. Zéro installation/compilation ; le runtime
  Python n’est pas exécuté par ce producteur. Son contrôle6d1969aa…/241262 B
  reste sur le VPS, hors de ce paquet public.
* **Consommateur provider, Root94498 exit0** : le runtimePython3.12.3 est monté
  en lecture seule dans l’imagefd9 inchangée. Les imports stdlib/HTTP nécessaires
  sont disponibles ; consommateurs et conteneur exit0, providerfencePASS et
  conteneur possédé absent. Aucun Python installé dans l’image, aucun build,
  Nano0 et HTTP0. Cela prouve ce consommateur, pas une fermeture SDK globale.

La source publique du producteur décrit l’algorithme de copie et d’admission ;
elle ne contient pas la table de628autorités réellement générée. Le contenu
privé control.json, source-provenance.json, ses samples et tout JSONproducteur
complet sont exclus. Les result-public.json sont les petits résumés sélectionnés
explicitement par Root, conservés sans réécriture.

Mainbc99 et les anciennes sources C6 demeurent identifiés séparément des étapes
réelles de ce futur batch. Le périmètre attendu reste PoW synthétiqueShanghai,
57HTTP, trois serveurs/deux mine et cinq preuves ; pas Beacon/réseau/production.
