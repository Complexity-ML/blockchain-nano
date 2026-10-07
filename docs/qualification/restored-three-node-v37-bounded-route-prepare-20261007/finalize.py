from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');r=Path(__file__).parent;p=r/'proof';root=Path('/private/tmp/nano-restored-three-node-v37-bounded-route-20261007');audits=Path('/private/tmp/nano-v36-post-stop-readonly-20261007')
def read(x):return json.loads(x.read_text())
def sha(x):return hashlib.sha256(x.read_bytes()).hexdigest()
def save(x,d):x.write_text(json.dumps(d,indent=2)+'\n')
h=read(r/'original-handles.json');assert all(x['exitCode']==0 and x['status']=='TERMINAL' for x in h.values());a=read(p/'artifacts.json');assert a['coreRevision']=='4ce3e4e2eaf5ea87372b9ca8eff399d23fdf9510' and a['relayRevision']=='4b2165b8a642811562bf8ddca0f0750fe1dcb079'
q=repo/'docs/qualification/bounded-dependency-route-integrated-20261007';core=repo/'docs/qualification/capella-published-signed-inventory-20261007';bindings={}
for node,arch,label in [('mac-native','native','native'),('mac-docker','arm64','mac-docker'),('vps-docker','amd64','vps')]:
 n=a['nodes'][node];assert any(l.startswith(n['nano']['sha256']+'  ') and l.endswith('/nano') for l in (core/(arch+'-build/programs.sha256')).read_text().splitlines())
 if arch=='native':assert n['p2p']['sha256']==read(repo/'docs/qualification/bounded-dependency-route-native-20261007/binding.json')['newMainSHA256']
 else:assert any(l.startswith(n['p2p']['sha256']+'  ') and l.endswith('/main') for l in (q/('docker-'+arch+'/programs-sha256.log')).read_text().splitlines())
 capsule=repo/'docs/qualification/v36-post-stop-readonly-20261007'/label;assert read(capsule/'exit.json')['exitCode']==0;i=read(capsule/'immutability.json');assert i['hashesUnchanged'] and i['exactStopCommitmentsVerified'] and i['primaryAndIndependentWitnessVerified']
 dst=p/'reused-readonly-audits'/label;dst.mkdir(parents=True)
 for f in ['capture.json','immutability.json','exit.json','error.log']:shutil.copy2(capsule/f,dst/f)
 bindings[node]={'coreQualification':'capella-published-signed-inventory-20261007','nanoSHA256':n['nano']['sha256'],'relayQualification':'bounded-dependency-route-integrated-20261007','relaySHA256':n['p2p']['sha256'],'auditedCapsuleSHA256':sha(capsule/'capture.json'),'coreUnchanged':True,'auditReusedByExactByteIdentity':True,'freshCaptureClaimed':False}
 if arch!='amd64':
  data=root/node/'data';expected=read(root/node/'data-copy-hashes.json');assert {str(f.relative_to(data)):sha(f) for f in data.rglob('*') if f.is_file()}==expected
  prior=read(audits/label/'data-before.json');assert expected=={k:v for k,v in prior.items() if k!='peer.key'}
save(p/'binary-qualification-binding.json',bindings);save(p/'audit-reuse.json',{'coreUnchanged':a['coreRevision'],'audits':'v36-post-stop-readonly-20261007','localCopiesWholeFileHashesExactExcludingPeerIdentity':True,'vpsWholeFilePhysicalBackupExact':True,'freshCaptureClaimed':False,'sourceAuditsSessions':[5836,89720],'sourceAuditsTerminalExit0':True,'sourceChainFinalityQualified':False})
oldplan=read(repo/'docs/qualification/restored-three-node-v36-signed-inventory-prepare-20261007/plan.json');newplan=read(p/'plan.json')
for before,after in zip(oldplan['configs'],newplan['configs']):
 for f in ['limits','catalogs','clock','coldCheckpoint','network']:assert before['capella'][f]==after['capella'][f]
