# Slots et époques Capella

Le traitement utilise les racines SSZ Capella à chaque slot, conserve les
headers, curseurs de retrait et résumés, puis compose les opérations communes
d'époque avec les paramètres du fork. La projection interne vers les champs
communs ne sert jamais à authentifier une racine Capella.

À la frontière historique, Capella ajoute les racines distinctes des vecteurs
block_roots et state_roots dans historical_summaries. La liste historique
antérieure demeure inchangée. Altair et Bellatrix continuent d'appeler leur
mise à jour historique habituelle via leurs fonctions publiques.

Sept scénarios officiels épinglés, 100 slots : états SSZ complets identiques
et rejeu slot par slot équivalent. Sont couverts les époques vides, frontières,
historiques et effet des balances sur les proposeurs. Cinq refus de
configuration/chronologie, budget de slots, échec tardif de sélection sync
avec reprise, sept annulations. Contrôles supplémentaires d'un header non
vide, de curseurs non nuls et de la racine Capella mise en cache.
Régression Altair/Bellatrix/Capella : 3/3.

Référence : [historiques Capella v1.4.0](https://github.com/ethereum/consensus-specs/blob/v1.4.0/specs/capella/beacon-chain.md#new-process_historical_summaries_update).
Ces candidats ne publient rien en base et supposent un parent authentifié.
Restent la composition du bloc signé, l'activation Bellatrix→Capella, le
Store/fork-choice/réacteur et le commit durable. Pas d'activation réseau.
