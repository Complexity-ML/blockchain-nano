# Préparation V29 : restauration froide des snapshots V28

Core cohérent e33ad89e (sources src/include/third_party/network identiques à main), mêmes binaires que le lot de qualification froid ; hashes revérifiés et liés aux sorties qualifiées. Images/volumes existants conservés, aucun rebuild.

Relecture historique actuelle réussie sur les trois acteurs, puis pins principaux et signataires comparés séparément aux reçus V28 : actifs16/16/33, historiques16/16/0. Copies APFS natives et Docker Mac vérifiées entièrement par SHA256, sans peer.key :1727/1758 fichiers. Tous journaux et témoins conservés. Source Mac V28 intacte.

VPS : volume arrêté sans writer, sauvegarde physique complète de2716 fichiers sur son hôte, hashes égaux aux digests privés post-V28 et aux sources après copie ; inodes indépendants. Sauvegarde privée0700 conservée sous /tmp/nano-restored-three-node-vps-v29-source-backup-20261006. Volume existant réutilisable ; aucun fichier/DB/configuration/clé privée transféré entre hôtes. Dérivation locale VPS comparée par digest au plan indépendant avant écriture.

Plan : seul budget administratif blocks augmenté1024→4096, cache froid8 records/16MiB ; capture explicite4096/128MiB. Clock, autres limites, profils et pins conservés. Le clock VPS reflète celui effectivement appliqué par le contrôleur V28, vérifié par la dérivation locale. Aucun changement de quorum/registre de validateurs. Le plafond de graphe reste une ressource locale, pas un plafond de validateurs.

Préparation terminale76593/0, sauvegarde42117/0. Aucune migration/duty/signature lancée à ce stade. Il faut démontrer la migration des snapshots, la reprise et trois nouveaux checkpoints de finalité communs en900s. Une hausse de capacité seule ne clôt pas la saturation historique ; consommation/RSS/GC/recovery/fork choice restent à qualifier. V28 FAILED et tous critères de production, Base–Nano/revue indépendante conservés.
