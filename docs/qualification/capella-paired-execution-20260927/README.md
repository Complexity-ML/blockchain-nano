# Capella : candidat consensus/exécution couplé

`prepare_capella_payload_state` authentifie le parent SSZ Capella, relie
son dernier header au parent EVM, déduit timestamp et RANDAO de l'état,
calcule et compare les retraits autorisés puis prépare leur débit et
l'exécution Shanghai. Le nouveau header et les curseurs sont réunis dans
un état privé ; les historiques SSZ sont préservés et leur offset ajusté.
Aucune écriture en base n'est effectuée par cette API.

Le pont d'exécution commun conserve les vérifications Bellatrix et ajoute
le profil Shanghai : transactions types 0/1/2, MPT des retraits, hash du
bloc, engagements EVM, horodatage et parent. L'API EL seule n'autorise pas
les retraits : cette autorité vient de la transition consensus englobante.
Les erreurs de callbacks d'historique ou d'annulation ne deviennent pas
un REVERT EVM accepté. L'activation Bellatrix→Capella n'est pas couverte :
le parent doit déjà être Shanghai.

Qualification construite sur une fixture de retraits officielle adaptée
au contexte EVM synthétique : quatre retraits, transaction PUSH0/SSTORE
réussie, état consensus complet attendu, header de taille différente et
historique conservé. Sept refus, dont un retrait gonflé avec un bloc EVM
indépendamment valide ; trois annulations avec parents inchangés, dont une
en fin de préparation. Un faux engagement d'état EVM entraîne un échec
sans publication du débit consensus.

Régression 6/6 : pont Bellatrix, retraits/exécution, conteneurs Capella,
changements BLS, payload SSZ et nouveau couplage. Résultats et empreintes
consignés. Pas de preuve d'un bloc signé complet, d'epoch, de fork-choice,
de crash ou de commit RocksDB par ce scénario.

Suite requise : slots/epochs Capella (historiques), racine du corps de bloc,
composition des opérations et signature, activation versionnée, intégration
au Store/réacteur et publication atomique durable. Le réseau demeure
Bellatrix et EIP-4895 reste partiel.
