# Dispatcher intégré : qualification ciblée — 6 octobre 2026

Le relais conserve session, watermark, capacité et tickets incertains après épuisement des réponses RPC. Il annule puis attend la retraite confirmée du travail avant de rouvrir l’admission. Les contradictions de protocole restent bloquantes. Le redémarrage du core/changement de session n’est pas traité par cette correction.

Docker ARM64/Mac et AMD64/VPS : 10 scénarios transport, 5 avec le registre réel de reçus, 10 régressions antérieures et un test HTTP réel passent. Ce dernier perd trois réponses après traitement serveur, conserve le délai CoreRequest de 5 secondes, puis confirme une seule exécution, une annulation, un Forget et une complétion. Le worker est simulé : ce test ne prouve pas la reprise du consensus réel.

Relais P2P complet compilé et lié sur Mac natif et les deux architectures Docker. L’exécution sans arguments produit la sortie d’usage attendue 1 ; ce n’est pas un test réseau. Aucun rebuild d’image, aucun transfert de stockage ni clé.

La génération CMake ARM64 et l’enregistrement des trois nouveaux tests sont vérifiés ; les tests ont été compilés/exécutés par les commandes sauvegardées des builds existants, pas par un CTest neuf. Les premiers échecs (inclusion json.hpp puis post-traitement Python absent) sont conservés. L’assertion des métadonnées de trace a été déplacée hors du callback qui neutralise volontairement les exceptions ; seul le scénario transport a été recompilé/rejoué ensuite sur chaque architecture. Son empreinte finale est transport-final.sha256 ; les premières empreintes restent historiques.

Les traces de transition ne publient que phase, watermark, nombre de travaux et horodatage ; aucun jeton de capacité ni contenu de transaction. Le cas d’observateur défaillant passe et un nouveau ticket utilise le watermark suivant après drainage.

La cause de l’échec WAN v8, la nouvelle finalité commune et la production restent ouvertes. Prochaine campagne : repartir exclusivement des engagements post-v8 audités, avec les limites 1024 déjà utilisées, sans revenir aux engagements v7 et sans réduire le critère de trois avancées distinctes.
