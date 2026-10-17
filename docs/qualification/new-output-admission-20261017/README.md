# Première admission des sorties neuves — correction qualifiée partiellement

Source candidate liée au commit parent919b4f3c + manifest auteur36f8a91d… ; trois
fichiers JS, aucun C++/CMake/image modifié. La vraie fonction save du driver est
testée, avec son prédicat observe parent, dans des projections séparées.

Qualification Root : baseline causale1FAIL avec Public input changed during FD read ;
candidat5nouveauxgroupes PASS, puis1groupe existant des autorisations du driver PASS
(trois invocations distinctes, pas un test unique6/6). Les55refus historiques ne
sont pas rejoués. Entrées sélectionnées inchangées. L’essai01 reste refusé : copies
non stables avant tests, générateur Root SyntaxError avant mutation, puis fixture
auteur SyntaxError avant tout cas ; aucune causalité n’en est déduite.

La correction concerne uniquement un fichier wx neuf avec ses octets connus :
SHA/taille et huit champs du FD créateur stricts ; trois lectures neuf champs
cohérentes et identiques sur au moins2s, fenêtre30s. Une variation ctime ne compte
pas comme échantillon stable ; toute autre mutation refuse. Les lignes held/newFiles
déjà présentes ne sont jamais remplacées et observe/fence restent stricts.

Un essai physique distinct utilise le vrai save et le helper sans simulation FS/temps
sur un JSON synthétique neuf de6 585 558octets :3045ms,4lectures, série stable2021ms,
zéro échantillon incohérent intra-lecture ; refus d’écraser une sortie déjà retenue,
octets inchangés. Il ne reproduit pas l’acteur ni l’incident intra-FD original.

Chaque save ajoute désormais au moins2s aux délais existants, sans augmentation de
budget. La tenue du cycle entier demeure à qualifier. Aucun journal, capsule réelle,
clé, nouvelle identité, signature ou campagne n’est ouvert/créé par ces contrôles.
La tentative03 reste FAILED ; aucune revue indépendante ni production qualifiées.
