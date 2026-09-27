# Coordinateur durable Capella

CapellaDurableReactor sérialise tick, bloc, vote et preuve d'équivoque. Il
valide le Store candidat, calcule le suffixe canonique depuis la tête choisie
et engage snapshot et état EVM dans le batch synchrone CAS existant. L'état
mémoire est publié après succès, sans callback ni allocation intermédiaire.
Un échec d'écriture rend l'instance indisponible jusqu'à réouverture.
Les checkpoints ne peuvent reculer en époque ni abandonner la branche
finalisée ; ancre et horloge sont également contrôlées.

Qualification : les cinq chaînes des trois suites utilisent un coordinateur
sur leur propre base. Chaque événement est comparé au Store préparé ; les
votes et slashings des scénarios de branches passent par la publication.
Les doublons conservent le token et les annulations préalables ne publient
rien. Après fermeture/réouverture, token, snapshot complet et tête EVM sont
identiques. La finalité complète atteint encore l'époque 2, tandis que la
participation 50 % reste sans progression. Régression 3/3, 27,80 secondes.

Limites : coordinateur local sur ChainStore, sans propriétaire Blockchain
(cache RPC/mempool), réseau ni signature locale. Les archives brutes de votes,
blocs et sorties ainsi que les notifications restent à intégrer. L'ancre et
le token de reprise sont fournis par un contexte de confiance externe. La
faute disque du nouveau coordinateur n'est pas injectée dans cette campagne ;
les anciens crashs testent séparément les batches du Store. Le passage d'un
Store Bellatrix vivant à Capella reste ouvert. Aucun réseau actif modifié.
