# Journal de signature Beacon

État au 25 septembre 2026 : composant local implémenté et testé, sans activation
des tâches de validateur dans `nano run`. Il ne remplace pas le journal de votes
hauteur/tour du consensus BFT historique.

## Réservation avant signature

`BeaconSigningJournal` protège une clé publique BLS dans une base dédiée.
Le détenteur de la clé secrète fournit un callback de signature ; la classe ne
conserve ni n'écrit cette clé. Le caller doit vérifier les tâches du validateur,
l'état, le choix de branche, la version de fork et l'exécution avant d'appeler
le journal. Le callback ne doit pas réentrer dans le même journal et doit borner
ses propres délais. Aucune méthode de signature publique RPC n'est ajoutée.

Pour une proposition, le journal construit la racine de signature du header
avec le domaine proposer. Pour une attestation, il utilise le domaine attester
et vérifie la cohérence slot/époque cible et source/cible. La racine des
validateurs au genesis fait partie des deux domaines.

Avant le callback, le journal réserve la racine via un checkpoint RocksDB
synchronisé avec comparaison de la séquence/digest attendus. Un échec de stockage
rend l'instance indisponible jusqu'à réouverture. Une signature produite par le
callback n'est retournée qu'après vérification BLS avec la clé publique liée.
Un callback qui échoue ou renvoie une signature invalide laisse sa réservation
intacte ; une tentative contradictoire ultérieure reste refusée.

## Politique minimale

- Proposition : slot strictement croissant ; seule la répétition exacte de la
  dernière racine au même slot est autorisée.
- Attestation : cible strictement croissante et source non décroissante ; seule
  la répétition exacte du dernier vote est autorisée à cible identique.

Ces gardes excluent les doubles votes et les votes qui s'entourent. Elles sont
conservatrices : un ancien message potentiellement sûr peut être refusé. Elles
ne constituent pas une pénalité/slashing sur les soldes et ne détectent pas des
signatures effectuées hors du journal.

La version de fork ne crée pas un nouvel espace de réservation : changer de
version ne permet donc pas de signer une autre racine au même slot/à la même
cible. La base est liée au chain ID, au genesis d'exécution, au genesis des
validateurs, à la clé publique et aux slots par époque. Elle contient uniquement
les deux dernières réservations et leurs identités ; le plafond interne de
4 Kio borne ce format fixe, pas le nombre de validateurs du réseau.

## Reprise et limites

Un nouveau journal requiert une base vide. Une base existante exige un engagement
séquence/digest conservé indépendamment et correspondant exactement au checkpoint.
Un second objet partageant cette base ne peut continuer depuis un état périmé.
L'appelant doit conserver les nouveaux engagements ; après un arrêt dans une
fenêtre où cet engagement externe n'a pas été mis à jour, la résolution reste
une procédure explicite, pas un redémarrage qui ignore le contrôle.

Cela ne protège pas contre la suppression du journal et un redémarrage volontaire
en mode initialisation, ni contre le rollback conjoint du disque et de son
engagement externe. Deux copies de la clé utilisées avec deux journaux distincts
ne sont pas coordonnées. Une clé volée peut être utilisée hors de Nano : ce
composant ne résout donc pas une compromission de quorum de type Ronin.
Le déploiement multi-clés, la migration et la gestion matérielle des clés restent
à intégrer et qualifier avant activation des validateurs.

## Qualification

`build-cmake/qualification/beacon-signing-journal-20260925/` contient :

- `journal-tests.log` : 18 refus avant callback, doublons exacts, mauvais domaine
  de fork au même slot, votes contradictoires, réouverture, mauvais engagement,
  mauvaise identité, instance périmée, concurrence, signature invalide du backend,
  échec de synchronisation WAL injecté et `SIGKILL` après réservation avant signature.
- `crypto-tests.log` : régressions des fixtures officielles BLS et racines de signature.

Une panne de processus n'est pas une coupure électrique du stockage. Ces tests
ne démontrent ni une finalité distribuée ni l'autorisation d'une tâche de validation.

## Interopérabilité à terminer

L'[EIP-3076](https://eips.ethereum.org/EIPS/eip-3076) définit l'échange d'historique
anti-slashing entre clients (format version 5). Son import/export n'est pas
implémenté ici. Il faut notamment qualifier les racines absentes, la fusion des
historiques, les bornes de refus et l'interdiction de signer dans des zones dont
l'historique est inconnu. Le format local du journal n'est pas un fichier EIP-3076.
Les règles de slashing sont définies par la
[spécification Beacon](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/).
