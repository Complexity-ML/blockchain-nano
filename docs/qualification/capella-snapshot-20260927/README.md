# Snapshot et restauration Capella

Le format local nano-capella-store-v1 conserve branches, états SSZ/EVM,
reçus, ancêtres d'exécution, horloge, votes, équivoques et checkpoints. Le
profil nano-capella-profile-v1 engage le socle existant, les trois paramètres
de retraits, la version genesis BLS et la limite des changements BLS. Le
chargeur authentifie le digest avant parsing et exige un encodage canonique.
Le registre justifié est recalculé depuis SSZ, jamais accepté comme copie.

Qualification sur les trois chaînes de neuf blocs : snapshots canonique et
avec branche concurrente, aller-retour exact, cache froid/chaud identique et
moins de callbacks à chaud. Mauvais digest, règles de balayage différentes
et état EVM muté sous les mêmes identifiants sont refusés, même avec cache.
Quatre contrôles de profil par chaîne : mauvais digest, budget insuffisant,
version genesis incompatible et limite de changements BLS nulle.

Chaque chaîne est ensuite commitée avec son vrai snapshot dans un batch
RocksDB consensus/exécution, fermée, rouverte et restaurée. Le token externe,
la tête et la racine d'état EVM doivent concorder. Régression chaîne Bellatrix,
chaînes Capella/snapshots et checkpoint Shanghai : 3/3 suites.

Limites : il s'agit de checkpoints LOCAUX déjà authentifiés, pas d'admission
réseau ni de protection contre le rollback du token externe. Le test disque
est une fermeture/réouverture normale du vrai Store ; les SIGKILL Shanghai
antérieurs utilisent leur propre snapshot synthétique. Il reste à combiner
ces deux campagnes, qualifier votes/finalité, conserver les branches Bellatrix
à travers le fork et raccorder le réacteur/réseau. Aucun profil actif changé.
