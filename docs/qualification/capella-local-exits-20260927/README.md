# Sélection locale des sorties volontaires Capella

Le constructeur local réservait de la place aux sorties mais laissait leur
liste vide. `prepare_local_body_with_exits` ajoute une sélection bornée de
candidats non fiables. Chaque ajout est soumis à la transition complète du
bloc ; une signature de largeur incorrecte est écartée avant encodage.
La sélection conserve les BLS changes en attente et ne publie pas l'état.

## Qualification

macOS ARM64, build Release. Deux tests passent en 9,29 s :
`consensus_capella_local_exits` (4,42 s) et
`consensus_capella_local_bls_changes` (4,87 s).

Le nouveau scénario construit indépendamment un bloc contenant une sortie
signée, puis compare ses octets à la sélection automatique. Signature
malformée, signature altérée, époque future correctement signée et doublon
sont exclus. Le budget d'essais et l'annulation préservent le checkpoint.
La sortie est incluse au slot 9 ; la durée minimale d'activité est configurée
à une époque uniquement dans ce profil de test. Paramètres réseau inchangés.
Le scénario existant continue avec branches concurrentes, deux reorgs et
réouvertures. Il ne prouve pas un retrait complet du validateur sortant : le
paiement de retrait déjà observé appartient au scénario de credentials.

Premier run en échec après les assertions de sélection (2,33 s) : le banc
supprimait les opérations de la branche alternative sans réinitialiser
l'offset du payload. Correction du banc, échec préservé. La revue avant ce run
avait également corrigé une durée de test nulle, interdite par les règles.

```sh
cmake --build build-release --target nano-capella-transition-tests -j 2
ctest --test-dir build-release -R '^consensus_capella_(local_exits|local_bls_changes)$' -V
```

## Suite requise

Cette API de construction n'est pas une file persistante ni une entrée RPC/P2P.
Raccorder admission, identité clé/index et domaine authentifiés, conservation
jusqu'à finalité, réinclusion après reorg, reprise et relais borné. Qualifier
ensuite sortie jusqu'au retrait, finalité et pannes du cycle complet.
Aucun fork public, EIP-7044/7002 ou objectif production clôturé.

Référence : [traitement des sorties volontaires](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#voluntary-exits).
Les domaines restent ceux du profil Capella existant, pas ceux de Deneb.
