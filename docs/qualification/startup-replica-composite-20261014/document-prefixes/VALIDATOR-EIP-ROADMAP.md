## 14 octobre 2026 — Timer et CLI par répliques : qualification limitée

Sources finales et CMake fcc réellement configurée sur Docker ARM/AMD64 ; quatre CTests distincts par architecture sur deux lots, huit exécutions totales, zéro nouvelle signature. Gate RO/RW de l’outil fermé uniquement pour lecture sur répliques + vrai LOCK ; writers privilégiés/écriture concurrente non garantis. [Preuves](qualification/deadline-replica-combined-20261014/README.md).

Shanghai, EIP/CSV et grant 2h/60 inchangés. Copies privées/journaux/UTC/duties/finalité, WAL/backup, natif/clean/install/CI/audit et production restent ouverts ; préfixe deadline50fe et historique ae37 conservés byte pour byte.

## 14 octobre 2026 — Startup steady : preuve limitée

Docker ARM/AMD64 : deux CTests réels PASS chacun, 13 lignes startup/deadline, zéro nouvelle signature ; runtime/helper/test V2 exacts, archive142 cohérente et readbacks conservés. [Preuves](qualification/capella-startup-deadline-20261014/README.md).

Grant 2h/60, profil Shanghai et statuts EIP/CSV inchangés. CLI maintenance RO/RW concurrent, UTC/journaux/duties/finalité, natif/install/CI/audit et production restent ouverts ; tous corps historiques conservés.

