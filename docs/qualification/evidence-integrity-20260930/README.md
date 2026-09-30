# Intégrité des archives de qualification — 30 septembre 2026

Le vérificateur `scripts/operations/verify-evidence.mjs` reçoit une empreinte
indépendante du manifeste. Pour cette vérification, les cinq empreintes sont
calculées depuis les objets Git du commit indiqué dans `results.json`, puis
comparées aux fichiers du checkout : 78 fichiers référencés, tous conformes.
La dernière archive contient uniquement les preuves préparatoires des cycles,
pas un verdict réseau. Huit tests ciblés passent (altération de fichier ou de
manifeste, fichiers absents, liens symboliques, chemins et budget de lecture).

Le contrôle lit des fichiers réguliers sans suivre le lien final, avec un budget
total de 32 MiB et un index borné à 256 entrées. Il ne décompresse pas les logs,
ne valide pas leur signification et ne démontre ni provenance des binaires,
ni reproductibilité, ni aptitude à la production. Une empreinte recalculée
seulement depuis un manifeste modifié ne constitue pas une ancre indépendante.

Usage :

```sh
node scripts/operations/verify-evidence.mjs chemin/manifest.json EMPREINTE_REVUE
```

Les fichiers du lanceur réseau actif et les binaires restent inchangés.
