Correction de présence obligatoire du pin SHA — 12 octobre 2026.

readRequiredPinnedJson(path, expectedSha256, read=readFile) exige un SHA string de 64 caractères hexadécimaux minuscules avant toute lecture. Une valeur undefined, null ou malformée ne peut plus sélectionner accidentellement le mode sans pin. La fonction appelle ensuite readPinnedJson, dont le code précédent est conservé byte pour byte.

Les appels de preuves obligatoires doivent utiliser cette nouvelle entrée. readPinnedJson sans pin reste destiné au seul index ou document de préparation explicitement externe ; le SHA retourné doit être lié immédiatement à sa preuve externe. Cela ne transforme pas une préparation non pinée en autorité cryptographique.

Seuls deux nouveaux tests ont été exécutés avec le filtre ^required pinned JSON (original 486bba/0, 63.734834 ms) : pins absents/malformés refusés avant toute lecture, et cas positif conservant les bytes authentifiées en exactement une lecture. Les 13 corps de tests précédents restent identiques et n’ont pas été rejoués ; les 23 tests de l’observateur non plus. Aucun runner, core, runtime, base, clé ou réseau n’a été modifié/exécuté dans ce lot.
