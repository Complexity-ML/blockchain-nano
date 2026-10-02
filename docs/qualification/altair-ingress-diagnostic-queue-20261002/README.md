# Diagnostic Altair AMD64 différé après la campagne

Candidat e8efb812af0e0274916cc2d4fecfc8dadf13e080, descendant de e9ae45ee : seul tests/test_altair_p2p.cpp diffère de 0f53b42d. Ajout de la réponse exacte, des métadonnées de jobs et des dernières raisons de fin de flux (32 maximum, champs sélectionnés sans clés/payloads). Traces réseau de test activées. Assertion de rejet, payload maximal et délais inchangés. Runtime non modifié.

Contrôleur VPS PID 750324 confirmé vivant, phase waiting-original-full-campaign, conteneur original 582618 confirmé vivant ; aucun diagnostic démarré. Il attend les fichiers terminaux, les sources/programmes/entrées de liaison inchangés et l’échec d’origine. Il vérifie la révision propre du checkout Git et que seul le fichier de test diffère.

Après cette attente : une seule compilation directe du fichier de test contre les bibliothèques du build AMD64 existant, en lecture seule ; image et volume réutilisés, aucun rebuild d’image. Compilation bornée 180 s et exécution ciblée 120 s, network none/loopback privé, 2 CPU/3 GiB, root read-only, cap-drop ALL. Empreintes des inputs, exécutable et sources avant/après conservées. Cette compilation directe n’est pas une régression complète du nouveau candidat et les branches positives de ce contrôleur ne sont pas encore exercées.

Contrôleurs de restauration et d’audits signés précédents conservés ; ils doivent refuser le full échoué. Diagnostic indépendant : aucun déploiement, fonds ou contournement de leur gate. Si le test repasse, l’échec précédent reste conservé et sa cause demeure inconnue tant qu’elle n’est pas expliquée.
