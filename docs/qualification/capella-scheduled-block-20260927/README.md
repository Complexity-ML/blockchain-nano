# Admission signée à travers le calendrier Capella

Les API prepare_capella_fork_state_transition et
prepare_capella_fork_unsigned_block composent le calendrier authentifié
Bellatrix→Capella avec le chemin commun de transition. L'état est migré au
slot exact, puis avancé au slot du bloc avant les contrôles de proposant,
parent, signature et domaine, RANDAO, payload, opérations et racine finale.
Construction reste distincte de l'admission et ne réserve aucune signature.
Aucun état n'est publié ni persisté par ces candidats.

Trois chaînes synthétiques de neuf blocs : continuation Capella existante,
premier bloc à la frontière d'époque 1 (slot 8), premier bloc au slot 11
après trois slots sans bloc. Les deux nouveaux chemins partent d'un état
Bellatrix et d'un parent EL Paris. Ils changent les credentials puis paient
un retrait lorsque le curseur revient au validateur concerné. PUSH0/SSTORE
et signatures sont vérifiés dans chaque bloc ; construction et réception
concordent sur les états. Les fixtures initiales et presets sont épinglés.

Le scénario existant conserve 20 refus et 18 annulations. Chacun des deux
scénarios de migration ajoute quatre refus (racine parent, calendrier futur,
budget de slots, ancien domaine de signature), soit 24 refus et 18 annulations.
Les scénarios ne sont pas un corpus officiel complet des blocs Capella.

Régression : blocs officiels Altair, chaîne signée Bellatrix, les trois
chaînes Capella et migration officielle des états : 4/4 suites. Le journal
de build conserve une erreur de nom de cible Altair, ensuite corrigée en
nano-altair-full-blocks-tests ; la régression finale utilise cette cible.

Restent le raccordement au Store/fork-choice/réacteur, la sélection durable
du profil de fork et sa restauration, les transactions atomiques consensus/EL,
puis la campagne réseau à travers la migration. Aucun réseau actif n'est
modifié, aucun EIP ni objectif production n'est clôturé.
