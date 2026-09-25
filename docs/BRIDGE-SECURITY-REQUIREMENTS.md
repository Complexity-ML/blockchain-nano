# Bridge USDC : critères de sécurité avant fonds réels

État au 25 septembre 2026 : exigences, pas implémentation ni qualification du
bridge. Les tests EVM, BFT et P2P ne prouvent pas la sécurité des réserves USDC.

## Scénario Ronin

Dans l'incident Ronin de 2022, la compromission des accès à quatre validateurs
Sky Mavis et à un cinquième via une délégation non révoquée a permis de franchir
le seuil autorisant les opérations du bridge. Source :
[analyse par Andrew Pratt, chercheur en sécurité, HackerOne](https://www.hackerone.com/blog/lessons-crypto-exploits).

Un bridge qui libère des fonds sur la seule base d'un quorum de signatures ne
peut pas distinguer un quorum honnête d'un quorum dont toutes les clés nécessaires
ont été volées. BLS, un seuil plus élevé ou le mot BFT ne suppriment pas cette
hypothèse de confiance. Plusieurs clés ou conteneurs sur le même VPS ne constituent
pas des domaines de compromission indépendants. La sécurité du consensus Nano
et l'autorisation des retraits sur la chaîne externe doivent être analysées
séparément, y compris leurs clés d'administration et de mise à niveau.

## Critères de clôture requis

- Définir les deux chaînes, les contrats et actifs, la finalité attendue et le
  mécanisme exact de preuve des dépôts/retraits. Documenter ce que chaque
  contrat vérifie lui-même et ce qu'il délègue aux signataires/relayers.
- Pour un modèle à signataires : préciser le quorum et ses domaines de
  compromission réels. Une concentration de clés sur le VPS initial doit
  apparaître comme confiance en un opérateur unique, jamais comme indépendance.
- Vérifier les événements et leur finalité avant émission ou libération ; lier
  chaque message à la chaîne source/destination, au contrat, à l'actif, au
  montant, au destinataire et à un identifiant consommable une seule fois.
- Qualifier les rôles de mint, de retrait, d'administration et de mise à niveau.
  Aucune clé d'administration oubliée ni délégation obsolète ne doit permettre
  de contourner le mécanisme de preuve ou le quorum annoncé.
- Définir et tester les mécanismes de limitation des pertes et de réaction
  retenus : plafonds de réserves et de sorties, temporisation, alerte, arrêt
  du bridge et rotation/révocation des clés. Ces mécanismes ne constituent pas
  une preuve de dépôt ; leur gouvernance et leurs conséquences sur les retraits
  doivent être explicites. Ils concernent le bridge, pas une censure générale
  des transactions de la L1.
- Réconcilier les réserves avec les jetons émis et les retraits en attente.
  Ne pas créditer une pool avec un actif présenté comme USDC garanti avant
  cette qualification et l'autorisation explicite des fonds réels.

## Scénarios adversariaux à implémenter et vérifier

| Scénario | Résultat à démontrer | État |
| --- | --- | --- |
| Relayer compromis, dépôt inventé | Pas d'émission sans preuve exigée par le modèle | Non commencé |
| Clés volées sous le quorum | Aucun retrait autorisé par ces seules signatures | Non commencé |
| Quorum entier compromis | Limites de confiance et pertes possibles démontrées ; aucune affirmation d'immunité | Non commencé |
| Délégation ancienne ou signataire révoqué | Refus après activation de la révocation selon les règles documentées | Non commencé |
| Clé admin/upgrade compromise | Voies de contournement identifiées et comportement de protection testé | Non commencé |
| Rejeu entre chaînes, forks ou contrats | Refus du message hors de son domaine | Non commencé |
| Même dépôt/retrait présenté plusieurs fois | Un seul effet sur les réserves et l'offre | Non commencé |
| Réorganisation avant finalité | Aucun crédit définitif d'un dépôt annulé | Non commencé |
| Faux montant, actif ou destinataire | Refus malgré un identifiant de dépôt existant | Non commencé |
| Relayer en panne puis reprise | Reprise sans double émission ni perte de retrait | Non commencé |
| Dépôt, swap et retrait vers la source | Retour effectif de l'actif garanti, réserves cohérentes | Non commencé |

Les bornes de sécurité du modèle choisi doivent être publiées avec les preuves.
Un audit indépendant reste une exigence distincte ; ces tests ne le remplacent pas.
