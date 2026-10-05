from pathlib import Path
import json,hashlib,shutil,subprocess,shlex
repo=Path('/Users/boris/Dev/blockchain-nano');old=repo/'docs/qualification/restored-three-node-v7-terminal-20261006';prep=repo/'docs/qualification/restored-three-node-v8-retention-prepare-20261006';root=Path('/private/tmp/nano-restored-three-node-v8-retention-20261006');previous=Path('/private/tmp/nano-restored-three-node-v7-20261006')
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ssh(args):return subprocess.check_output(['ssh','-o','BatchMode=yes','ubuntu@145.239.199.166',shlex.join(args)],text=True)
resources=read(old/'resources.json');artifacts=read(old/'artifacts.json');nstop=read(old/'native-state.json');assert nstop['exitCode']==0 and nstop['signalCode'] is None
for line in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines():
 if '/nano run ' in line:raise AssertionError('Native writer still running')
for name,remote in [(resources['localContainer'],False),(resources['remoteContainer'],True)]:
 cmd=['docker','inspect',name,'--format','{{json .State}}'];s=json.loads(ssh(['sudo','-n']+cmd) if remote else subprocess.check_output(cmd,text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
assert not ssh(['sudo','-n','docker','ps','-q','--filter','volume='+resources['vpsVolume']]).strip()
input=read(repo/'docs/qualification/restored-three-node-v7-prepare-20261006/input.json')
input['macNativeCapsule']=read(old/'post-stop-mac/mac-native-capture.json');input['macCapsule']=read(old/'post-stop-mac/mac-docker-capture.json');input['vpsCapsule']=read(old/'post-stop-vps/capture.json');input['finalized']=read(old/'result.json')['baseline'];input['writersStopped']=True
input['retentionExperiment']={'blocks':'0x400','rationale':'Private bounded 900-second experiment resumes exact post-v7 pins with retained-block capacity 1024; envelope and SSZ byte limits unchanged. Does not qualify recovery at saturation or production. Resource peaks must be observed.'}
# All config paths remain the controlled mount paths; only independently audited pins are replaced by the planner.
prep.mkdir(exist_ok=False);root.mkdir(exist_ok=False);save(prep/'input.json',input)
artifacts['coreRevision']='05649987fd814c711f3960c70658cf6ad15d42ae'
programs=read(Path('/private/tmp/nano-atomic-proposal-native-20261006/programs.json'))
artifacts['nodes']['mac-native']['nano']={'path':'/private/tmp/nano-atomic-proposal-native-20261006/nano','sha256':programs['nano']}
for arch,name in [('arm64','mac-docker'),('amd64','vps-docker')]:
 folder='/build/atomic-runtime-'+arch+'-d71f574a-20261006'
 lines=(repo/'docs/qualification/capella-atomic-proposal-admission-20261006'/arch/('atomic-runtime-'+arch+'-d71f574a-20261006')/'programs.sha256').read_text().splitlines()
 digest=next(l.split()[0] for l in lines if l.endswith('  '+folder+'/nano'))
 artifacts['nodes'][name]['nano']={'path':folder+'/nano','sha256':digest}
artifacts['relayRevision']='5902c9838910c7fcc618ee230c9b6727cb444313'
artifacts['nodes']['mac-native']['p2p']={'path':'/private/tmp/nano-export-resume-p2p-native-20261006/candidate/test','sha256':'5e1a93411e0918f4933dd0222503bba374dc1fbf9e459da23a68350cee95766f'}
for arch,name,digest in [('arm64','mac-docker','7ced2ccf8e6cc03720645effe79f46463c916d7ea0aa9e96b3be7a43da093213'),('amd64','vps-docker','0af33cfa27d0b233abfd4d41cb4e1d5a1029aadf7a61f56d1cad854c838e04cb')]:
 artifacts['nodes'][name]['p2p']={'path':'/build/export-resume-p2p-'+arch+'-9ebc4cec-20261006/nano-p2p','sha256':digest}
save(prep/'artifacts.json',artifacts)
script="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';const p=restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8')));await writeFile(process.argv[2],JSON.stringify(p,null,2)+'\\n');"
subprocess.run(['node','--input-type=module','-e',script,str(prep/'input.json'),str(prep/'plan.json')],cwd=repo,check=True)
counts={}
for name in ['mac-native','mac-docker']:
 source=previous/name/'data';dest=root/name/'data';dest.parent.mkdir();shutil.copytree(source,dest,ignore=shutil.ignore_patterns('peer.key'));before={str(f.relative_to(source)):sha(f) for f in sorted(source.rglob('*')) if f.is_file() and f.name!='peer.key'};after={str(f.relative_to(dest)):sha(f) for f in sorted(dest.rglob('*')) if f.is_file()};assert before==after;save(dest.parent/'data-copy-hashes.json',after);counts[name]=len(after)
manifest={'sourceCheckout':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'sourceMacContainer':resources['localContainer'],'sourceMacExpectedExit':0,'sourceVpsContainer':resources['remoteContainer'],'sourceVpsExpectedExit':0,'sourceNativeStop':nstop,'sourceMacNativeData':str(previous/'mac-native/data'),'sourceMacDockerData':str(previous/'mac-docker/data'),'localRunDirectory':str(root),'vpsCopiedVolume':resources['vpsVolume'],'vpsVolumeReusedAfterReadonlyAudit':True,'independentMacSourcesPreserved':True,'localFilesCopiedWithoutPeerIdentity':counts,'signerJournalsNotReset':True,'networkQualified':False,'productionQualified':False,'artifacts':{f.name:sha(f) for f in prep.iterdir() if f.is_file()}}
save(prep/'manifest.json',manifest)
(prep/'README.md').write_text("""# Restored v8: explicit bounded retention experiment

Post-v7 independent read-only capsules and observed stopped source writers retain exact primary, witness and own signer commitments. Mac databases copied locally to separate owner paths without peer.key; VPS stopped volume stays on VPS. No journals reset, merged or transferred.

Core 05649987 includes capacity refusal before signing and atomic scheduled proposal admission. Relay 5902c983 includes bounded preparation caching for resumed ancestry exports. Qualified linked programs reused on native Mac, ARM64 Docker and AMD64 Docker; no image build.

Local retained-block capacity explicitly changes 256 to 1024 on all three nodes after source verification. Profile, finality baseline, pins, envelope (128 MiB), total SSZ (64 MiB), individual state, network budgets and 900-second deadline unchanged. This does NOT fix full-store pressure recovery, bound resident memory, qualify pruning or close production. 1024 is a private fixture/capture resource ceiling, not a validator limit or consensus restriction. Resource observations required before operational claims.

Three distinct advancing common checkpoints plus stable observations and real owner proposal/attestation duties remain required. Old failed v7 remains authoritative and preserved. Preparation alone has not launched or qualified a network.
""")
paragraph='\n\n2026-10-06, préparation v8 de rétention explicitement expérimentale : sources post-v7 auditées, engagements exacts, journaux séparés sans remise à zéro. Candidate cœur 05649987 / relais 5902c983 liée sur les trois environnements, images réutilisées. Capacité locale 256→1024 blocs, autres limites en octets et règles inchangées ; ce changement ne clôture ni saturation, ni pruning, ni mémoire, ni production. Échéance originale 900 s et trois avances communes distinctes nécessaires. Préparation seule sans reprise qualifiée. Preuves restored-three-node-v8-retention-prepare-20261006/.\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+paragraph)
print(json.dumps({'prepared':str(root),'macCopyFiles':counts,'signersStarted':False,'vpsDatabaseExported':False}))
