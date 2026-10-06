from pathlib import Path
import json,hashlib,shutil,subprocess,shlex
repo=Path('/Users/boris/Dev/blockchain-nano');old=repo/'docs/qualification/restored-three-node-v11-target-terminal-20261006';prep=repo/'docs/qualification/restored-three-node-v12-vote-cache-prepare-20261006';root=Path('/private/tmp/nano-restored-three-node-v12-vote-cache-20261006');previous=Path('/private/tmp/nano-restored-three-node-v11-target-20261006')
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ssh(args):return subprocess.check_output(['/usr/bin/ssh','-4','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(args)],text=True)
resources=read(old/'resources.json');artifacts=read(old/'artifacts.json');nstop=read(old/'native-state.json');assert nstop['exitCode']==0 and nstop['signalCode'] is None
for line in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines():
 if '/nano run ' in line:raise AssertionError('Native writer still running')
for name,remote in [(resources['localContainer'],False),(resources['remoteContainer'],True)]:
 cmd=['docker','inspect',name,'--format','{{json .State}}'];s=json.loads(ssh(['sudo','-n']+cmd) if remote else subprocess.check_output(cmd,text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
assert not ssh(['sudo','-n','docker','ps','-q','--filter','volume='+resources['vpsVolume']]).strip()
# Compare the exact stopped files with independently retained audit digests.
for name in ['mac-native','mac-docker']:
 source=previous/name/'data'
 expected=read(old/('post-stop-mac/'+name+'-before.json'))
 actual={str(f.relative_to(source)):sha(f) for f in sorted(source.rglob('*')) if f.is_file()}
 assert actual==expected,'Stopped Mac data changed: '+name
remote_manifest={line.split('  ',1)[1].removeprefix('/var/lib/nano/'):line.split('  ',1)[0] for line in (old/'post-stop-vps/data-before.sha256').read_text().splitlines()}
volume_path='/var/lib/docker/volumes/'+resources['vpsVolume']+'/_data'
code="from pathlib import Path;import hashlib,json;p=Path("+repr(volume_path)+");print(json.dumps({str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}))"
assert json.loads(ssh(['sudo','-n','python3','-c',code]))==remote_manifest,'Stopped VPS data changed'
input=read(repo/'docs/qualification/restored-three-node-v11-target-prepare-20261006/input.json')
input['macNativeCapsule']=read(old/'post-stop-mac/mac-native-capture.json');input['macCapsule']=read(old/'post-stop-mac/mac-docker-capture.json');input['vpsCapsule']=read(old/'post-stop-vps/capture.json');input['finalized']=read(old/'result.json')['baseline'];input['writersStopped']=True
input.pop('retentionExperiment',None)
source_plan=read(repo/'docs/qualification/restored-three-node-v11-target-prepare-20261006/plan.json')
for key in ['macNativeConfig','macConfig','vpsConfig']:
 input[key]['capella']['limits']=source_plan['configs'][0]['capella']['limits']
assert input['macConfig']['capella']['limits']['blocks']=='0x400'
# All config paths remain the controlled mount paths; only independently audited pins are replaced by the planner.
prep.mkdir(exist_ok=False);root.mkdir(exist_ok=False);save(prep/'input.json',input)
artifacts['coreRevision']='f077205fabb5ddaba550896d99e0c1130354813b'
artifacts['nodes']['mac-native']['nano']={'path':'/private/tmp/nano-vote-cache-integrated-native-20261006/nano','sha256':sha(Path('/private/tmp/nano-vote-cache-integrated-native-20261006/nano'))}
for arch,name in [('arm64','mac-docker'),('amd64','vps-docker')]:
 folder='/build/vote-target-runtime-'+arch+'-20261006'
 lines=(repo/'docs/qualification/vote-target-cache-integrated-20261006'/arch/Path(folder).name/'programs.sha256').read_text().splitlines()
 digest=next(l.split()[0] for l in lines if l.endswith('  '+folder+'/nano'))
 artifacts['nodes'][name]['nano']={'path':folder+'/nano','sha256':digest}
artifacts['relayRevision']='76bb246857dee768e916c5fc0456079d490c4581'
artifacts['nodes']['mac-native']['p2p']={'path':'/private/tmp/nano-dispatch-recovery-p2p-native-20261006/test','sha256':sha(Path('/private/tmp/nano-dispatch-recovery-p2p-native-20261006/test'))}
for arch,name in [('arm64','mac-docker'),('amd64','vps-docker')]:
 folder='/build/dispatch-recovery-p2p-'+arch+'-1f521b7b-20261006'
 text=(repo/'docs/qualification/dispatch-transport-recovery-integrated-20261006'/arch/Path(folder).name/'program.sha256').read_text()
 artifacts['nodes'][name]['p2p']={'path':folder+'/nano-p2p','sha256':text.split()[0]}
save(prep/'artifacts.json',artifacts)
script="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';const p=restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8')));await writeFile(process.argv[2],JSON.stringify(p,null,2)+'\\n');"
subprocess.run(['node','--input-type=module','-e',script,str(prep/'input.json'),str(prep/'plan.json')],cwd=repo,check=True)
counts={}
for name in ['mac-native','mac-docker']:
 source=previous/name/'data';dest=root/name/'data';dest.parent.mkdir();shutil.copytree(source,dest,ignore=shutil.ignore_patterns('peer.key'));before={str(f.relative_to(source)):sha(f) for f in sorted(source.rglob('*')) if f.is_file() and f.name!='peer.key'};after={str(f.relative_to(dest)):sha(f) for f in sorted(dest.rglob('*')) if f.is_file()};assert before==after;save(dest.parent/'data-copy-hashes.json',after);counts[name]=len(after)
manifest={'sourceCheckout':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'sourceMacContainer':resources['localContainer'],'sourceMacExpectedExit':0,'sourceVpsContainer':resources['remoteContainer'],'sourceVpsExpectedExit':0,'sourceNativeStop':nstop,'sourceMacNativeData':str(previous/'mac-native/data'),'sourceMacDockerData':str(previous/'mac-docker/data'),'localRunDirectory':str(root),'vpsCopiedVolume':resources['vpsVolume'],'vpsVolumeReusedAfterReadonlyAudit':True,'independentMacSourcesPreserved':True,'localFilesCopiedWithoutPeerIdentity':counts,'signerJournalsNotReset':True,'networkQualified':False,'productionQualified':False,'artifacts':{f.name:sha(f) for f in prep.iterdir() if f.is_file()}}
save(prep/'manifest.json',manifest)
(prep/'README.md').write_text("""# Reprise v12 : cache des cibles de vote

Sources post-v11 auditées séparément, engagements et journaux 16/16/33 conservés sans réinitialisation ni fusion. Trois arrêts originaux propres/sans OOM. Copies Mac locales seulement, sans peer.key ; volume VPS conservé sur son hôte. Core f077205f qualifié sur Mac natif et Docker ARM64/AMD64, relais 76bb2468 inchangé. Empreintes des fichiers arrêtés vérifiées contre les audits avant copie.

Limites 1024 blocs, profil, finalité de départ 1284 et critère de trois avancées distinctes en 900 secondes inchangés. SSH IPv4 avec vérification habituelle de clé hôte. Aucun transfert inter-hôtes de base ou clé privée, aucune image reconstruite.

Les continuations de cibles de vote restent disponibles lorsque des racines concurrentes alternent. Le contre-exemple et les régressions passent sur les trois architectures ; cela ne prouve pas la cause v11 ni la finalité réseau. Préparation seule, production ouverte. Parents distincts, coût/pression des caches, publications complètes et pruning restent ouverts. Aucun fonds réel, vente ou déploiement public.
""")
paragraph='\n\n2026-10-06 — Préparation v12 : engagements post-v11 exacts, core f077205f et relais 76bb2468. Empreintes propriétaires inchangées, copies Mac locales et volume VPS conservés ; journaux non réinitialisés/fusionnés. Cache des cibles de vote qualifié sur les trois environnements ; profil/limites et critère de trois avancées en 900 s inchangés. Préparation seule ; cause v11, finalité et production ouvertes. Preuves restored-three-node-v12-vote-cache-prepare-20261006/.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+paragraph)
print(json.dumps({'prepared':str(root),'macCopyFiles':counts,'signersStarted':False,'vpsDatabaseExported':False}))
