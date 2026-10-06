# Votes sur cibles concurrentes : contre-exemple et prototype natif

Parent core 504c9df8, dépôt c193eb4a inchangé ; prototype hors dépôt, aucun binaire réseau remplacé. Deux blocs frères sont construits et admis par les transitions complètes. Deux branches servent à générer des attestations sur deux racines cibles distinctes à la même époque ; chaque vote est validé séparément sur le même Store commun par l'oracle pleine durée. Signatures/équivocation du proposant synthétiques ; aucune clé réelle ni journal de validateur actif utilisé.

Ancien core, exact même objet test-qualified : sortie 1 « Alternating valid votes erased target continuation ». Son cache de deux entrées remplace la cible de l'époque à chaque racine différente et ne termine pas le préfixe. Prototype : entrée primaire de chaque époque conservée et cache partagé borné pour les cibles secondaires, validation source/profile/racine maintenue. Disposition de classe inchangée. Les deux votes progressent en 43 paires ; état Store identique à l'oracle combiné, signatures invalides rejetées, duplicata sans publication, témoin rouvert avec votes et engagement identiques.

Limites : prototype natif seulement, non intégré, non déployé ; Docker ARM64/AMD64, régressions complémentaires, pression/éviction et coût du cache restent à qualifier. Aucun lien causal avec les refus génériques de v11 démontré. Parents distincts et finalité réseau restent ouverts. Ne constitue pas une réussite de production.

Premiers essais de fixture conservés séparément : include support manquant après déplacement du fichier (compilation 1), fixture à une seule racine cible (précondition 1), votes du slot courant trop tôt pour l'oracle on_attestation (précondition 1). Correction du chemin puis construction de deux vrais frères et traitement au slot suivant ; les échecs préalables ne sont pas des bugs de consensus. Le défaut de cache n'est revendiqué que sur la fixture finale valide.

Compilation native concurrente de v11 documentée dans son dossier : pas de benchmark CPU isolé. Pas de compilation VPS ni ouverture de bases propriétaires pendant la campagne.
