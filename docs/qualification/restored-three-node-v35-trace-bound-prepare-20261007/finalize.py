from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');r=Path(__file__).parent;p=r/'proof';prev=repo/'docs/qualification/restored-three-node-v34-startup-memo-prepare-20261007';q=repo/'docs/qualification/core-trace-bound-http-20261007'
def read(x):return json.loads(x.read_text())
def sha(x):return hashlib.sha256(x.read_bytes()).hexdigest()
def save(x,d):x.write_text(json.dumps(d,indent=2)+'\n')
a=read(p/'artifacts.json');old=read(prev/'artifacts.json');bindings={}
for node,arch in [('mac-native','native'),('mac-docker','arm64'),('vps-docker','amd64')]:
 assert a['nodes'][node]['nano']==old['nodes'][node]['nano'];n=a['nodes'][node]['p2p'];lines=(q/(arch+'-integration/binaries.sha256.log')).read_text().splitlines();assert any(l.startswith(n['sha256']+'  ') and l.endswith('/nano-p2p') for l in lines)
 bindings[node]={'coreUnchanged':True,'coreQualification':'capella-startup-reference-memo-20261007','nanoSHA256':a['nodes'][node]['nano']['sha256'],'relayQualification':'core-trace-bound-http-20261007','relaySHA256':n['sha256'],'aliasDigestVerified':True}
save(p/'binary-qualification-binding.json',bindings)
oldplan=read(prev/'plan.json');newplan=read(p/'plan.json')
for before,after in zip(oldplan['configs'],newplan['configs']):
 for f in ['limits','catalogs','clock','coldCheckpoint','network']:assert before['capella'][f]==after['capella'][f]
assert oldplan['baseline']==newplan['baseline']
for n in ['copy-local.py','local-copy-original.log','local-copy-receipt.json','prepare-vps.py','vps-backup-original.log','prepare.py','prepare-original.log','finalize.py']:shutil.copyfile(r/n,p/n)
save(p/'terminal-handles.json',{'localCopy':{'originalSession':91843,'exitCode':0,'terminalChunk':'8af4df'},'vpsBackupAndAuditBinding':{'originalSession':31892,'exitCode':0,'terminalChunk':'a64bc3'},'prepare':{'originalSession':29940,'exitCode':0,'terminalChunk':'71e10b'}})
stop=read(Path('/private/tmp/nano-restored-three-node-v34-startup-memo-20261007/campaign/native-state.json'));assert stop['exitCode']==0 and stop['signalCode'] is None
(p/'README.md').write_text('''# V35 — préparation de reprise avec diagnostic borné

Sources V34 arrêtées proprement0, puis auditées entièrement en lecture seule : aucun fichier changé, engagements exacts vérifiés séparément, principal/témoin indépendant et rejeu canonique complets. Copies Mac indépendantes1845/1879 fichiers (seule identité peer.key exclue) ; sauvegarde physique VPS3008 fichiers exacte avant réutilisation du volume. Journaux et témoins anti-double-signature conservés ; pins actifs16/16/33 et historiques16/16/0 vérifiés.

Cœur c4996498 inchangé, relais b2af22df qualifié nativement et sous Docker ARM64/AMD64 : borne de diagnostic partagée et catégorie d’inventaire autorisée. Les aliases de chemin Docker conservent le SHA du même binaire. Aucun rebuild d’image. Config VPS dérivée sur son hôte, comparée indépendamment au plan JS ; aucune configuration, clé ou base privée exportée.

Budgets4096 blocs/catalogue128KiB/cache8/16MiB, profil, horloge, contrôleur, fenêtre900secondes et gardes inchangés. Aucun signataire lancé par cette préparation. Le prochain run doit vérifier les duties puis trois checkpoints communs distincts qui avancent ; les diagnostics serviront à attribuer les refus, sans prétendre corriger la cause globale. V34 reste FAILED, ainsi que les échecs historiques. Production, autres campagnes consensus, EVM/RPC, Base–Nano/Doppler et revue indépendante restent ouverts.
''')
manifest={'sourceCore':a['coreRevision'],'sourceRelay':a['relayRevision'],'sourceCheckout':'b2af22df3ce5035321aa8b4330021b3d0616716b','sourceCampaign':'V34-clean-stop-and-full-readonly-audit','sourceMacContainer':'nano-restored-three-node-mac-muxcwsxz','sourceVpsContainer':'nano-restored-three-node-vps-muxcwsxz','sourceMacExpectedExit':0,'sourceVpsExpectedExit':0,'sourceNativeStop':stop,'localRunDirectory':'/private/tmp/nano-restored-three-node-v35-trace-bound-20261007','vpsCopiedVolume':'nano-restored-three-node-vps-20261005-data','vpsSourceBackup':read(p/'vps-local-backup-receipt.json')['backupLocation'],'vpsPhysicalBackupVerified':True,'sourceMacSnapshotsPreserved':True,'signerJournalsNotReset':True,'sourceLimitsUnchanged':True,'sourceBudgetsUnchanged':True,'signersStarted':False,'fullJournalsAndWitnessesPreserved':True,'networkQualified':False,'productionQualified':False};files={str(x.relative_to(p)):sha(x) for x in sorted(p.rglob('*')) if x.is_file()};manifest.update(files=files,artifacts=files);save(p/'manifest.json',manifest)
shutil.copytree(p,repo/'docs/qualification/restored-three-node-v35-trace-bound-prepare-20261007')
msg='Préparation V35 terminée : copies complètes Mac1845/1879 et sauvegarde VPS3008 fichiers exactes depuis V34 auditée ; pins actifs16/16/33 et historiques16/16/0 conservés. Cœur c4996498 inchangé, relais b2af22df qualifié sur les trois environnements, config VPS dérivée localement et liée au plan indépendant. [Preuves](qualification/restored-three-node-v35-trace-bound-prepare-20261007/README.md). Aucun signataire lancé par la préparation, budgets/horloge/délais inchangés. La reprise sert à attribuer les refus de catalogue et vérifier la finalité ; V34 FAILED et tous critères production/revue indépendante restent ouverts.'
for f in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 x=repo/'docs'/f;s=x.read_text();marker='## État courant vérifié — 7 octobre 2026\n\n';assert marker in s;x.write_text(s.replace(marker,marker+msg+'\n\nÉtat précédent conservé : ',1))
print('V35 preparation sealed; all source/binary/pin bindings exact; no signer started')
