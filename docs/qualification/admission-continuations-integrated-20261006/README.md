# Admission continue d'un bloc signé — qualification ciblée

Source de base `bed17913`, sources modifiées figées dans `sources/` (344 empreintes). Le contre-exemple précédent est conservé dans `../block-receipt-deferral-baseline-20261006/`.

Le cœur distingue admission, rejet et préparation différée. Le worker conserve le même travail, son annulation et sa réservation, puis reprend à la fin de la file. Aucun reçu ni ACK n'est émis avant une décision terminale. La validation/signature, les engagements et les limites restent applicables.

Qualification native, Docker ARM64 et Docker AMD64 : interférences/alternances, bloc préparé (146 vérifications, deux variantes signées, une seule soumission), wire/reprise, runtime configuré, reprise automatique et HTTP ; quatre tests SDK ciblés et compilation du relais. Sur le bloc retenu slot 10751 du contre-exemple, le reçu initial accepte sans 21 resoumissions manuelles, en natif et ARM64. Transport et horloge sont modélisés pour ce contre-exemple ; ceci ne qualifie pas le WAN.

Échecs conservés : include de scan secp256k1 manquant, ancienne assertion attendant des resoumissions, et AMD64 RocksDB `Too many open files` avec la limite Docker par défaut de 1024. Seuls reprise automatique, HTTP et SDK sont repris après la sortie terminale AMD64, avec `nofile=65536`, sans recompilation du cœur ni reconstruction d'image.

La finalité commune après restauration reste ouverte. Il reste à relancer une campagne privée avec les nouveaux binaires ; ni EVM de forks non activés, ni charge, ni production ne sont qualifiées par ce lot. La candidate utilise encore l'instrumentation de faute de synchronisation ; une version release doit être qualifiée séparément. Aucune base ni clé transférée entre hôtes ; les originales locales sont contrôlées inchangées.
