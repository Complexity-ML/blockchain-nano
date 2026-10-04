# Packaging des binaires isolés, 4 octobre 2026

Écart : le septième argument sélectionnait le cœur isolé, mais le P2P venait
obligatoirement de `/build/network`, incompatible avec la disposition cohérente
`/build/coherent-efbeb9bc-<arch>/network`. Aucun packaging incohérent déclaré réussi.

Correction : huitième argument facultatif pour la racine qualifiée contenant
`network`. Septième et huitième arguments peuvent être le même sous-répertoire.
Les usages historiques restent valables ; chemins injectés/absolus/traversée
refusés avant Docker. Export de seulement trois fichiers depuis le volume RO.
P2P sélectionné obligatoire, refus de symlinks (fichier/dossier/parent direct),
révision et empreintes attendues toujours vérifiées lors de l’assemblage.

Dix tests du précontrôle avec Docker simulé : exit 0. Trois tests supplémentaires
exécutent le vrai shell d’export avec des fixtures locales : bonnes bytes P2P
malgré un P2P racine périmé, absence refusée, symlinks refusés. Exit 0.
Les reçus des sorties outil sont conservés dans `results.json` ; il ne s’agit
pas d’une exécution Docker ni d’un assemblage d’image. Le script source et les
tests exacts sont archivés avec leurs empreintes.

Aucune image/build qualifié n’a été modifié ; assemblage réel et cycle opérationnel
restent ouverts jusqu’aux campagnes en cours et à leurs preuves terminales.
