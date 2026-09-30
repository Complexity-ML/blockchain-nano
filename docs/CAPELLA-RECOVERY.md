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

## Engagement externe pour une restauration

Le champ optionnel `capella.recoveryCommitment` permet d'exiger un checkpoint
exact conservé indépendamment de la sauvegarde :

```json
{
  "recoveryCommitment": {
    "sequence": "<quantité hexadécimale canonique positive>",
    "digest": "<empreinte 0x de 32 octets>"
  }
}
```

Il nécessite `recoveryWitness` et interdit le bootstrap. Le réacteur compare cet
engagement au champ `next` de l'intention du témoin avant toute restauration,
publication manquante, lecture d'horloge ou ouverture des signataires. Une paire
base/témoin cohérente mais ancienne est refusée si elle diffère de l'engagement.
L'intention et la relation primaire/témoin restent intégralement vérifiées.

L'engagement est exact, pas un minimum de séquence : un témoin plus récent est
également refusé. Après progression du nœud, il faut conserver un nouvel
engagement approuvé pour sa prochaine restauration. Le reçu d'arrêt fournit le
checkpoint, mais sa copie hors du répertoire restauré et sa protection relèvent
de l'opérateur. Ne pas reconstruire ce champ depuis la sauvegarde suspecte ni
l'effacer automatiquement après un refus. En cas de publication interrompue,
un engagement sur le `next` durable peut autoriser sa complétion ; un ancien
engagement sur `previous` ne la valide pas.

Ce champ protège le checkpoint de chaîne. Les journaux anti-double-signature
ont leurs propres engagements (`validator.keys[].restart`) et témoins : leur
conservation indépendante et leur restauration cohérente restent nécessaires.
Un engagement absent conserve le modèle de confiance historique par témoin.

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

## Sauvegarde/restauration hors ligne qualifiée — 30 septembre 2026

Sur ARM64 et VPS, le répertoire complet d'un nœud arrêté de C est archivé puis
restauré : tous les fichiers et SHA-256 concordent (chaîne, témoins, journaux,
identité P2P). Audit canonique restauré réussi, source et copie inchangées.
[Preuves et procédure](qualification/capella-backup-restore-20260930/README.md).
Cette preuve reste hors ligne et locale : reprise effective des signataires,
refus de sauvegardes anciennes/incomplètes, finalité commune après restauration,
stockage hors site et résistance de l'archive à une coupure restent ouverts.
