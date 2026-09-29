# Sortie Mac/VPS avec panne avant finalité — 29 septembre 2026

**Troisième campagne réussie.** Deux hôtes physiques exécutent le code de nœud
`77cb84b51a7fd3b334880d15a22a34b7eb532025`, avec 32 validateurs synthétiques
par hôte. Le Mac exécute le binaire natif ; le VPS Ubuntu x86-64 utilise Docker
(2 vCPU, 4 Go). Les empreintes des binaires, de l’image et du banc exécuté sont
conservées dans les événements et le manifeste.

## Parcours démontré

La sortie du validateur 0 est soumise uniquement sur le VPS via le RPC privé
par le shim du banc. Sa clé de validation est sur le Mac. Les deux vrais
daemons cpp-libp2p communiquent par TCP à travers un tunnel SSH privé ; aucun
port RPC/P2P public n’est ouvert. Il ne s’agit pas d’un essai de découverte
publique des pairs ni de conditions Internet sans tunnel.

Après observation de l’inclusion sur les deux hôtes, le conteneur VPS reçoit
SIGKILL (code 137), avant finalité. Il redémarre avec la même configuration,
le même volume et la même identité P2P. Les pairs se reconnectent et la
finalité commune avance jusqu’à l’époque 8. Le contrôle final attend trois
observations cohérentes et rattache le checkpoint Beacon au bloc d’exécution.

- Bloc d’exécution finalisé : **55** (`0x37`).
- Hash : `0xba18f4c2f5538cadf219caed7bfd46e3c7138774eb6ad0c800d0cc65b335e95b`.
- Solde finalisé du destinataire : **33005098687000000000 wei**.
- Un seul paiement de capital ; les autres paiements sont partiels/résiduels.
- Tous les blocs d’exécution, racines d’état et retraits de 0 à 55 sont égaux
  sur les deux RPC ; la somme exacte des retraits égale le solde au bloc 55.
- Les deux groupes de validateurs ont proposé et attesté ; leur arrêt indique
  une horloge et un validateur sains. Aucun OOM du conteneur n’est observé.

Après arrêt, le vérificateur C++ rouvre le propriétaire du Mac avec son témoin
RocksDB. Il contrôle la comptabilité, la cohérence propriétaire/consensus,
l’archive effectivement retirée, et le journal du validateur 0 : aucune
proposition ni attestation après son époque de sortie (6).
La réouverture hors ligne du VPS passe également sur `24971b8` : tous les
champs comptables, de finalité et d’archive correspondent exactement au Mac.
Le journal du validateur 0 est contrôlé sur le Mac seulement, qui détient sa
clé dans le banc. Le test d’élagage d’une sortie en attente passe sur ce même
binaire Linux (1/1, 89,17 s). Voir `vps-reopened/` pour les preuves.

## Échecs conservés

1. Le RPC était prêt avant le premier en-tête signé : HTTP 404 au démarrage.
   Le banc attend maintenant les en-têtes avec un délai borné.
2. Un changement de finalité et l’élagage ont supprimé l’ancien checkpoint
   entre deux lectures : HTTP 404 pendant la vérification finale. Le banc
   relit les checkpoints avant de retenter. L’essai reste marqué échoué,
   même si la vérification hors ligne ultérieure du Mac passe.

`executed-harness.mjs` est le banc exact de la troisième campagne. Après son
lancement, le fichier maintenu dans `tests/capella` a reçu un durcissement :
un 404 exige désormais un changement de checkpoint du nœud concerné, plutôt
que de l’un ou l’autre nœud. La troisième campagne a exercé une relecture lors du passage de l’époque 7
à 8 ; son événement ne consignait pas le nœud ayant renvoyé 404. Le contrôle
plus strict ajouté ensuite n’est donc pas revendiqué comme exécuté lors de
cette campagne. Les logs bruts sont compressés sans
modification. Les clés enregistrées sont uniquement les scalaires publics du
jeu synthétique, jamais des clés de portefeuille utilisateur.

## Limites du jalon

Tenure et délai de retrait raccourcis pour le test (une époque), slots de six
secondes et époques de huit slots. Ce résultat ne qualifie ni les paramètres
mainnet, ni une perte électrique/disque physique, ni chaque point de coupure
au moment du paiement, de la finalisation ou de la suppression de l’archive.
La régression finale du jalon sur les sources publiées reste à consolider.
Aucune déclaration de production prête, d’EIP globalement conforme ou de
bridge prêt à recevoir des fonds ne découle de ce résultat.

## Reproduction

Depuis un dépôt Mac et un dépôt VPS contenant le même code de nœud, les
binaires déjà construits et le volume `nano-linux-sync-build` préparé :

```sh
node tests/capella/exit-multihost.mjs ubuntu@145.239.199.166 NODE_REVISION /tmp/NEW-EXIT-RUN
build-release/nano-capella-transition-tests --verify-exit-owner /tmp/NEW-EXIT-RUN/fixture/genesis.json /tmp/NEW-EXIT-RUN/mac.json /tmp/NEW-EXIT-RUN/mac-data/chain
```

Le vérificateur s’utilise seulement après arrêt du propriétaire. Les preuves
stockées ne contiennent pas les bases complètes ; leurs chemins et volumes
sont consignés dans `resources.json` pour poursuivre l’inspection.
