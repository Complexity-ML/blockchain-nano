from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');r=Path(__file__).parent;p=r/'proof';prev=repo/'docs/qualification/restored-three-node-v35-trace-bound-prepare-20261007';q=repo/'docs/qualification/core-trace-bound-http-20261007'
def read(x):return json.loads(x.read_text())
def sha(x):return hashlib.sha256(x.read_bytes()).hexdigest()
def save(x,d):x.write_text(json.dumps(d,indent=2)+'\n')
a=read(p/'artifacts.json');bindings={};newq=repo/'docs/qualification/capella-published-signed-inventory-20261007'
for node,arch in [('mac-native','native'),('mac-docker','arm64'),('vps-docker','amd64')]:
 n=a['nodes'][node];lines=(newq/(arch+'-build/programs.sha256')).read_text().splitlines();assert any(line.startswith(n['nano']['sha256']+'  ') and line.endswith('/nano') for line in lines)
 relay=(q/(arch+'-integration/binaries.sha256.log')).read_text().splitlines();assert any(line.startswith(n['p2p']['sha256']+'  ') and line.endswith('/nano-p2p') for line in relay)
 bindings[node]={'coreQualification':'capella-published-signed-inventory-20261007','nanoSHA256':n['nano']['sha256'],'relayQualification':'core-trace-bound-http-20261007','relaySHA256':n['p2p']['sha256'],'sourceBoundToCompiledCandidate':True}
save(p/'binary-qualification-binding.json',bindings)
# Revalidate historical storage with the freshly compiled candidate on every host.
handles=read(r/'original-handles.json')
for key in ['candidateReadOnly','vpsCandidateReadOnly']:assert handles[key]['status']=='TERMINAL' and handles[key]['exitCode']==0
for label in ['native','mac-docker','vps']:
 source=r/'candidate-readonly'/label;assert read(source/'exit.json')['exitCode']==0
 capsule=read(source/'capture.json');prior=read(repo/'docs/qualification/v35-post-stop-readonly-20261007'/label/'capture.json')
 for field in ['restart','validatorKeys','canonicalReplay']:assert capsule[field]==prior[field]
 target=p/'candidate-readonly'/label;target.mkdir(parents=True)
 for name in ['capture.json','exit.json','error.log']:shutil.copyfile(source/name,target/name)
 proof='immutability.json' if label=='vps' else 'binding.json';assert read(source/proof)['hashesUnchanged'];shutil.copyfile(source/proof,target/proof)
for name in ['candidate-readonly.py','candidate-readonly-original.log','vps-candidate-readonly.py','vps-candidate-readonly-original.log']:shutil.copyfile(r/name,p/name)
oldplan=read(prev/'plan.json');newplan=read(p/'plan.json')
for before,after in zip(oldplan['configs'],newplan['configs']):
 for f in ['limits','catalogs','clock','coldCheckpoint','network']:assert before['capella'][f]==after['capella'][f]
assert oldplan['baseline']==newplan['baseline']
for n in ['copy-local.py','local-copy-original.log','local-copy-receipt.json','prepare-vps.py','vps-backup-original.log','prepare.py','prepare-original.log','finalize.py']:shutil.copyfile(r/n,p/n)
handles=read(r/'original-handles.json');assert handles['prepare']['exitCode']==0;save(p/'terminal-handles.json',handles)
stop=read(Path('/private/tmp/nano-restored-three-node-v35-trace-bound-20261007/campaign/native-state.json'));assert stop['exitCode']==0 and stop['signalCode'] is None
(p/'README.md').write_text('''# V36 — préparation avec inventaires signés publiés

Sources V35 arrêtées0 et intégralement auditées. Copies Mac1914/1945 fichiers (peer.key seul exclu), sauvegarde VPS3113 fichiers exacte et indépendante. Pins actifs16/16/33 et historiques16/16/0 conservés séparément. Journaux et témoins conservés, aucune clé réinitialisée.

Nouveau cœur qualifié par les trois parcours ciblés, consommateurs ABI et programmes recompilés ; relais b2af22df inchangé. Config VPS dérivée sur son hôte et comparée au plan indépendant. Budgets, profil, horloge, délais et règles inchangés. Images existantes réutilisées. Aucun signataire lancé par cette préparation.

V35 FAILED reste conservée. Les prochaines duties et trois finalités communes distinctes qui avancent restent à démontrer. Aucun critère production, EIP, Base–Nano/Doppler ou revue indépendante promu.
''')
manifest={'sourceCore':a['coreRevision'],'sourceRelay':a['relayRevision'],'sourceCheckout':a['coreRevision'],'sourceCampaign':'V35-clean-stop-and-full-readonly-audit','sourceMacContainer':'nano-restored-three-node-mac-muxdz001','sourceVpsContainer':'nano-restored-three-node-vps-muxdz001','sourceMacExpectedExit':0,'sourceVpsExpectedExit':0,'sourceNativeStop':stop,'localRunDirectory':'/private/tmp/nano-restored-three-node-v36-signed-inventory-20261007','vpsCopiedVolume':'nano-restored-three-node-vps-20261005-data','vpsSourceBackup':read(p/'vps-local-backup-receipt.json')['backupLocation'],'vpsPhysicalBackupVerified':True,'sourceMacSnapshotsPreserved':True,'signerJournalsNotReset':True,'sourceLimitsUnchanged':True,'sourceBudgetsUnchanged':True,'signersStarted':False,'fullJournalsAndWitnessesPreserved':True,'networkQualified':False,'productionQualified':False};files={str(x.relative_to(p)):sha(x) for x in sorted(p.rglob('*')) if x.is_file()};manifest.update(files=files,artifacts=files);save(p/'manifest.json',manifest)
shutil.copytree(p,repo/'docs/qualification/restored-three-node-v36-signed-inventory-prepare-20261007')
msg='Préparation V36 terminée depuis V35 auditée : copies Mac1914/1945 et sauvegarde VPS3113 fichiers exactes ; pins actifs16/16/33 et historiques16/16/0 conservés. Nouveau cœur avec inventaires signés publiés qualifié sur trois environnements ; relais inchangé. [Preuves](qualification/restored-three-node-v36-signed-inventory-prepare-20261007/README.md). Aucun signataire lancé ; budgets/profil/horloge/délais conservés. Finalité commune, production, Base–Nano/Doppler et revue indépendante restent ouverts.'
for f in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 x=repo/'docs'/f;s=x.read_text();marker='## État courant vérifié — 7 octobre 2026\n\n';assert marker in s;x.write_text(s.replace(marker,marker+msg+'\n\nÉtat précédent conservé : ',1))
print('V36 preparation sealed; all source/binary/pin bindings exact; no signer started')
