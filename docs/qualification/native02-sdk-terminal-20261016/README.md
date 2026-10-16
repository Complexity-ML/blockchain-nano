# Native02 : construction SDK terminée, consommateur et couverture par valeur

Le build source natif à quatre jobs a réellement terminé avec succès après 6 683,292 s. Son reçu ne revendique ni construction d’image Docker/Core, ni démarrage d’un nœud. Le probe du proxy antérieur est réutilisé ; aucun nouveau probe dans ce namespace n’est revendiqué. Le succès du build ne ferme pas à lui seul la causalité de tous les producteurs.

## Consommateur fonctionnel actuel

Configuration ARM64 : 31 820 ms ; build de `nano-p2p` : 21 636 ms ; refus d’usage sans identité : code 1 attendu en 377 ms. Le M indépendant est égal au MD réel. Les 101 occurrences d’archives sont 101 fichiers distincts du nouveau SDK ; les 1 739 dépendances de headers Boost et les deux archives random/filesystem viennent du provider Hunter 1.87, tag `-a64`. Le programme est `65d08eeef65ef80a255980bef297940dd187cca4b726df1c5487875ff578b734`, 13 406 616 octets. Aucune identité réseau n’est fournie, aucun nœud ou réseau n’est démarré et aucune admission de campagne n’est accordée.

Les sources du consommateur restent celles de `5fc36d9a1e2bd2bd1c61201064c1f85a0498996f`. Le HEAD observé `769ab6f380de4005463ddf81d0079961f7f63565` conserve les trois entrées Git réseau/tiers/script SDK ; les changements de harness/documentation ne sont pas présentés comme de nouvelles sources du consommateur. La collecte de diagnostics constate zéro warning de linker ou de deployment target. Cela ne prouve ni exécution sur d’anciens macOS, ni fermeture complète du loader, ni portage de ce SDK sur AMD64.

## Archives : couverture historique par valeur, sans causalité ar

Les 810 occurrences de membres ont une valeur correspondant à une compilation qualifiée : 636 occurrences ont un seul candidat qualifié observé et 174 plusieurs. Ce ne sont pas 810 TUs/tests ni 636 recettes `ar` prouvées. L’index conserve ses catégories distinctes : 2 276 appels M==MD, 74 returnNonzero, 22 refus independent-M-nonzero et 371 forward. Ces échecs/refus restent tels quels ; les forward ne deviennent pas des compilations qualifiées.

Les 888 candidats historiques sélectionnés ne sont pas un inventaire d’objets courants vérifiés : 137 chemins `.o` sont présents, 741 absents ; 266 chemins `.d` sont présents, 622 absents. Ces comptes ont des portées différentes et ne sont pas additionnés aux 810 membres. Les comparaisons utilisent les reçus historiques, sans rafraîchir les anciennes autorités. La causalité des archives, tous les producteurs de sources générées et le DAG complet restent ouverts. L’ancien audit global en échec sur sept fichiers hors closure reste un échec.

Les reçus publics sont : build `2a6264b6…` (318 octets), consommateur `cca7d10c…` (1 558), jointure `d83b40ca…` (1 242), diagnostics `8c50bff1…` (427). Les sources opérateurs revues du consommateur `4382…` et de la jointure `e125…` accompagnent la consolidation ; le helper pur `2f47…` est déjà publié. Aucune donnée SDK, table privée ou log brut n’est inclus dans ces textes.

## CI et frontière de production

La [CI du commit 769ab6f3](https://github.com/Complexity-ML/blockchain-nano/actions/runs/38863463537) conclut failure pour facturation : changes/operations non démarrés, network/core skipped, aucun test de code exécuté. Aucune relance ou modification de facturation/dépense n’est effectuée ; ce n’est pas un PASS CI. Le reçu public est `8ca86224…`, 540 octets.

Les échecs et qualifications antérieurs restent conservés. Les 24 identités connues ne sont pas des admissions de campagne. Réseau actif, restauration des anciens journaux, duties à trois propriétaires, finalité commune, cycle complet, nouvelle conformité EIP/fork et production restent non qualifiés. Les quatre documents historiques reçoivent le même appendice sans retrait de critère.
