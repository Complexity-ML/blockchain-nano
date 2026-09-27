# Reprise Capella avec témoin

Le mode `automaticRecovery` permet au même fichier de configuration de servir
au premier démarrage et aux reprises. Il est désactivé par défaut.

Dans la section `capella` d'une configuration déjà validée :

```json
{
  "automaticRecovery": true,
  "recoveryWitness": "/var/lib/nano/witness",
  "initial": {
    "file": "/config/initial.json",
    "sha256": "<empreinte du snapshot initial approuvé>"
  },
  "restart": null
}
```

Cet extrait complète les paramètres obligatoires existants (profil, limites,
horloge, etc.) ; ce n'est pas un fichier de lancement autonome.

## Décision de démarrage

- Sans checkpoint dans les deux stores : charger le snapshot initial authentifié.
- Avec un checkpoint dans l'un ou l'autre : exiger la reprise par témoin. Le
  réacteur vérifie son intention de publication et la correspondance avec le
  checkpoint primaire ; une erreur ne déclenche jamais un retour au bootstrap.
- Témoin vide mais checkpoint primaire présent : refuser le démarrage.

Le répertoire de chaîne, le témoin, les journaux anti-double-signature et leurs
propres témoins doivent persister entre les redémarrages. Ne pas les supprimer
pour contourner un refus. Conserver aussi l'identité P2P. Les clés de validation
restent propres à chaque opérateur ; les clés publiques des fixtures de test ne
conviennent pas à un réseau public.

## Après un redémarrage

Vérifier l'identité P2P, le checkpoint finalisé conservé, puis l'avancement d'une
finalité commune avec les autres pairs. Un processus démarré, une réponse HTTP
ou une tête qui avance ne suffisent pas. En cas de refus, conserver les données
et logs pour diagnostiquer la différence entre primaire et témoin.

Ne pas réutiliser simultanément les mêmes clés depuis une copie restaurée et
un nœud encore actif. La sauvegarde/restauration cohérente de tous ces éléments
et la protection indépendante contre le rollback restent à qualifier.

## Limites de confiance et preuves

Le témoin constitue une frontière de confiance explicite. Un témoin et une base
restaurés ensemble à un ancien état ne fournissent pas de preuve indépendante
contre ce rollback. Si les deux sont perdus, ce mode ne peut pas distinguer cette
perte d'un premier lancement. Une politique opérationnelle externe reste nécessaire.

[Tests runtime](qualification/capella-automatic-recovery-20260927/README.md) :
reprise avec configuration conservée et refus d'un témoin manquant qualifiés.
[Campagne Docker](qualification/capella-automatic-docker-recovery-20260927/README.md) :
SIGKILL, 96 secondes hors ligne, reprise sans changer la configuration et
finalité commune d’époque 7 qualifiés sur deux nœuds locaux.
Aucune qualification générale de production ni modification du VPS n'est impliquée.
