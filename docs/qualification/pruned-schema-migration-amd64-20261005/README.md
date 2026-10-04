# Migration prunée explicite sur copies AMD64 — 5 octobre 2026

Helper 23df7f63 sur branche de candidate, fixture WAN 092a153c arrêtée conservée. Docker/image/volume existants ; aucune reconstruction d'image. Nouveau lecteur linké contre le cœur 59fa6530 qualifié, objet d'entrée 092a153c réutilisé par identité du source. Sources runtime vérifiées identiques à 59fa6530. Trois copies neuves ; original toujours monté en lecture seule. Aucun déplacement des données actives.

Contrôleur terminal exit 0, revue indépendante des hachages et JSON passante. Migration normale nano/1 pruné → nano/2-state-pruned ; mauvais engagement externe refusé sans modification de fichiers. SIGSTOP aux frontières connues puis SIGKILL (137) avant l'écriture et après l'écriture synchrone. Avant : reprise effectue la migration. Après : reprise détecte le schéma déjà migré, ne réécrit rien. Tous les rejeux sont idempotents (zéro écriture).

Seule meta/schema change ; digest ordonné de toutes les autres clés/valeurs identique dans les trois parcours. Audits avant/après byte-identiques en JSON : 77 blocs, 59 états prunés rejoués, engagement indépendant séquence 827/digest c5366531… vérifié. Trois refus explicites unsupported Nano database schema par l'ancien lecteur. Fichiers source et anciennes bibliothèques/binaires qualifiés re-hachés inchangés ; programmes re-hachés indépendamment. Sources, copies et témoins séparés conservés hors Git.

Échec conservé : c56da67f n'a pas compilé (comparaison std::string/JSON), avant toute copie/migration. Correctif de typage 23df7f63, nouveau répertoire de qualification ; aucune assertion ni délai assoupli.

Portée : copies privées AMD64 et audits hors ligne. Pas de migration automatique ou en place d'un réseau public, panne électrique, portabilité ARM64 du helper ou reprise des duties après migration acquises. La qualification antérieure de duties 59fa6530 ne remplace pas cette dernière preuve spécifique. Runtime main non promu, revue indépendante et critères financiers ouverts.
