# Copies physiques et préparation de reprise — 12 octobre 2026

Le cœur C++ est inchangé : source `85eb492f641452c4746ee60f027eacefa775b0b0`, relais `4b2165b8a642811562bf8ddca0f0750fe1dcb079`. Les six binaires sont identifiés. Le relais natif disparu a été reconstruit depuis sa source identique et ses 101 dépendances conservées : le nouveau SHA n'est pas présenté comme celui de l'ancien binaire. Onze contrôles CLI, une admission réciproque Noise/TCP en loopback et une éviction après divergence du genesis passent ; cela reste distinct d'une qualification WAN ou consensus.

## Résultats acquis

- Les 65 paires principales/témoins du bootstrap natif ont été copiées physiquement entre les trois environnements : 16 Mac natif, 16 Docker ARM64, 33 Docker AMD64. Les audits en lecture seule passent avec 130 réouvertures froides, namespace, clé, floors, engagements et intentions exacts, contenu complet et fichiers/inodes inchangés. La source est préservée. La compatibilité physique vérifiée inclut RocksDB natif 11.8 vers Docker 8.
- Le nouveau helper de capture arrêtée compile en natif et AMD64 avec les archives existantes ; 25 contrôles purs passent. La revue extérieure ne trouve pas de blocage statique. Il ne construit aucun signer ni reactor et n'importe rien. La vérification complète des messages, de l'archive et du rejeu canonique reste au core lors de sa reprise, sans être revendiquée par cette capture.
- Les deux sources Mac arrêtées sont capturées aux engagements actuels, principaux et témoins distincts. Les copies entières incluent les bases, journaux, témoins et identités P2P : 367 fichiers par nœud, 9 520 490 et 8 976 380 octets. Les destinations sont auditées en lecture seule et concordent ; les sources restent inchangées et les inodes des copies indépendants.
- L'observateur strict conserve les refus de futures erreurs HTTP404, les contrôles de headers signés, l'ancestry, les régressions, les trois owners et la provenance one-shot. La suite finale de 23 cas passe ; elle n'est pas additionnée aux 13 cas de la version antérieure comme une qualification supplémentaire du réseau.

## Tentatives et échecs conservés

Le premier contrôleur `26044/1` refuse le drapeau administratif `NANO_TEST_NETWORK_TRACE=1` pourtant présent dans les capsules exportées. Aucun writer, reservation startup ou nœud n'a été lancé. Le lecteur est corrigé pour ce seul champ exact ; les 65 clés réelles et leurs hash bindings demeurent contrôlés.

Le deuxième contrôleur `81954/1` démarre les trois nœuds et leur protocole local, puis refuse HTTP404 `Signed block not retained` pour le checkpoint initial. Il s'arrête avec trois reçus propres, healthy, et zéro proposition/attestation/sync message admis. Aucune nouvelle finalité commune n'a été observée. Le snapshot est volontairement une ancre trusted sans archive signée ; le test demandait son header trop tôt. La correction attend le premier checkpoint plus récent, en dérivant l'unique checkpoint SSZ initial (époque0/racine zéro) depuis le buffer initial hash-validé. Le baseline ne compte jamais comme progrès. Tous les headers futurs restent authentifiés et leurs erreurs fatales ; aucun archive ni signature n'est fabriqué.

La revue de préparation trouve également un raccordement incorrect du commitment témoin indépendant à l'argument de reprise du core. Cet argument attend le commitment principal `next` de l'intention témoin. Le contrôleur est corrigé avant toute tentative restaurée ; les deux engagements restent conservés et comparés séparément.

L'import d'ancre signée via le chemin duplicate réseau est explicitement exclu : le core ne réauthentifie pas cette branche et interdit son backfill. Aucune modification du core n'est nécessaire pour corriger le prérequis de ce test.

## VPS et autorisation

La capture arrêtée VPS s’est terminée localement avec code0 ; sa capsule reste sur le VPS et ses engagements n’ont pas été lus sur le Mac. Le premier essai local avait refusé le chargement d’une bibliothèque faute de montage du volume de build ; ce refus avant ouverture des bases est conservé, et le seul montage omis a été ajouté. La copie complète et son audit de destination VPS se sont ensuite terminés strictement sur le VPS avec code0 (session11319, terminale8ecb6). Les quatre preuves dérivées restent sur le VPS ; elles doivent être autorisées, transférées et concordantes avant de relancer trois duties. Aucune valeur de checkpoint ni capsule VPS n’a été consultée sur Mac par cette opération. Deux actions de transfert ont été refusées par la revue automatique : capsule privée complète, puis engagements et métadonnées dérivés vers le Mac. Une autorisation humaine dédiée pour quatre preuves dérivées seulement est en attente. Les clés, bases et capsules brutes restent sur le VPS. Aucun contournement par un autre export n'est réalisé.

Les preuves de capture Mac et copie complète ci-jointes sont des résumés publics. Aucune base, clé privée, environnement de signature, inventaire complet ou snapshot privé n'est dans ce bundle.

## Limites

Aucune reprise complète à trois nœuds, reprise effective de duties ou nouvelle finalité commune n'est encore qualifiée. V37 FAILED et le refus historique de rattrapage prolongé restent conservés. Le NAS proposé reste à localiser ; la recherche GitHub n'a retrouvé aucune sauvegarde complète V37 dans le périmètre examiné. Aucun statut EIP ni production n'est promu, aucune image n'est reconstruite, aucun fonds réel ni déploiement public effectué.

Les critères des quatre documents, la CI et la revue de sécurité indépendante restent ouverts. Le goal demeure actif. La prochaine étape est d'auditer les copies complètes et engagements actuels du VPS, reprendre les trois owners depuis des copies durables, puis prouver au moins deux nouvelles finalités communes distinctes et trois lectures stables avec participation réelle de chaque owner.
