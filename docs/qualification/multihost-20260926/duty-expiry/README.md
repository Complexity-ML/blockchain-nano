# Expiration des devoirs RANDAO et attestation

La classification déjà utilisée pour les propositions est appliquée aux deux
autres devoirs. Un dépassement du slot avec horloge non décroissante produit
ExpiredBellatrixDuty ; un recul d'horloge et les exceptions du fournisseur
restent fatals. Aucune signature expirée ne revient à l'appelant. Les écritures
du journal ne sont pas supprimées ou réinitialisées.

Deux nouveaux scénarios de boucle avec horloge synthétique et fournisseur BLS :
- RANDAO : expiration dans le fournisseur, aucune proposition ni réservation
  de proposition inventée, reprise au slot suivant ; erreur fournisseur fatale.
- Attestation : proposition déjà admise, expiration de la signature de vote,
  aucune attestation publiée, cible réservée retrouvée après réouverture du
  journal, nouvelle proposition au slot suivant ; erreur fournisseur fatale.

Résultats : proposition 5,23 s et RANDAO 4,73 s dans clock-fixture-ctest.log ;
attestation 5,26 s dans attestation-ctest.log. La chaîne signée passe en 3,42 s
dans initial-ctest.log, sur le même changement de code de production.
Les deux échecs intermédiaires d'attestation sont conservés : injection non
atteinte à l'epoch initiale, puis saut d'horloge supérieur au pas autorisé. La
fixture finale avance l'horloge slot par slot jusqu'à l'epoch 1. Aucun garde de
production n'a été assoupli. L'assertion de reprise exige une proposition de
plus que le compteur précédant l'expiration.

Ces contrôles ne prouvent ni toute fenêtre temporelle de signature, ni la
reprise WAN. Les trois validateurs de la campagne WAN échouée restaient actifs :
ce lot n'est pas présenté comme sa correction. Le binaire réseau n'est pas
reconstruit ni déployé par cette qualification de l'exécutable de tests.
