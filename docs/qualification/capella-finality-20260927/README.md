# Finalité FFG locale Capella

Deux nouveaux scénarios produisent 33 blocs signés avec transactions Shanghai,
retraits et attestations de comités. Chaque attestation est signée par une clé
de test réellement présente dans le registre, avec source, cible et domaine
dérivés du post-état. Les votes du slot précédent sont inclus dans le corps
SSZ suivant, puis traités par l'admission complète et le Store.

Participation complète : 256 attestations incluses, checkpoint justifié au
moins à l'époque 3 et finalisé à l'époque 2. Contrôle à 50 % des membres de
chaque comité : 128 attestations incluses, justification et finalité restent
à l'époque 0. Aucun réglage FFG n'est abaissé pour obtenir ces résultats.

Les deux scénarios conservent les refus, annulations, contrôles des contextes,
snapshots, commit/réouverture et deux SIGKILL chacun. Régression avec les trois
chaînes courtes précédentes : 3/3 suites, 13,10 secondes pour cette exécution.
L'alternance court/long du banc disque est une injection directe de fixtures
pour éprouver l'atomicité ; elle n'autorise pas une réorganisation du protocole
au-delà d'un checkpoint finalisé.

Limites : production et admission locales, pas transport P2P ni validateurs
sur plusieurs hôtes. Le contrôle 50 % est un scénario précis, pas l'ensemble
des seuils de stake et attaques possibles. Les branches Bellatrix à travers
l'activation, le réacteur Capella, les dépôts natifs et la campagne distribuée
restent ouverts. Ce résultat ne rend pas la chaîne prête pour la production.
