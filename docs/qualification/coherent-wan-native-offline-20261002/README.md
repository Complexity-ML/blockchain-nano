# Rejeu réel du témoin arrêté sur Mac natif — 4/4

Binaire CMake Mac SHA-256 `8dd333a646feab19a853ede3ff386668f93eb404181e19ae1af97f3ebdff845d`, source 554ebc3a avec runtime égal à 2d2764de. Les huit tests natifs et leurs entrées constituent le parent vérifié ; aucun nouveau build/rejeu de ces huit cas.

Fixture `/tmp/nano-backup-restore-20260930/arm64/final/restored/chain`, arrêtée, **99 blocs / 97 transactions**, profil/anchor/checkpoint conservés de manière distincte. Les chemins profile/anchor `/work` des requêtes publiques sont adaptés au checkout natif, leur contenu/digest inchangé et vérifié par l'audit.

Première tentative : wrapper `sandbox-exec` refuse d'appliquer un sandbox imbriqué (`sandbox_apply: Operation not permitted`, exit 71 pour les quatre requêtes), avant toute exécution Nano. Sources/base/binaire avant/après acquis, assertion refuse la qualification ; preuve conservée. Seconde tentative depuis l'exécution autorisée hors sandbox Codex, mais **processus Nano restreint par sandbox macOS : écritures de fichiers et réseau interdits**. CLI `audit-capella`, 180 secondes maximum par cas.

**4/4** : positif accepté en 2,062 s, tête non finalisée refusée en 1,680 s, époque future refusée en 1,674 s, racine malformée refusée en 0,023 s. Erreurs exactes attendues. Positif : époque 0xc, racine `0xaf6a0f3a57914de810bc5a7aed56a00e21166bec3fa4bb8396de312f5692e1c6`, séquence 1147, digest `0x89a622679ab2817050fe4283d40b91d262ea2ef51b938dcd54e7d2352b431d9d`. Les deux drapeaux canonicalHistoryVerified/externalFinalizedAncestorVerified sont vrais et requiredFinalized est le checkpoint de la requête.

Base complète/sources/binaire avant/après identiques. Vérification supplémentaire après audit des **198 fichiers d'entrée** : identiques aux empreintes prises avant la sélection native précédente, incluant dépendances dynamiques accessibles. Limite du cache dyld inchangée. Rejeu CPU/OS macOS concret, distinct du lien partiel ARM64 Linux antérieur. Aucun ancien échec supprimé.

**Limites :** checkpoint choisi dans cette fixture historique, aucune nouvelle observation WAN indépendante, aucun transfert de clés/journaux privés, aucune reprise des duties. Votes historiques autonomes et fork-choice complet restent non vérifiés. Les campagnes Docker et l'audit AMD64 en attente restent indépendants ; aucune porte production globale fermée.
