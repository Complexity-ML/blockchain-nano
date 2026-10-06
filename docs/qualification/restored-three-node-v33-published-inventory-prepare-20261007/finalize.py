from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path(__file__).parent;proof=root/'proof'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
art=read(proof/'artifacts.json');expected=read(Path('/private/tmp/nano-published-block-inventory-draft-20261007/next-restoration-state.json'))['artifacts'];assert art==expected
bindings={};coreproof=repo/'docs/qualification/published-block-inventory-20261007';relay=repo/'docs/qualification/restored-three-node-v32-preparation-priority-prepare-20261007/artifacts.json';oldart=read(relay)
for node,arch in [('mac-native','native'),('mac-docker','arm64'),('vps-docker','amd64')]:
 if arch=='native':assert art['nodes'][node]['nano']['sha256']==read(coreproof/'native-build/programs.json')['nano']
 else:assert art['nodes'][node]['nano']['sha256']+'  '+art['nodes'][node]['nano']['path'] in (coreproof/(arch+'-runtime-link/programs.sha256')).read_text().splitlines()
 assert art['nodes'][node]['p2p']==oldart['nodes'][node]['p2p']
 bindings[node]={'coreQualification':'published-block-inventory-20261007','coreSHA256':art['nodes'][node]['nano']['sha256'],'relayQualification':'restored-three-node-v32-preparation-priority-prepare-20261007','relayUnchanged':True,'relaySHA256':art['nodes'][node]['p2p']['sha256']}
save(proof/'binary-qualification-binding.json',bindings)
for name in ['copy-local.py','local-copy-original.log','vps-backup.py','vps-backup-original.log','bind-vps-audit.py','bind-vps-audit-original.log','prepare.py','prepare-original.log','finalize.py']:shutil.copy2(root/name,proof/name)
(proof/'README.md').write_text('''# V33 restoration preparation — 7 October 2026

V32 remains FAILED: no advancing common finality within the unchanged 900-second observation window. Its terminal evidence and post-stop audits are retained. This preparation uses those current stopped sources, not an earlier checkpoint.

New native/ARM64/AMD64 core df31cd48 uses a single immutable publication for block inventory capture, qualified by the held-writer counterexample and targeted concurrency/wire/clock/private HTTP checks. It does not prove the whole WAN failure cause. Relay77848f1f unchanged. Binary hashes are re-read on each host and bound to the recorded qualification. Existing images/build volumes reused; no rebuild here. Incremental artifacts do not qualify a clean reproducible release or full CTest/EEST.

Complete Mac copies1842/1832 files exclude only peer.key; exact hashes and independent file inodes, source unchanged, all journals/witnesses retained. VPS physical backup2871 files has exact hashes/independent inodes/source unchanged, kept privately on the VPS. Active16/16/33 and historical16/16/0 signer commitments verified separately against V32 stop receipts and earlier inventory; full key inventories32/32/33. VPS configuration independently derived on that host and its canonical digest equals the Mac planner; no private configuration/data exported.

Original blocks4096/catalogue128KiB/cold cache8/16MiB, clock, profile, consensus and controller deadlines retained. No retention experiment. Controller/planner/helper unchanged; no redundant model reruns. Preparation starts no signer. V33 must resume actual duties and demonstrate three new common finalized checkpoints within900 seconds after readiness, with clean terminal exits and durable finality evidence. It is not yet a passing network run or production qualification.

All production, EVM profile, Base/native backing/conversion/redemption, bridge, Doppler, operational and independent-review gates remain open. No funds, sale or public deployment.
''')
source=Path('/private/tmp/nano-restored-three-node-v32-preparation-priority-20261007');stop=read(source/'campaign/native-state.json');assert stop['exitCode']==0 and stop['signalCode'] is None
manifest={'sourceCore':art['coreRevision'],'sourceRelay':art['relayRevision'],'sourceCheckout':art['coreRevision'],'sourceCampaign':'cc9826f6e141977f32422e0e6798c026b7e47b7e','sourceMacContainer':'nano-restored-three-node-mac-muxae47a','sourceVpsContainer':'nano-restored-three-node-vps-muxae47a','sourceMacExpectedExit':0,'sourceVpsExpectedExit':0,'sourceNativeStop':stop,'localRunDirectory':'/private/tmp/nano-restored-three-node-v33-published-inventory-20261007','vpsCopiedVolume':'nano-restored-three-node-vps-20261005-data','vpsSourceBackup':read(proof/'vps-local-backup-receipt.json')['backupLocation'],'vpsPhysicalBackupVerified':True,'sourceMacSnapshotsPreserved':True,'signerJournalsNotReset':True,'sourceLimitsUnchanged':True,'signersStarted':False,'sourceBudgetsUnchanged':True,'fullJournalsAndWitnessesPreserved':True,'networkQualified':False,'productionQualified':False}
files={str(p.relative_to(proof)):sha(p) for p in sorted(proof.rglob('*')) if p.is_file()};assert 'manifest.json' not in files;manifest.update(files=files,artifacts=files);save(proof/'manifest.json',manifest)
dest=repo/'docs/qualification/restored-three-node-v33-published-inventory-prepare-20261007';shutil.copytree(proof,dest)
message='Préparation V33 : cœur df31cd48 (capture d’inventaire de blocs depuis une publication immuable), relais77848f1f conservé ; binaires natif/ARM64/AMD64 rehashés et liés aux qualifications ciblées. Captures V32 et engagements actifs16/16/33, historiques16/16/0 vérifiés ; copies Mac1842/1832 et sauvegarde physique VPS2871 fichiers exactes, journaux/témoins conservés. Configuration VPS dérivée localement, budgets et délais inchangés. [Preuves](qualification/restored-three-node-v33-published-inventory-prepare-20261007/README.md). Aucun nouveau signer lancé par la préparation ; duties et trois nouvelles finalités communes restent à démontrer. V32 FAILED conservé. Aucun critère de production supplémentaire clos par cette préparation.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;s=p.read_text();needle='## État courant vérifié — 7 octobre 2026\n\n';assert needle in s;p.write_text(s.replace(needle,needle+message+'\n\nÉtat précédent conservé : ',1))
print('V33 preparation archived, manifest contract complete; no signer started')
