# AMD01 : refus du lecteur de logiciels avant Nano

La session Root94771 est terminale exit1. Le [dispatcher](dispatch/exit-public.json)
a exécuté le hostV2 f87d2542…/39285 B ; son
[stdout exact](dispatch/stdout.log) conserve le résultat hostFAILED. Il rapporte
huit inputs tenus et la fence finale après cleanupPASS, mais aucune étape Docker,
aucun worker, aucun Nano et aucune requêteHTTP démarrés. L’absence du conteneur
possédé est attestée ; imageFinalFence reste null. Cela ne vaut donc pas une
qualification d’image ou de runtime après exécution. L’audit consommateur
[refuse](dispatch/host-result-audit-public.json) également de promouvoir ce reçu.

Première erreur host : ValueError, « bounded single-link regular input ».
Le [diagnostic Root](diagnosis/stdout.log), réalisé en lecture seule et terminé
exit0, trouve Docker47253395 B, Git4066232 B et Sudo282032 B ; chacun possède
un seul lien. Le lecteur de hostV2 bornait ses inputs matériels à32Mio : le
binaire /usr/bin/docker dépasse cette borne. Ces faits établissent le problème
de limite du lecteur ; ils n’établissent ni corruption du binaire ni mutation
d’une ancienne autorité. Aucun échec C++/EVM ou du réseau n’en découle.

Le [hostV2 exécuté](../../source/amd-host-v2/host.py), son manifeste, son patch et
son README sont copiés exactement. La future V3 doit relever uniquement le
plafond applicable au chemin /usr/bin/docker à64Mio et choisir de nouveaux
TOOL/OUT02 ; elle reste à joindre sur preuve terminale réelle. Aucune image,
compilation C++, clé, domaine signé ou ancien résultat n’est modifié par ce refus.

Ce prédécesseur demeure FAILED après toute reprise ultérieure. Les bases,
contrôles privés, tables d’autorité et JSONproducteurs complets ne sont pas
copiés. Ce dossier ne contient que les neuf fichiers publics désignés par Root.
