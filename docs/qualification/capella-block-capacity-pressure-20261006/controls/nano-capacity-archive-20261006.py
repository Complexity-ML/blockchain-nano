from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');raw=Path('/private/tmp/nano-capacity-evidence-20261006');dest=repo/'docs/qualification/capella-block-capacity-pressure-20261006'
assert not dest.exists()
for arch in ['arm64','amd64']:
 folder=raw/arch/f'capacity-pressure-{arch}-5902c983-20261006'
 for name,expect in [('controller-exit.txt',0),('baseline-run-exit.txt',1),('fixed-run-exit.txt',0),('configured-runtime-exit.txt',0),('scheduler-exit.txt',0)]:assert (folder/name).read_text().strip()==str(expect)
 for line in (folder/'sources.sha256').read_text().splitlines():
  sha,path=line.split(maxsplit=1);assert path.startswith('/work/');assert hashlib.sha256((repo/path[len('/work/'):]).read_bytes()).hexdigest()==sha,(arch,path)
 assert 'full graph invoked automatic proposal signer' in (folder/'baseline-run.log').read_text()
 row=json.loads((folder/'fixed-run.log').read_text().splitlines()[-1]);assert row['providerCalls']==0 and row['journalUnchanged'] and row['clockAdvanced'] and row['witnessRestored'] and row['incomingRefusals']==3 and row['independentValidAdmission']
 for name in ['configured-runtime','scheduler']:
  rows=[json.loads(line) for line in (folder/(name+'.log')).read_text().splitlines() if line.startswith('{')];assert rows
  row=rows[-1];assert row['witnessRestart'] and not row.get('networkTransport',True)
  if name=='configured-runtime':assert row['blocks']==16 and row['keysLoaded']==64
  else:assert row['blocks']==40 and row['votes']==272 and row['syncMessages']==1280 and row['finalizedEpoch']==3
shutil.copytree(raw,dest)
controls=dest/'controls';controls.mkdir()
for path in ['/private/tmp/nano-capacity-build-20261006.sh','/private/tmp/nano-capacity-public-20261006/build.sh','/private/tmp/nano-capacity-collect-20261006.py','/private/tmp/nano-capacity-archive-20261006.py']:
 name='build-amd64.sh' if path.endswith('/build.sh') else Path(path).name;shutil.copyfile(path,controls/name)
summary={'baseCommit':'5902c9838910c7fcc618ee230c9b6727cb444313','parentCoreProducer':'b5ee00acce4bd1a73bbd3c3024102d157315fe92','sourceHashes':{p:hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in ['src/consensus_capella_signing.cpp','src/consensus_capella_reactor.cpp','tests/test_capella_transition.cpp','CMakeLists.txt']},'qualifiedArchitectures':['docker-arm64','docker-amd64'],'baselineExit':1,'fixedExit':0,'configuredRuntimeBlocks':16,'configuredRuntimeKeys':64,'schedulerBlocks':40,'schedulerVotes':272,'schedulerSyncMessages':1280,'freshCMakeConfigureQualified':False,'nativeCandidateQualified':False,'runtimeNanoRelinked':False,'deployed':False,'capacityRecoveryQualified':False,'wanFinalityQualified':False,'signAdmissionRaceQualified':False,'productionQualified':False}
(dest/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
readme='''# Refus de capacité de blocs avant signature — 6 octobre 2026

Les API de proposition automatique et explicite refusent une nouvelle proposition quand le nombre de blocs retenus atteint la limite locale configurée. La proposition automatique refuse avant le fournisseur RANDAO ; l’explicite avant la réservation dans le journal anti-double-signature. L’admission entrante refuse un nouveau bloc avant sa préparation et sa publication ; les doublons déjà retenus gardent leur chemin existant. Aucun changement du poids des validateurs, des règles de consensus ou de la capacité configurée.

Une fixture avec le cœur réel et RocksDB atteint exactement la capacité du graphe initial. L’ancien cœur invoque le fournisseur et signe quand même : assertion reproduite, sortie 1 conservée. Le correctif rend une absence de proposition, zéro appel fournisseur et garde le journal intact. Trois refus entrants gardent le token et le tip inchangés. Un autre récepteur provisionné admet le même bloc signé valide. L’horloge avance encore, sans agrandir le graphe ; fermeture puis restauration via témoin gardent token et journal. ARM64 et AMD64 : sorties 0. Les unités recompilées remplacent seulement celles de la copie isolée du cœur b5ee00ac ; les archives et exécutables parents restent inchangés.

Régressions concernées sur les deux architectures : runtime configuré, 16 blocs/64 clés et restauration avec témoins ; scheduler, 40 blocs/272 attestations/1280 messages sync, finalité epoch 3 et journaux rouverts. Ce sont des scénarios locaux, sans transport réseau. Le premier contrôleur runtime ARM64 est sorti 1 car /work et ses fixtures n’étaient pas montés ; état et erreur conservés, correction du montage et nouveau contrôleur sortie 0, sans recompilation. Aucune image reconstruite.

Limites : le scénario de pression ne démontre pas que la boucle complète de validation continue toutes ses duties sous saturation. La course entre signature et admission concurrente reste ouverte. Le correctif ne libère pas d’espace, ne prouve pas le drainage/pruning ni une reprise de finalité, et ne répare pas à lui seul la campagne v7. Préservation des contextes authentifiés et progrès de finalité sous pression restent nécessaires. Autres budgets (octets d’état/enveloppe) non qualifiés ici.

La nouvelle cible CTest est enregistrée, mais les commandes ont réutilisé les flags et liens CMake existants ; pas de configuration fraîche qualifiée. Aucun nouveau binaire nano runtime lié/déployé, pas de qualification native Mac de ce nouveau cœur. Nœuds et journaux v7 laissés arrêtés, aucun transfert de stockage ni réinitialisation, aucun fonds ou lancement public.

summary.json identifie sources et limites ; programs.sha256 identifie archives/exécutables de test. Les seuls états Docker conservés sont .State, sans environnements. manifest.json couvre tous les fichiers sauf lui-même.
'''
(dest/'README.md').write_text(readme)
append='''\n## Refus de capacité avant signature — 6 octobre 2026\n\nGarde de capacité de blocs implémentée dans la proposition et l’admission :\nrefus avant fournisseur/réservation/publication quand le graphe est plein,\nsans augmenter sa limite. Régression avant/après Docker ARM64 et AMD64 :\nancien cœur signe (échec 1 conservé), correctif zéro appel fournisseur, journal\nintact, refus sans écriture, horloge active et restauration avec témoin (sortie 0).\nRuntime 16 blocs/64 clés et scheduler 40 blocs/272 votes/1280 sync passent\nsur les deux architectures. Échec initial de montage des fixtures ARM64 conservé.\nQualification locale seulement ; nouveau cœur natif/runtime non livré, course\nsignature/admission, autres budgets, libération de capacité et reprise de finalité\nrestent ouverts. Aucun rebuild d’image ni relance WAN.\n[Preuves et périmètre](qualification/capella-block-capacity-pressure-20261006/README.md).\n'''
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+append)
manifest={str(p.relative_to(dest)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(dest.rglob('*')) if p.is_file()}
(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Archived',len(manifest),'files after source/terminal/scope verification')
