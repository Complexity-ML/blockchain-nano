# File durable de sorties volontaires Capella

## Implémentation

`voluntary_exit_ssz` authentifie la sortie contre la tête admise : index, clé,
époque, domaine BLS, durée d'activité et éligibilité. Une admission ne modifie
pas le registre consensus ; elle publie uniquement la file dans le checkpoint.
Le doublon exact est idempotent, une seconde demande pour la même identité
clé/index est refusée. La capacité est liée au budget `messages` du profil.

Le format durable `nano-capella-durable-v4` archive le message signé et sa
racine d'admission. La clé est dérivée à nouveau de cet état authentifié à la
reprise ; les signatures, identités uniques, empreintes et encodages canoniques
sont vérifiés. Les formats antérieurs restent lisibles et leur écriture est
conservée tant qu'aucune sortie n'est archivée. Un ancien binaire ne doit pas
être utilisé pour rouvrir une base contenant le format v4.

Les propositions automatiques consultent cette file. Une demande n'est
applicable qu'à l'index portant toujours la même clé, sans sortie déjà planifiée.
L'inclusion non finalisée ne l'efface pas ; une reorg peut donc la rendre de
nouveau applicable. L'état finalisé gouverne le retrait de la file. Lors d'un
élagage, la racine d'admission est réancrée uniquement si la même identité et
la signature restent vérifiables ; une branche définitivement abandonnée ne
peut imposer une sortie au nouvel occupant de son index.

## Couverture et limites

Le scénario ajoute admission invalide, annulation, doublon exact, capacité
pleine avec deuxième sortie valide, réouverture, sélection automatique et
bascules vers une branche avec/sans sortie. La file reste durable dans les
deux branches non finalisées. Deux régressions couvrent le sélecteur explicite
et la file de changements BLS, notamment l'ancien format v3.

Le profil du test configure une époque minimale d'activité. Aucun paramètre
réseau n'est changé. Le premier build a échoué sur une confusion tableau/vecteur
pour les clés BLS ; diagnostic conservé, représentation corrigée en 48 octets.

Le retrait après finalité et le réancrage lors d'élagage sont implémentés mais
pas qualifiés par ce scénario. Reprise sur erreur WAL, perte de témoin,
réindexation des dépôts, RPC/P2P, relais, retrait final des fonds et qualification
multihôte restent à couvrir. La sortie locale n'est pas encore accessible par
le réseau. Aucun EIP ou objectif production clôturé.

```sh
cmake --build build-release --target nano-capella-transition-tests -j 2
ctest --test-dir build-release -R '^consensus_capella_(queued_exits|local_exits|queued_bls_changes)$' -V
```

## Résultat

macOS ARM64, Release : 3/3 tests passent en **13,98 s**, dont la file durable
en **4,72 s**. Logs, JUnit et empreintes des sources/binaires joints.
