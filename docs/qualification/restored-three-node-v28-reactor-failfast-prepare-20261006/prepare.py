from pathlib import Path
import json,hashlib,shutil,subprocess,shlex,sys
repo=Path('/Users/boris/Dev/blockchain-nano');old=Path('/private/tmp/nano-v27-stopped-view-20261006');prep=repo/'docs/qualification/restored-three-node-v28-reactor-failfast-prepare-20261006';root=Path('/private/tmp/nano-restored-three-node-v28-reactor-failfast-20261006');previous=Path('/private/tmp/nano-restored-three-node-v27-catalog-busy-retry-20261006');macold=old
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ssh(args):return subprocess.check_output(['/usr/bin/ssh','-4','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(args)],text=True)
resources=read(macold/'resources.json');resources.update(read(old/'resources.json'));artifacts=read(old/'artifacts.json');nstop=read(macold/'native-state.json');assert nstop['exitCode']==0 and nstop['signalCode'] is None
for line in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines():
 if '/nano run ' in line:raise AssertionError('Native writer still running')
for name,remote in [(resources['localContainer'],False),(resources['remoteContainer'],True)]:
 cmd=['docker','inspect',name,'--format','{{json .State}}'];s=json.loads(ssh(['sudo','-n']+cmd) if remote else subprocess.check_output(cmd,text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
assert not ssh(['sudo','-n','docker','ps','-q','--filter','volume='+resources['vpsVolume']]).strip()
# Compare the exact stopped files with independently retained audit digests.
for name in ['mac-native','mac-docker']:
 source=previous/name/'data'
 expected=read(old/'post-stop-native'/(name+'-before.json'))
 actual={str(f.relative_to(source)):sha(f) for f in sorted(source.rglob('*')) if f.is_file()}
 assert actual==expected,'Stopped Mac data changed: '+name
remote_manifest={line.split('  ',1)[1].removeprefix('/var/lib/nano/'):line.split('  ',1)[0] for line in (old/'post-stop-vps/data-before.sha256').read_text().splitlines()}
volume_path='/var/lib/docker/volumes/'+resources['vpsVolume']+'/_data'
code="from pathlib import Path;import hashlib,json;p=Path("+repr(volume_path)+");print(json.dumps({str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()}))"
assert json.loads(ssh(['sudo','-n','python3','-c',code]))==remote_manifest,'Stopped VPS data changed'
input=read(repo/'docs/qualification/restored-three-node-v27-catalog-busy-retry-prepare-20261006/input.json')
input['macNativeCapsule']=read(macold/'post-stop-native/mac-native-capture.json');input['macCapsule']=read(macold/'post-stop-native/mac-docker-capture.json');input['vpsCapsule']=read(old/'post-stop-vps/capture.json');input['finalized']=read(old/'result.json')['baseline'];input['writersStopped']=True
input.pop('retentionExperiment',None)
# Current owned pins match independent stop receipts; old, unowned journals
# must match the previous prepared capsule. Preserve all 32 Mac journals.
prior=read(repo/'docs/qualification/restored-three-node-v27-catalog-busy-retry-prepare-20261006/input.json')
def key(p):return ('index',p['index']) if p['index'] is not None else ('key',p['publicKey'])
pin_checks={}
for n,(name,field) in enumerate([('mac-native','macNativeCapsule'),('mac-docker','macCapsule'),('vps-docker','vpsCapsule')]):
 current={key(p):p['restart'] for p in input[field]['validatorKeys']}
 owned={key(p):p['restart'] for p in read(old/(str(n)+'-external-stop-receipt.json'))['validatorKeys']}
 assert all(current[k]==v for k,v in owned.items())
 historic={key(p):p['restart'] for p in prior[field]['validatorKeys'] if key(p) not in owned}
 assert all(current[k]==v for k,v in historic.items())
 assert set(current)==set(owned)|set(historic)
 pin_checks[name]={'currentOwnedPinsVerified':len(owned),'historicalPinsUnchanged':len(historic),'allCapturedPinsVerified':len(current)}

source_plan=read(repo/'docs/qualification/restored-three-node-v27-catalog-busy-retry-prepare-20261006/plan.json')
for key in ['macNativeConfig','macConfig','vpsConfig']:
 input[key]['capella']['limits']=source_plan['configs'][0]['capella']['limits']
assert input['macConfig']['capella']['limits']['blocks']=='0x400'
# All config paths remain the controlled mount paths; only independently audited pins are replaced by the planner.
prep.mkdir(exist_ok=False);root.mkdir(exist_ok=False);save(prep/'input.json',input)
assert not subprocess.check_output(['git','diff','HEAD','--','src','include','tests/test_capella_transition.cpp','CMakeLists.txt'],cwd=repo),'Candidate core not committed'
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
assert revision==sys.argv[1]
compiled_inputs=read(Path('/private/tmp/nano-catalog-reactor-failfast-20261006/source-worktree-match.json'))['matched']
assert len(compiled_inputs)==1602 and all(sha(repo/k)==v for k,v in compiled_inputs.items()), 'Current sources differ from qualified frozen core'
artifacts['coreRevision']=revision;artifacts['relayRevision']=revision;artifacts['relaySourceAlignmentRevision']=revision
artifacts['coreBuildRevision']='02ec0ab0b3841a09ffa5749cc9db54faa61a2b20'
artifacts['relayBuildRevision']='2f93dda09ce9765e5240d9ddcb3d74b82144138f';artifacts['relayReuseProof']='relay-reuse/reuse-boundary.json'
artifacts['relayCompilerInputsUnchangedFromBuildRevision']=True
artifacts['relayReuseScope']='35 project compiler inputs per architecture; explicit busy handling and retry headers included; qualified snapshot identical to committed runtime inputs'
artifacts['nodes']['mac-native']['nano']={'path':'/private/tmp/nano-catalog-reactor-failfast-20261006/native/nano','sha256':sha(Path('/private/tmp/nano-catalog-reactor-failfast-20261006/native/nano'))}
artifacts['nodes']['mac-native']['p2p']={'path':'/private/tmp/nano-catalog-busy-integrated-native-20261006/network/nano-p2p','sha256':'0c06ce4f220f385f7e460c1f932b5f34ca61d88ec2d37070f2d0475b579eb686'}
assert sha(Path(artifacts['nodes']['mac-native']['p2p']['path']))==artifacts['nodes']['mac-native']['p2p']['sha256']
for arch,name,digest in [('arm64','mac-docker','198a29327f61c3542d7f3968a56fdb473c3f031f59b5d81231ee9a1e3cbad5dc'),('amd64','vps-docker','99e4252ec36b8212e74e908d7e91ff7cca539d3c5073209ccc7814aa753b738b')]:
 artifacts['nodes'][name]['p2p']={'path':'/build/catalog-busy-integrated-'+arch+'-20261006/network/nano-p2p','sha256':digest}
 runtime_hashes=read(Path('/private/tmp/nano-catalog-reactor-failfast-20261006/integrated-summary.json'))['runtimeBinaryHashes'][arch]
 artifacts['nodes'][name]['nano']={'path':'/build/catalog-reactor-failfast-'+arch+'-20261006/nano','sha256':runtime_hashes['nano']}
shutil.copytree('/private/tmp/nano-v28-relay-reuse-20261006',prep/'relay-reuse')
save(prep/'signer-pin-verification.json',pin_checks)
save(prep/'artifacts.json',artifacts)
script="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';const p=restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8')));await writeFile(process.argv[2],JSON.stringify(p,null,2)+'\\n');"
subprocess.run(['node','--input-type=module','-e',script,str(prep/'input.json'),str(prep/'plan.json')],cwd=repo,check=True)
counts={}
for name in ['mac-native','mac-docker']:
 source=previous/name/'data';dest=root/name/'data';dest.parent.mkdir();subprocess.run(['/bin/cp','-cR',str(source),str(dest)],check=True);(dest/'peer.key').unlink(missing_ok=True);before={str(f.relative_to(source)):sha(f) for f in sorted(source.rglob('*')) if f.is_file() and f.name!='peer.key'};after={str(f.relative_to(dest)):sha(f) for f in sorted(dest.rglob('*')) if f.is_file()};assert before==after;save(dest.parent/'data-copy-hashes.json',after);counts[name]=len(after)
manifest={'sourceCheckout':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'sourceMacContainer':resources['localContainer'],'sourceMacExpectedExit':0,'sourceVpsContainer':resources['remoteContainer'],'sourceVpsExpectedExit':0,'sourceNativeStop':nstop,'sourceMacNativeData':str(previous/'mac-native/data'),'sourceMacDockerData':str(previous/'mac-docker/data'),'localRunDirectory':str(root),'vpsCopiedVolume':resources['vpsVolume'],'vpsVolumeReusedAfterReadonlyAudit':True,'independentMacSourcesPreserved':True,'localFilesCopiedWithoutPeerIdentity':counts,'copyMethod':'APFS copy-on-write clone; independent destination; excludes peer identity','signerJournalsNotReset':True,'networkQualified':False,'productionQualified':False,'artifacts':{str(f.relative_to(prep)):sha(f) for f in sorted(prep.rglob('*')) if f.is_file()}}
save(prep/'manifest.json',manifest)
(prep/'README.md').write_text("""# Préparation V28 : acquisition catalogue non bloquante

États post-V27 arrêtés et audités. Fichiers revérifiés avant copies APFS Mac sans peer.key ; volume VPS gardé sur son hôte. Engagements possédés 16/16/33 et historiques 16/16 comparés séparément. Aucun journal réinitialisé, aucune base/configuration/requête/clés transférée entre hôtes.

Cœur 02ec0ab0 : huit modes concernés passent sur natif, Docker ARM64 et Docker AMD64. Les erreurs de driver initiales sont conservées. Relais 2f93dda0 réutilisé avec 35 entrées compilateur projet inchangées par architecture. Les exécutables core corrigés et leurs hashes figurent dans artifacts.json. Aucune image reconstruite.

Mêmes limites 1024 blocs et quorum. Trois nouvelles finalités communes distinctes requises en 900 secondes, baseline epoch 1284. Préparation seule, aucun signataire lancé. V27 FAILED et contre-exemple EE5 conservés ; cause globale WAN, tous critères non démontrés et revue indépendante ouverts.
""")
paragraph='\n\n2026-10-06 — Préparation V28 : états post-V27 audités et revérifiés, pins possédés 16/16/33 et historiques conservés séparément, copies Mac sans peer.key et volume VPS réutilisé sur son hôte. Cœur 02ec0ab0 qualifié dans huit modes sur trois architectures ; relais 2f93dda0 réutilisé avec 35 dépendances projet inchangées par architecture. Mêmes quotas et trois nouvelles finalités communes en 900 s à démontrer. Aucun signataire lancé ni image reconstruite. V27 FAILED, cause globale et critères non démontrés/revue indépendante ouverts. Preuves restored-three-node-v28-reactor-failfast-prepare-20261006/.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+paragraph)
print(json.dumps({'prepared':str(root),'macCopyFiles':counts,'signersStarted':False,'vpsDatabaseExported':False}))
