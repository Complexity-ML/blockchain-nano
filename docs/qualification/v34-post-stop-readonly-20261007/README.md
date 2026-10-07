# V34 — audits complets après arrêt

Les captures en lecture seule passent sur Mac natif, Docker ARM64 et Docker AMD64/VPS. Les sorties originales 76517 et 79302 sont terminales à 0. Le cœur de capture est c4996498 ; les captures Docker ont été reliées après l’arrêt de V34.

| Nœud | Fichiers inchangés | Pins actifs vérifiés | Pins historiques conservés |
| --- | ---: | ---: | ---: |
| Mac natif | 1846 | 16 | 16 |
| Docker Mac | 1880 | 16 | 16 |
| Docker VPS | 3008 | 33 | 0 |

Chaque audit vérifie séparément les engagements exacts du reçu d’arrêt, le stockage principal et son témoin indépendant, ainsi que le rejeu canonique complet. Les dictionnaires de hashes de fichiers, configurations, demandes et bases restent privés sur leur hôte. Les capsules publiées contiennent uniquement des engagements et diagnostics, aucune clé privée.

Cela prouve l’intégrité de la restauration après les trois arrêts propres de V34. Cela ne prouve ni une reprise de duties après cet audit, ni une nouvelle finalité : V34 reste FAILED, finalité commune epoch1284 inchangée. Aucun writer relancé ; aucune nouvelle image construite. Production, revue indépendante et Base–Nano/Doppler restent ouverts.
