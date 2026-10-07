from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');r=Path(__file__).parent;p=r/'proof'
def read(x):return json.loads(x.read_text())
def sha(x):return hashlib.sha256(x.read_bytes()).hexdigest()
def save(x,d):x.write_text(json.dumps(d,indent=2)+'\n')
a=read(p/'artifacts.json');q=repo/'docs/qualification/capella-startup-reference-memo-20261007';old=read(repo/'docs/qualification/restored-three-node-v33-published-inventory-prepare-20261007/artifacts.json');bindings={}
for node,arch in [('mac-native','native'),('mac-docker','arm64'),('vps-docker','amd64')]:
 n=a['nodes'][node]
 if arch=='native':assert n['nano']['sha256']==read(q/'native-repair/programs.json')['nano']
 else:assert n['nano']['sha256']+'  '+n['nano']['path'] in (q/(arch+'-repair/programs.sha256')).read_text().splitlines()
 assert n['p2p']==old['nodes'][node]['p2p']
 bindings[node]={'coreQualification':'capella-startup-reference-memo-20261007','nanoSHA256':n['nano']['sha256'],'relayUnchanged':True,'relaySHA256':n['p2p']['sha256']}
save(p/'binary-qualification-binding.json',bindings)
crash=read(r/'current-crash-audit-verification.json');capsule=read(repo/'docs/qualification/v33-post-stop-readonly-20261007/vps/capture.json');assert crash['currentRecoveryCommitment']==capsule['restart'] and crash['capsuleSHA256']==sha(repo/'docs/qualification/v33-post-stop-readonly-20261007/vps/capture.json') and crash['externalSignerPinsUnchanged']==33
assert 'ℹ pass 57' in (r/'crash-helper-tests-original.log').read_text() and 'ℹ fail 0' in (r/'crash-helper-tests-original.log').read_text()
for n in ['copy-local.py','local-copy-original.log','vps-backup.py','vps-backup-original.log','bind-vps-audit.py','bind-vps-audit-original.log','bind-vps-crash-audit.py','bind-vps-crash-audit-original.log','prepare.py','prepare-original.log','current-crash-audit-verification.json','crash-helper-tests-original.log','finalize.py']:shutil.copy2(r/n,p/n)
save(p/'terminal-handles.json',{'localCopy':{'originalSession':68692,'exitCode':0,'terminalChunk':'7cfec5'},'vpsBackup':{'originalSession':72681,'exitCode':0,'terminalChunk':'f77707'},'bindAudit':{'originalSession':56384,'exitCode':0,'terminalChunk':'f35bea'},'prepare':{'originalSession':62974,'exitCode':0,'terminalChunk':'c501dd'},'helperTests':{'originalSession':73428,'exitCode':0,'terminalChunk':'3773e7'},'bindCrashAudit':{'originalSession':45956,'exitCode':0,'terminalChunk':'6649a8'}})
(p/'README.md').write_text('''# V34 restoration preparation — 7 October 2026

New core c4996498 startup certificate reuse is scoped-qualified on native/ARM64/AMD64; binary hashes re-read and tied to those logs. Relay77848f1f unchanged. Existing Docker images/build volumes reused; no image rebuild. Full independent VPS copy profile ready35500ms, clean stop,33 signer pins unchanged, but no IPC readiness observed before immediate stop, no duties or finality. This does not qualify WAN or resolve V33 FAILED.

Mac sources remain audited V32 clean stops: copies1842/1832 files, excluding only peer.key, full journals/witnesses and independent inodes. VPS source is V33 forced-stop137/noOOM, read-only replay audit current core0x3b94,2936 files unchanged, primary/independent witness validated and all33 external signer pins unchanged. Independent physical backup remains on VPS. No disposable diagnostic copy adopted. Active16/16/33 and historical16/16/0 pins verified separately; config independently derived on VPS matches JS planner.

Preparation helper now explicitly accepts an independently audited crash source rather than claiming a clean stop: required proof format, stopped137/noOOM, read-only/minimum bounds/primary-witness validation, all signer pins exact, current capsule SHA and recovery commitment bound. Legacy healthy stop support retained.57 helper/source-stop tests pass, including missing/altered crash evidence and unbound capsule rejection; real host-local proof accepted. Source configuration/data/env remain private. No guards/deadlines/consensus limits/signing rules relaxed.

Original4096 block budget/catalogue128KiB/cache8/16MiB, clocks/profile and controller unchanged. No signer launched by preparation. Next run must demonstrate actual duties and three distinct advancing common finalized checkpoints within900seconds after readiness, clean stops and durable ancestry. All historical failures, WAN A/partitions/lifecycle/rotation/sanctions/resource/release/independent-review/EVM/bridge/Doppler criteria remain open. No sale, funds or public deployment.
''')
stop=read(Path('/private/tmp/nano-restored-three-node-v32-preparation-priority-20261007/campaign/native-state.json'));assert stop['exitCode']==0 and stop['signalCode'] is None
m={'sourceCore':a['coreRevision'],'sourceRelay':a['relayRevision'],'sourceCheckout':a['coreRevision'],'sourceCampaign':'V32-Mac-and-V33-audited-VPS','sourceMacContainer':'nano-restored-three-node-mac-muxae47a','sourceVpsContainer':'nano-restored-three-node-vps-muxbpvx9','sourceMacExpectedExit':0,'sourceVpsExpectedExit':137,'sourceCrashReadonlyAudited':True,'sourceNativeStop':stop,'localRunDirectory':'/private/tmp/nano-restored-three-node-v34-startup-memo-20261007','vpsCopiedVolume':'nano-restored-three-node-vps-20261005-data','vpsSourceBackup':read(p/'vps-local-backup-receipt.json')['backupLocation'],'vpsPhysicalBackupVerified':True,'sourceMacSnapshotsPreserved':True,'signerJournalsNotReset':True,'sourceLimitsUnchanged':True,'sourceBudgetsUnchanged':True,'signersStarted':False,'fullJournalsAndWitnessesPreserved':True,'networkQualified':False,'productionQualified':False}
f={str(x.relative_to(p)):sha(x) for x in sorted(p.rglob('*')) if x.is_file()};m.update(files=f,artifacts=f);save(p/'manifest.json',m)
shutil.copytree(p,repo/'docs/qualification/restored-three-node-v34-startup-memo-prepare-20261007')
msg='Préparation V34 : cœur c4996498 qualifié natif/ARM64/AMD64, relais77848 inchangé ; copies Mac1842/1832 et sauvegarde VPS2936 fichiers exactes, pins actifs16/16/33 et historiques16/16/0 vérifiés. Source VPS137 auditée, capsule courante0x3b94 liée par SHA ; outil explicite après crash et57 tests passent. [Preuves](qualification/restored-three-node-v34-startup-memo-prepare-20261007/README.md). Aucun signataire lancé ; budgets/délais conservés. Reprise effective et trois finalités communes restent à démontrer, V33 FAILED et tous autres critères/revue indépendante conservés.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 x=repo/'docs'/name;s=x.read_text();marker='## État courant vérifié — 7 octobre 2026\n\n';assert marker in s;x.write_text(s.replace(marker,marker+msg+'\n\nÉtat précédent conservé : ',1))
print('V34 preparation complete; source crash explicit; no writer started')
