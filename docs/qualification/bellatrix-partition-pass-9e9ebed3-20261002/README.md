# Cas Bellatrix de partition locale réussi — campagne non terminale

Candidat Docker ARM64 9e9ebed3 : `consensus_bellatrix_partition_recovery` passe en **372,25 s / 720 s**, limite inchangée. Trace extraite du segment CTest complet alors que la campagne continue ; aucun cas relancé.

Deux cœurs indépendants et deux démons P2P sur un seul hôte physique ; horloge réelle, stake distribué, 64 clés locales au total. Les deux démons P2P sont suspendus par SIGSTOP pendant **96 s** (deux époques), puis repris par SIGCONT. Le test exige des branches concurrentes vivantes et interdit à une moitié isolée d’avancer la finalité. Les observations pendant la coupe conservent la finalité initiale. Après reconnexion, le checkpoint Beacon et son payload d’exécution convergent en **95 s**, dans la fenêtre de reprise originale de 240 s ; checkpoint finalisé observé à l’époque 6.

Ce cas ne teste pas une partition WAN, une latence asymétrique prolongée, un arrêt matériel ou la restauration des trois propriétaires. `crashRecovered=false`. Les programmes sont liés au manifeste antérieur de campagne, mais l’audit d’intégrité post-campagne n’est pas encore disponible ; aucune qualification terminale des 427 cas n’est acquise. Toutes les exigences de production restantes sont conservées.
