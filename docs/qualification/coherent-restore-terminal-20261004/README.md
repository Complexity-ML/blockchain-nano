# Restauration terminale efbeb9bc — 4 octobre 2026

Contrôleur et revue indépendante : sortie 0. Trois vrais nœuds sur deux hôtes
(Mac natif, Docker Mac, Docker VPS), réseau privé SSH, 64 validateurs synthétiques.
Arrêt au checkpoint 2, sauvegarde/restauration hors ligne, même identité P2P et
21 engagements de signature conservés séparément. Les 21 journaux ont avancé
sans régression ; reprise effective : 124 attestations, 19 propositions,
441 messages sync admis, zéro échec de duty observé.

Finalité commune 2 → 3 en 86 247 ms dans le contrôle de reprise, sous sa borne
inchangée de 180 000 ms. Fenêtre stable 5 → 8 ; checkpoint terminal 8 identique
sur les trois nœuds. Audit en lecture seule terminal : 78 blocs canoniques,
1 transaction, racines/indices/reçus vérifiés avec ancrage finalisé externe.
Fichiers de la sauvegarde source inchangés et audit initial sans mutation.

`review/review.mjs` est le contrôle réellement exécuté, avec ses chemins
originaux et imports de la candidate figée. Son résultat est conservé ; les
entrées publiques/rapports nécessaires sont archivés ici. Les clés privées,
variables d'environnement et archives RocksDB ne sont pas ajoutées au dépôt.
Le hash de l'archive physique reste dans `reports/restore/archive.sha256`.
Les gros journaux sont gzip sans modification ; le manifeste conserve le
SHA-256 et la taille des octets originaux.

Périmètre limité : aucun test de coupure physique d'alimentation, de réseau
public, de 64 machines indépendantes ou de choix de fourche historique complet.
Les votes historiques autonomes ne sont pas tous rejoués. L'anomalie WAN A
reste inexpliquée. Les deux campagnes 427/427 ne remplacent pas ces exigences.
Main n'est pas promu au runtime de la candidate ; production non qualifiée.
L'échec initial de permission et ses dépendants sont conservés dans
`../coherent-recovery-queue-correction-20261004/`.
