# Mesurer le stockage Nano hors ligne

Ce helper C++ ouvre chaque base Nano en lecture seule et vérifie les checksums.
Il affiche les nombres d'entrées et tailles logiques par catégorie, les plus
grandes valeurs et les tailles apparentes des fichiers. Il n'affiche aucune clé
ni valeur, ne signe pas, ne compacte pas, n'élague pas. Les liens symboliques
sont refusés et le logger RocksDB n'écrit aucun fichier.

Compiler avec les mêmes headers/librairie RocksDB que le lecteur supporté et
`third_party/json.hpp`, puis lancer sur le répertoire data d'un nœud arrêté.
Exemple Mac :

```sh
c++ -std=c++20 -O2 -Ithird_party -I/opt/homebrew/include tools/storage-profile/profile.cpp -L/opt/homebrew/lib -lrocksdb -o /tmp/nano-storage-profile
sandbox-exec -p '(version 1)(allow default)(deny network*)(deny file-write*)' /tmp/nano-storage-profile /PATH/TO/OFFLINE/DATA
```

Vérifier les empreintes de tous les fichiers avant/après et conserver le code
terminal. Les rapports de comptabilité ne prouvent ni consensus, ni finalité,
ni reprise de duties. Aucun choix automatique de données supprimables : le
pruning de l'état historique doit être intégré à l'ancre finalisée et au rejeu.
