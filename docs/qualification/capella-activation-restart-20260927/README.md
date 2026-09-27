# Reprise pendant l’activation d’un nouveau validateur Capella

## Nouveau périmètre

Le scénario étend l’admission native déjà qualifiée : dépôt réel exécuté dans
l’EVM après élagage, consommation unique, nouvelle clé BLS absente du genesis,
attente d’activation puis votes acceptés par deux reactors avec bases distinctes.

Deux reprises supplémentaires sont exercées :

1. Une fois l’époque d’activation fixée, mais avant activation, fermeture et
   réouverture du premier owner et du journal de la nouvelle clé.
2. Juste après son premier vote admis, réouverture du second owner et de ce même
   journal ; tentative d’attestation contradictoire au même target.

Chaque reprise vérifie l’engagement durable, l’état Beacon et la racine EVM,
ainsi que le dernier target signé. Pendant l’attente, aucun des deux reactors
ne doit appeler le signataire. Après le premier vote, le journal doit refuser
le vote contradictoire avant le signataire et sans modifier son engagement.
Le scénario doit ensuite accepter au moins un autre vote légitime de cette clé.

## Limites

Réouvertures ordonnées dans un processus local, horloge synthétique, clés de test,
profil natif de dépôt Nano ; pas de SIGKILL, interruption au milieu d’un commit,
panne disque, transport P2P/WAN ou comparaison exacte des délais d’activation
avec un client Ethereum. Les engagements de reprise sont retenus par le banc,
pas par une infrastructure indépendante de témoins déployée.

Aucune règle du runtime n’est modifiée. Cette preuve ne clôture pas EIP-6110
ni la préparation production de tout le cycle validateur.

## Commande

```sh
ctest --test-dir build-release -R '^consensus_capella_deposit_activation_restart$' -V
```

## Résultat

Passé en **159,53 secondes**. Dépôt consommé au slot 80 ; première reprise
au slot 104, avant l’activation programmée à l’époque 17. Seconde reprise au
slot 142, après le premier vote de la nouvelle clé (target 17). Le vote
contradictoire est refusé avant tout appel au signataire ; trois votes légitimes
sont acceptés au total. La finalité atteint l’époque 18 au slot 161.
Les deux bases sont également rouvertes à la fin et conservent les 65 entrées
du registre, la date d’activation et le curseur de dépôt à 1.

## Échec initial conservé

Le premier run s’est arrêté après 139,06 s sur l’exception attendue
`Conflicting or old beacon attestation refused`. Le type d’exception intercepté
avait été corrigé pendant la compilation initiale ; le binaire pouvait encore
contenir l’ancien catch. Son empreinte, le journal et le JUnit sont conservés.
Une reconstruction avec source stabilisée et un marqueur explicite du refus
passe ensuite. Aucun changement du runtime n’a été nécessaire ; cet échec de
banc ne constitue pas une preuve de vulnérabilité du journal.
