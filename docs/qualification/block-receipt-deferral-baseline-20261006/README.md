# Préparation incomplète classée comme rejet — contre-exemple réel

Source 0a605778, core lié 5787915e (empreinte du libnano_core.a conservée). Session originale 28205/sortie 1 attendue. Test sur un bloc signé conservé du réseau privé v15, racine 0x35cd4a6b…, slot 10751, parent admis au slot 10574.

Le vrai CapellaConsensusService et son worker/registre de reçus rendent completed/rejected pour le premier ticket. Aucune admission ni écriture durable de ce bloc à cette étape. Le même bloc est ensuite admis après 21 continuations explicites supplémentaires sur le même reactor et les mêmes règles. Cela établit que cette préparation incomplète est annoncée comme un rejet dans ce chemin d’admission. La correction doit conserver un travail en attente, ses budgets et sa cancellation, sans ACK avant commit ni boucle qui monopolise les autres messages.

Magasins originaux readonly et 1441/1406 empreintes inchangées ; seule chain du magasin Docker Mac a été copiée localement. Aucun journal, témoin ou stockage de signature copié. Source/readers/profil/engagements v15 vérifiés ; aucune base exportée vers le VPS. La copie et les originaux restent distincts.

Le transport, l’horloge runtime et les refus WAN ne sont pas qualifiés par ce test. Il isole un défaut d’intégration réel ; il ne prouve pas que sa correction suffira à rétablir la finalité commune. Les échecs v15 et tous les critères de production restent ouverts.
