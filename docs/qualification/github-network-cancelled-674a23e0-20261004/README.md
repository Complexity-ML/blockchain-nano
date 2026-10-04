# CI réseau annulée — preuve conservée

Run GitHub 37228861808, révision 674a23e035b3dce5a39b07fd24f9f750dcfc593b,
job réseau 111514133627. Le build s’arrête dans le log publié à 73 %,
conclusion cancelled ; aucun résultat terminal des tests réseau ni qualification.
Installation et configuration réussies, artefact partiel uploadé.
Sources/version et logs byte-exact (gzip si volumineux), hashes dans archive-manifest.json.

Le workflow avait cancel-in-progress:true dans un groupe partagé par branche.
Le push e87fc7b8 a déclenché le run 37231284533 et interrompu le run actif
à 20:13:40 UTC. Le nouveau run saute core/network et ne remplace pas leurs preuves.
Correction : cancel-in-progress:false, conservation du même groupe pour éviter
le chevauchement des runs ; un run déjà actif pourra terminer au prochain push.
Cette correction ne transforme pas l’annulation en succès. La prochaine
qualification réseau automatique doit encore produire ses résultats terminaux.
Aucun lien avec la campagne VPS efbeb9bc, qui reste active et indépendante.
