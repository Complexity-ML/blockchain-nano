# Capture catalogue sans attente derrière admission — qualification native partielle

Draft basé sur main d5b2d26f. Les sessions appellent try_capture_inventory : verrou du réacteur acquis via try_to_lock, nullptr converti en CapellaInventoryCaptureBusy existant. API bloquante existante conservée pour ses appelants ; projection atomique partagée sous verrou. Réservations et barrières owner/génération/retrait/annulation restent appliquées. Pas de changement de quota, délai RPC ou quorum.

Trois TU core reconstruits (réacteur, RPC, runtime), archive dérivée de EE5 ; fixture reconstruite et liée. Nouveau scénario vérifie deux refus pendant admission réelle retenue, puis reprise de capacité, retrait de requête et annulation, primaire inchangé. Scénarios génération/capture/worker passent aussi. HTTP initialement refusé par le sandbox lors du bind ; sortie 1 originale 42101 conservée. Même binaire sans recompilation : HTTP loopback original 35744 sortie 0. Cinq modes natifs passent, sources figées inchangées.

Docker ARM64 original 34853 et AMD64 original 75818 en cours à la rédaction. Ce dossier ne les qualifie pas. Pas de nouvelle image, aucun réseau WAN relancé. Le défaut de verrou est corrigé dans le draft et testé nativement ; cause globale de V27, finalité qui avance et production restent ouvertes. Pas de push de ce draft avant qualification du lot.

Mise à jour ARM64 : original 34853 / sortie 1 avant scénario (fixture cherchée /work/tests, montage /work/source/tests). Journaux conservés dans arm64-failed. Correction du chemin via symlink local, même binaire sans recompilation : original 82245 / sortie 0, cinq modes passent. Source figée inchangée. AMD64 original 75818 reste en cours ; le lot trois architectures et la finalité WAN ne sont pas qualifiés.
