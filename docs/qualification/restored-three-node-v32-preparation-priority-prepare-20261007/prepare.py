from pathlib import Path
import json,hashlib,subprocess,shlex,copy
repo=Path('/Users/boris/Dev/blockchain-nano');out=Path(__file__).parent;proof=out/'proof';proof.mkdir();source=Path('/private/tmp/nano-restored-three-node-v31-canonical-memo-20261007');root=Path('/private/tmp/nano-restored-three-node-v32-preparation-priority-20261007');audits=Path('/private/tmp/nano-v31-post-stop-readonly-20261007')
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ssh(args):return subprocess.check_output(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166',shlex.join(args)],text=True)
def docker(args,remote=False):return ssh(['sudo','-n','docker']+args) if remote else subprocess.check_output(['/Users/boris/.docker/bin/docker']+args,text=True)
for name,remote in [('nano-restored-three-node-mac-mux94cau',False),('nano-restored-three-node-vps-mux94cau',True)]:
 s=json.loads(docker(['inspect',name,'--format','{{json .State}}'],remote));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
assert not docker(['ps','-q','--filter','volume=nano-restored-three-node-vps-20261005-data'],True).strip()
assert not any('/nano run ' in l for l in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())
x=read(repo/'docs/qualification/restored-three-node-v31-canonical-prepare-20261007/input.json');old=copy.deepcopy(x);oldplan=read(source/'campaign/plan.json');checks={}
def identity(k):return ('index',k['index']) if k['index'] is not None else ('key',k['publicKey'])
for i,(name,field,cfgfield,label) in enumerate([('mac-native','macNativeCapsule','macNativeConfig','native'),('mac-docker','macCapsule','macConfig','mac-docker'),('vps-docker','vpsCapsule','vpsConfig','vps')]):
 x[field]=read(repo/'docs/qualification/v31-post-stop-readonly-20261007'/label/'capture.json');receipt=read(source/f'campaign/{i}-external-stop-receipt.json');assert x[field]['restart']==receipt['restart']
 actual=oldplan['configs'][i]['capella']
 for f in ['limits','catalogs','clock','coldCheckpoint']:x[cfgfield]['capella'][f]=copy.deepcopy(actual[f])
 x[cfgfield]['capella']['network']['enabled']=True;assert x[cfgfield]['capella']['limits']['blocks']=='0x1000'
 current={identity(k):k['restart'] for k in x[field]['validatorKeys']};owned={identity(k):k['restart'] for k in receipt['validatorKeys']};historical={identity(k):k['restart'] for k in old[field]['validatorKeys'] if identity(k) not in owned};assert set(current)==set(owned)|set(historical);assert all(current[k]==v for k,v in (owned|historical).items());checks[name]={'ownedPinsVerified':len(owned),'historicalPinsVerified':len(historical),'fullInventory':len(current)}
 for cfg in [x[cfgfield]]:assert cfg['capella']['catalogs']['rootBytes']=='0x20000'
x.pop('retentionExperiment',None);x['finalized']=x['macCapsule']['canonicalReplay']['finalized'];x['writersStopped']=True;save(proof/'input.json',x)
script="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';await writeFile(process.argv[2],JSON.stringify(restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8'))),null,2)+'\\n');"
subprocess.run(['node','--input-type=module','-e',script,str(proof/'input.json'),str(proof/'plan.json')],cwd=repo,check=True)
plan=read(proof/'plan.json');vps=copy.deepcopy(plan['configs'][2]);binary='/build/catchup-priority-amd64-20261007/network/nano-p2p';vps.update({'p2p_binary':binary,'p2p_identity':'/var/lib/nano/peer.key','bootnodes':[],'rpc_host':'127.0.0.1','rpc_port':8545,'listen_addr':'/ip4/127.0.0.1/tcp/30303'});vps['capella']['profile']['file']='/config/profile.json';vps['capella']['network']['enabled']=True
expected=hashlib.sha256(json.dumps(vps,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()).hexdigest();options={'coldCheckpoint':x['coldCheckpoint']};helper=(repo/'tests/capella/vps-local-launch-prepare.py').read_text().split("if __name__=='__main__':")[0]
remote=helper+"\naudit=Path('/tmp/nano-restored-three-node-vps-mux94cau/local-readonly-audit');c=derive(json.loads((audit/'config.json').read_text()),json.loads((audit/'capture.json').read_text()),8545,30303,sys.argv[1],json.loads(sys.argv[2]));assert canonical_hash(c)==sys.argv[3];print(json.dumps({'independentConfigDigestVerified':True,'localOnly':True}))"
assert json.loads(ssh(['python3','-c',remote,binary,json.dumps(options),expected]))['independentConfigDigestVerified'];save(proof/'vps-local-plan-check.json',{'independentConfigDigestVerified':True,'expectedCanonicalSHA256':expected,'noConfigurationExport':True,'signersStarted':False});save(proof/'signer-pin-verification.json',checks)
art={'coreRevision':'7329b7ad47c373bc93f6568859a521be3c4f5755','relayRevision':'77848f1fa52fa2c44ae622e0809a74501616e28a','nodes':{},'networkQualified':False,'productionQualified':False}
for field in ['src','include','third_party']:subprocess.run(['git','diff','--quiet',art['coreRevision'],'--',field],cwd=repo,check=True)
subprocess.run(['git','diff','--quiet',art['relayRevision'],'--','network'],cwd=repo,check=True)
np=Path('/private/tmp/nano-durable-canonical-memo-20261007/native-build/nano');pp=Path('/private/tmp/nano-catchup-priority-build-20261007/native/network/nano-p2p');art['nodes']['mac-native']={'nano':{'path':str(np),'sha256':sha(np)},'p2p':{'path':str(pp),'sha256':sha(pp)}}
for arch,name,image,volume in [('arm64','mac-docker','e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','nano-clean-arm64-build-20261002'),('amd64','vps-docker','fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','nano-clean-rpc-build-20261002')]:
 paths=['/build/durable-canonical-memo-'+arch+'-20261007/nano','/build/catchup-priority-'+arch+'-20261007/network/nano-p2p'];lines=docker(['run','--rm','--network','none','--read-only','-v',volume+':/build:ro','--entrypoint','sha256sum',image]+paths,arch=='amd64').splitlines();hashes={l.split('  ',1)[1]:l.split('  ',1)[0] for l in lines};art['nodes'][name]={'image':'sha256:'+image,'buildVolume':volume,**{k:{'path':p,'sha256':hashes[p]} for k,p in zip(['nano','p2p'],paths)}}
save(proof/'artifacts.json',art)
for name,label in [('mac-native','native'),('mac-docker','mac-docker')]:
 src=source/name/'data';expected_files=read(audits/label/'data-before.json');assert {str(p.relative_to(src)):sha(p) for p in src.rglob('*') if p.is_file()}==expected_files
 copied=read(root/name/'data-copy-hashes.json');assert copied=={k:v for k,v in expected_files.items() if k!='peer.key'};assert all(sha(root/name/'data'/k)==v for k,v in copied.items())
save(proof/'local-copy-receipt.json',read(out/'local-copy-receipt.json'));backup=read(out/'vps-backup-original.log');assert backup['allHashesEqual'] and backup['sourceUnchanged'] and backup['sourceFiles']==2850 and backup['independentFileInodes'];save(proof/'vps-local-backup-receipt.json',backup)
print(json.dumps({'prepared':str(root),'pinChecks':checks,'sourceLimitsPreserved':'4k/4k/4k','localAndIndependentVpsPlanMatch':True,'signersStarted':False}))
