# Retrait du patch notices proposé — revue source

La proposition `cli-readonly-install-notices-source-proposal-20261018-01` est REFUSÉE EN REVUE SOURCE et ne doit pas être appliquée. Le diagnostic initial « seulement huit notices installées » était faux : la lecture avait omis les trois règles inconditionnelles déjà présentes dans le CMake public, lignes571–573, après les targets blob/KZG :

```cmake
install(FILES third_party/secp256k1/COPYING DESTINATION ${CMAKE_INSTALL_DATADIR}/nano/licenses RENAME secp256k1-LICENSE)
install(FILES third_party/ckzg/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/nano/licenses RENAME ckzg-LICENSE)
install(FILES third_party/ckzg/blst/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/nano/licenses RENAME blst-LICENSE)
```

Ces règles complètent déjà les huit notices de la section399–412 et couvrent les onze noms du manifeste. Les hashes des trois sources vendored étaient exacts, mais cela ne démontrait aucun défaut d’installation. Le candidat b1545f10…/180310 et son patch19b4f75b…/1072 ne font qu’ajouter trois doublons ; leur README comporte donc une conclusion incorrecte. Ils restent conservés comme proposition historique refusée, jamais comme correctif ou échec de compilation/install observé.

Root a confirmé ne pas avoir appliqué le patch. Aucun fichier R, source C++, image, contrôle ou ancienne autorité n’a été modifié par l’auteur. Les payloads annoncés stables du paquet refusé restent immobiles ; la présente note séparée les invalide pour l’application. La génération/install réelle Native reste à qualifier sur le CMake C8 existant inchangé. Aucun nouveau défaut de notices n’est revendiqué.
