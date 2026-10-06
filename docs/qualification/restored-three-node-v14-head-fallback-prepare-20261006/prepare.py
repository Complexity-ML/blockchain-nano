from pathlib import Path
import json,hashlib,shutil,subprocess,shlex
repo=Path('/Users/boris/Dev/blockchain-nano');old=repo/'docs/qualification/restored-three-node-v13-parent-cache-terminal-20261006';prep=repo/'docs/qualification/restored-three-node-v14-head-fallback-prepare-20261006';root=Path('/private/tmp/nano-restored-three-node-v14-head-fallback-20261006');previous=Path('/private/tmp/nano-restored-three-node-v13-parent-cache-20261006')
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
input=read(repo/'docs/qualification/restored-three-node-v13-parent-cache-prepare-20261006/input.json')
input['macNativeCapsule']=read(old/'post-stop-mac/mac-native-capture.json');input['macCapsule']=read(old/'post-stop-mac/mac-docker-capture.json');input['vpsCapsule']=read(old/'post-stop-vps/capture.json');input['finalized']=read(old/'result.json')['baseline'];input['writersStopped']=True
input.pop('retentionExperiment',None)
source_plan=read(repo/'docs/qualification/restored-three-node-v13-parent-cache-prepare-20261006/plan.json')
for key in ['macNativeConfig','macConfig','vpsConfig']:
 input[key]['capella']['limits']=source_plan['configs'][0]['capella']['limits']
assert input['macConfig']['capella']['limits']['blocks']=='0x400'
# All config paths remain the controlled mount paths; only independently audited pins are replaced by the planner.
prep.mkdir(exist_ok=False);root.mkdir(exist_ok=False);save(prep/'input.json',input)
assert not subprocess.check_output(['git','diff','HEAD','--','src','include','tests/test_capella_transition.cpp','CMakeLists.txt'],cwd=repo),'Candidate core not committed'
artifacts['coreRevision']='5787915e4c62ce3c84dc46e1a9829257cdf628a1'
artifacts['nodes']['mac-native']['nano']={'path':'/private/tmp/nano-parent-cache-native-20261006/nano','sha256':sha(Path('/private/tmp/nano-parent-cache-native-20261006/nano'))}
for arch,name in [('arm64','mac-docker'),('amd64','vps-docker')]:
 folder='/build/parent-cache-'+arch+'-20261006'
 lines=(repo/'docs/qualification/parent-slot-cache-integrated-20261006'/arch/Path(folder).name/'runtime-programs.sha256').read_text().splitlines()
 digest=next(l.split()[0] for l in lines if l.endswith('  '+folder+'/nano'))
 artifacts['nodes'][name]['nano']={'path':folder+'/nano','sha256':digest}
artifacts['relayRevision']='7248698dd92fa7fbb17b878034b460fe3f8af584'
artifacts['nodes']['mac-native']['p2p']={'path':'/private/tmp/nano-head-fallback-20261006/native/runtime','sha256':sha(Path('/private/tmp/nano-head-fallback-20261006/native/runtime'))}
for arch,name in [('arm64','mac-docker'),('amd64','vps-docker')]:
 folder='/build/head-fallback-p2p-'+arch+'-20261006'
 digest=(repo/'docs/qualification/head-catalog-fallback-integrated-20261006'/(arch+'-runtime')/'program.sha256').read_text().split()[0]
 artifacts['nodes'][name]['p2p']={'path':folder+'/nano-p2p','sha256':digest}
save(prep/'artifacts.json',artifacts)
script="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';const p=restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8')));await writeFile(process.argv[2],JSON.stringify(p,null,2)+'\\n');"
subprocess.run(['node','--input-type=module','-e',script,str(prep/'input.json'),str(prep/'plan.json')],cwd=repo,check=True)
counts={}
for name in ['mac-native','mac-docker']:
 source=previous/name/'data';dest=root/name/'data';dest.parent.mkdir();shutil.copytree(source,dest,ignore=shutil.ignore_patterns('peer.key'));before={str(f.relative_to(source)):sha(f) for f in sorted(source.rglob('*')) if f.is_file() and f.name!='peer.key'};after={str(f.relative_to(dest)):sha(f) for f in sorted(dest.rglob('*')) if f.is_file()};assert before==after;save(dest.parent/'data-copy-hashes.json',after);counts[name]=len(after)
manifest={'sourceCheckout':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'sourceMacContainer':resources['localContainer'],'sourceMacExpectedExit':0,'sourceVpsContainer':resources['remoteContainer'],'sourceVpsExpectedExit':0,'sourceNativeStop':nstop,'sourceMacNativeData':str(previous/'mac-native/data'),'sourceMacDockerData':str(previous/'mac-docker/data'),'localRunDirectory':str(root),'vpsCopiedVolume':resources['vpsVolume'],'vpsVolumeReusedAfterReadonlyAudit':True,'independentMacSourcesPreserved':True,'localFilesCopiedWithoutPeerIdentity':counts,'signerJournalsNotReset':True,'networkQualified':False,'productionQualified':False,'artifacts':{f.name:sha(f) for f in prep.iterdir() if f.is_file()}}
save(prep/'manifest.json',manifest)
(prep/'README.md').write_text("""# Reprise v14 : historique après refus de tête

Engagements post-v13 audités séparément et journaux 16/16/33 conservés. Copies locales Mac sans peer.key ; volume VPS maintenu sur son hôte. Aucun transfert inter-hôtes de base ou clé privée. Les empreintes de chaque stockage arrêté doivent correspondre aux preuves v13 avant copie.

Correction du relais : une tête non confirmée ou un bloc refusé ne bloque plus le parcours borné des éléments historiques indépendants. Aucun ACK fictif ; délais et budgets conservés. Replay réel comparatif : deux frontières admises en 26 cycles avec la correction, contre une seule en 128 cycles avant correction. Exécutables qualifiés sur les trois architectures. Cela ne prouve pas la finalité réseau ; pression/éviction/coût des caches et causes WAN restent ouverts.

Profil, limite 1024 blocs, finalité initiale 1284 et critère de trois avancées distinctes en 900 secondes inchangés. Image et volumes de build réutilisés. SSH IPv4 et vérification habituelle de la clé hôte. Préparation seule : aucun fonds réel, vente ou déploiement public. Production et revue indépendante ouvertes.
""")
paragraph='\n\n2026-10-06 — Préparation v14 : engagements post-v13 exacts, cœur 5787915e et relais 7248698d avec parcours historique après refus de tête. Copies locales séparées et volume VPS conservé ; journaux non réinitialisés/fusionnés. Même profil, même limite et même critère de finalité en 900 s. Préparation seule, pression/éviction/coût des caches, cause globale WAN, finalité et production ouverts. Preuves restored-three-node-v14-head-fallback-prepare-20261006/.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+paragraph)
print(json.dumps({'prepared':str(root),'macCopyFiles':counts,'signersStarted':False,'vpsDatabaseExported':False}))
