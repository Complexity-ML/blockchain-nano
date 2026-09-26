# Validation par lot des attestations candidates

La construction locale valide le lot de candidats par la transition complète.
Le budget doit réserver une validation par candidat en cas de refus du lot ;
sinon le chemin individuel est utilisé directement. Un refus déclenche le
repli individuel dans le même ordre. Les compteurs de capacité et de travail
restent bornés, et toute annulation remonte sans publication partielle.

Test ajouté : huit candidats valides nécessitent une validation au lieu de huit ;
lot contenant un candidat invalide, remplissage après refus, budgets 0 à 10,
absence de marge pour le lot et annulation. Cinq tests Release passent en
31,63 s, dont chaîne Bellatrix signée, boucle validateur, slashing et sortie.

Ce changement affecte la politique de construction locale, pas la validation
des blocs reçus. Il ne retire aucune vérification BLS, SSZ ou EVM. Avec des
candidats invalides, le coût peut augmenter d’une tentative de lot, toujours
à l’intérieur du budget. Le gain WAN/VPS doit encore être mesuré à cette
révision ; le retard intermittent n’est pas déclaré résolu.
