# Échec AMD64 : bloc Altair à la limite configurée

Campagne complète 0f53b42d encore active. Test 287 `consensus_altair_libp2p` échoué en 2,37 s : `full configured block bound failed before core validation`. L’assertion attend exactement un rejet du cœur `accepted: false` pour un bloc zéro de taille maximale configurée. Le test échoue avant la suite des blocs officiels ; aucun passage libp2p complet ne doit être revendiqué sur AMD64.

Diagnostic conservé depuis le LastTest temporaire, avec SHA256 du log. Trois programmes impliqués vérifiés contre le manifeste initial : test C++, nano-p2p et nano-altair-probe inchangés. Aucun autre échec observé au prélèvement ; campagne non terminale. Réponse exacte du pair non enregistrée ; cause inconnue. Ne pas attribuer l’échec à une limite, un timeout ou une attaque sans diagnostic.

Candidat isolé `qualification/altair-ingress-diagnostic-20261002`, e9ae45ee : ajoute uniquement la réponse exacte et les métadonnées bornées des jobs en cas d’échec. Assertion, payload, limites, délais et runtime inchangés. Non compilé/non exécuté ; nouvelle collecte diagnostique justifiée après la clôture de la campagne existante. Aucun assouplissement ni exclusion du test.

Le gate complet refusera cette campagne et ne doit pas démarrer la restauration WAN. ARM64 427/427 et ses preuves restent valides dans leur périmètre ; ils ne compensent pas cet échec AMD64. Tous les critères et échecs historiques demeurent ouverts.