assert oldplan['baseline']==newplan['baseline']
for f in ['copy-local.py','local-copy-original.log','local-copy-receipt.json','prepare-vps.py','vps-backup-original.log','prepare.py','prepare-original.log','reuse-plan.json','finalize.py']:shutil.copy2(r/f,p/f)
save(p/'terminal-handles.json',h);stop=read(Path('/private/tmp/nano-restored-three-node-v36-signed-inventory-20261007/campaign/native-state.json'));assert stop['exitCode']==0 and stop['signalCode'] is None
(p/'README.md').write_text('''# V37 — préparation avec relais à chemin borné

Sources V36 arrêtées0, audits exacts terminés0 sur trois environnements. Copies Mac1971/1956 fichiers (peer.key seul exclu), sauvegarde physique VPS3198 fichiers, hashes exacts et inodes indépendants. Pins actifs16/16/33 et historiques16/16/0 conservés séparément. Aucun journal/témoin réinitialisé.

Cœur4ce3e4e2 et programmes nano inchangés ; audits V36 réutilisés avec la même révision et identité exacte des fichiers source/copies. Aucune nouvelle capture C++ n’est revendiquée. Nouveau relais4b2165b8 qualifié dans les trois environnements ; alias de mêmes octets, images existantes réutilisées. Config VPS dérivée localement et comparée au plan indépendant. Budgets, profil, horloge, règles, délais900s et critère de trois nouvelles finalités communs inchangés.

Aucun signataire lancé par cette préparation. V36 FAILED et tous les échecs historiques restent conservés. Reprise effective, finalité commune, autres écarts consensus/ressources, release/CI GitHub facturation, Base–Nano/Doppler et revue indépendante restent ouverts. Aucun profil/EIP supplémentaire qualifié.
''')
manifest={'sourceCore':a['coreRevision'],'sourceRelay':a['relayRevision'],'sourceCheckout':a['relayRevision'],'sourceCampaign':'V36-clean-stop-and-full-readonly-audit','sourceMacContainer':'nano-restored-three-node-mac-muxfgr38','sourceVpsContainer':'nano-restored-three-node-vps-muxfgr38','sourceMacExpectedExit':0,'sourceVpsExpectedExit':0,'sourceNativeStop':stop,'localRunDirectory':str(root),'vpsCopiedVolume':'nano-restored-three-node-vps-20261005-data','vpsSourceBackup':read(p/'vps-local-backup-receipt.json')['backupLocation'],'vpsPhysicalBackupVerified':True,'sourceMacSnapshotsPreserved':True,'signerJournalsNotReset':True,'sourceLimitsUnchanged':True,'sourceBudgetsUnchanged':True,'signersStarted':False,'fullJournalsAndWitnessesPreserved':True,'coreAuditsReusedByExactByteIdentity':True,'networkQualified':False,'productionQualified':False};files={str(x.relative_to(p)):sha(x) for x in sorted(p.rglob('*')) if x.is_file()};manifest.update(files=files,artifacts=files);save(p/'manifest.json',manifest)
shutil.copytree(p,repo/'docs/qualification/restored-three-node-v37-bounded-route-prepare-20261007')
msg='Préparation V37 terminée : copies indépendantes Mac1971/1956 et sauvegarde VPS3198 fichiers exactes ; pins actifs16/16/33 et historiques16/16/0 conservés. [Preuves](qualification/restored-three-node-v37-bounded-route-prepare-20261007/README.md). Relais4b2165b8 qualifié sur les trois environnements, cœur4ce3e4e2 inchangé ; audits V36 réutilisés sur identité exacte des fichiers, sans nouvelle capture revendiquée. Aucun signataire lancé ; budgets/profil/horloge/délais conservés, images réutilisées. Finalité commune, CI facturation/release, production/Base–Nano/Doppler et revue indépendante restent ouverts.'
for f in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 x=repo/'docs'/f;s=x.read_text();marker='Relais à chemin de dépendances borné intégré';assert marker in s;x.write_text(s.replace(marker,msg+'\n\nÉtat précédent conservé : '+marker,1))
print('V37 preparation sealed; exact reuse/core/relay/physical-copy/pin bindings, no signer started')
