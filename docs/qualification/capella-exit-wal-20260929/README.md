# Admission de sortie : reprise après erreur WAL

Le 29 septembre 2026, `consensus_capella_queued_exit_sync` et la régression du
banc partagé `consensus_capella_queued_bls_sync` passent : **2/2, 9,30 s**.
Base `f1c6c3a` plus changements identifiés par le manifeste.

Pour chaque type d'opération, quatre cas sont exécutés : erreur fsync sur le WAL
principal ou sur le témoin, suffixe conservé ou tronqué. Après l'erreur, six ou
huit accès sont refusés selon le lieu de la panne, et le RPC ne sert pas l'état
incertain. Après réouverture, le checkpoint est complet, la tête et la racine
EVM correspondent au consensus, et le témoin restaure l'opération lorsque sa
réservation subsiste. Si la réservation du témoin a été perdue, la sortie doit
être soumise de nouveau. Les deux chemins aboutissent au même checkpoint.

La sortie réadmise deux fois ne republie pas de checkpoint ; une nouvelle
réouverture conserve exactement les mêmes 112 octets signés. Les échecs de
synchronisation sont injectés, puis le processus sort par `_exit` sans nettoyage.
La perte du suffixe est simulée par troncature du WAL du banc isolé.

Ce test porte sur **l'admission**, pas sur chaque instruction d'inclusion,
finalisation, paiement ou élagage. Ce n'est pas une panne physique et ce n'est
pas encore une preuve Linux/VPS. Les témoins restent dans le même répertoire
physique du Mac de qualification. Le jalon de production reste ouvert.

```sh
ctest --test-dir build-release -R '^consensus_capella_queued_(exit|bls)_sync$' -V
```

Les espaces finaux des journaux texte de compilation/CTest sont normalisés.
