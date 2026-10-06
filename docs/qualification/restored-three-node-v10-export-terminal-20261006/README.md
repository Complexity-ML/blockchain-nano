# Reprise v10 : export retenu — résultat terminal du 6 octobre 2026

Contrôleur original session 76765, checkout e6b166c4 ; core bc6d1794, relais 76bb2468. Résultat FAILED, sortie 1 : aucune avancée commune depuis 1284 dans le délai initial de 900 s. Critère de trois avancées distinctes conservé, sans restart de nœud ni prolongation. Le premier contrôle préalable avait refusé un hash de commit incorrect avant tout démarrage ; ses traces et son résultat restent archivés séparément.

Les trois propriétaires sont arrêtés par le contrôleur original : sorties 0, sans OOM, trois reçus. Journaux et témoins jamais réinitialisés ni fusionnés. Propositions locales 42/35/27 et attestations locales 303/303/43 ; ces duties ne prouvent pas la finalité commune. Les engagements post-v10 remplacent ceux de v9 pour toute prochaine reprise.

Traces terminales : 107+114 blocs Mac→VPS acceptés, 96 VPS→Mac acceptés. Aucun attestation send-completion accepted observé. Nombreux refus de catalogues d’attestations ; VPS : 306 dispatch-unavailable et 40 deadlines. Aucune transition transport-suspended/resumed/protocol-blocked. La circulation bidirectionnelle améliore le périmètre de transport observé, sans démontrer une résolution causale ni la reprise du consensus.

Un relevé de 2 s identifie nano comme écrivain : 53964800 octets write_bytes et 296 ticks CPU supplémentaires, contre zéro octet et 5 ticks pour nano-p2p. Relevé ponctuel, pas latence par opération ni preuve causale. Les compteurs cumulés d’écriture ne sont pas l’occupation disque. Cgroup préterminal brut conservé ; ce n’est pas nécessairement le pic final. Hypothèses à isoler : coût des publications complètes et progression des préparations quand plusieurs branches alternent. Les tests existants de retry du même bloc ne qualifient pas cette alternance réseau.

Audits indépendants après arrêt, lecture seule : core/witness/signers vérifiés contre les reçus exacts, fichiers avant/après inchangés, historique d’exécution canonique et ancêtre finalisé requis vérifiés. Aucun historique complet de votes/fork-choice rejoué ; aucun succès réseau extrapolé. La base VPS reste sur le VPS, les sources Mac restent locales ; aucune base ni clé privée transférée entre hôtes. Les inspections archivées omettent l’environnement.

Production, finalité après restauration, saturation/pruning, cause WAN A, portabilité physique, rotations/lifecycle et revue indépendante restent ouverts. Aucun fonds réel, vente ou déploiement public.
