from pathlib import Path
import json,hashlib,subprocess,shlex,copy
repo=Path('/Users/boris/Dev/blockchain-nano');out=Path('/private/tmp/nano-restoration-cold-prepare-20261006');source=Path('/private/tmp/nano-restored-three-node-v28-reactor-failfast-20261006');audits=Path('/private/tmp/nano-v28-post-stop-native-replay-budget-20261006');root=Path('/private/tmp/nano-restored-three-node-v29-cold-20261006');proof=out/'proof';proof.mkdir()
def read(p):return json.loads(p.read_text())
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ssh(args):return subprocess.check_output(['/usr/bin/ssh','-4','-o','BatchMode=yes','-o','ConnectTimeout=8','ubuntu@145.239.199.166',shlex.join(args)],text=True)
def docker(args,remote=False):return ssh(['sudo','-n','docker']+args) if remote else subprocess.check_output(['/Users/boris/.docker/bin/docker']+args,text=True)
for name,remote in [('nano-restored-three-node-mac-muwtdigu',False),('nano-restored-three-node-vps-muwtdigu',True)]:
 state=json.loads(docker(['inspect',name,'--format','{{json .State}}'],remote));assert state['Status']=='exited' and state['ExitCode']==0 and not state['OOMKilled']
assert not docker(['ps','-q','--filter','volume=nano-restored-three-node-vps-20261005-data'],True).strip()
assert not any('/nano run ' in l for l in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())
x=read(repo/'docs/qualification/restored-three-node-v28-preflight-rejection-20261006/initial-preparation/input.json');old=copy.deepcopy(x)
for name,field,cfgfield in [('mac-native','macNativeCapsule','macNativeConfig'),('mac-docker','macCapsule','macConfig')]:
 x[field]=read(repo/'docs/qualification/current-core-historical-readonly-20261006'/name/'capture.json')
 actual=read(audits/(name+'-config.json'))['capella']
 for field2 in ['limits','clock']:assert x[cfgfield]['capella'][field2]==actual[field2]
x['vpsCapsule']=read(repo/'docs/qualification/current-core-historical-readonly-20261006/vps-docker/capture.json')
# V28 controller applied the native catch-up clock to every actor; independently
# check this public administrative configuration against VPS-local derive below.
x['vpsConfig']['capella']['clock']=copy.deepcopy(x['macNativeConfig']['capella']['clock'])
x['finalized']=x['macCapsule']['canonicalReplay']['finalized'];x['writersStopped']=True
x['retentionExperiment']={'blocks':'0x1000','rationale':'Bounded cold restoration of actual V28 snapshots; old saturation failure remains open.','captureBudget':{'blocks':'0x1000','envelopeBytes':'0x8000000'}}
x['coldCheckpoint']={'cacheRecords':'0x8','cacheEncodedBytes':'0x1000000'}
def identity(k):return ('index',k['index']) if k['index'] is not None else ('key',k['publicKey'])
checks={}
for n,(name,field) in enumerate([('mac-native','macNativeCapsule'),('mac-docker','macCapsule'),('vps-docker','vpsCapsule')]):
 current={identity(k):k['restart'] for k in x[field]['validatorKeys']};receipt=read(source/'campaign'/(str(n)+'-external-stop-receipt.json'));assert x[field]['restart']==receipt['restart']
 owned={identity(k):k['restart'] for k in receipt['validatorKeys']};historical={identity(k):k['restart'] for k in old[field]['validatorKeys'] if identity(k) not in owned}
 assert set(current)==set(owned)|set(historical);assert all(current[k]==v for k,v in (owned|historical).items())
 checks[name]={'ownedPinsVerified':len(owned),'historicalPinsVerified':len(historical),'fullInventory':len(current)}
save(proof/'input.json',x)
script="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';await writeFile(process.argv[2],JSON.stringify(restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8'))),null,2)+'\\n');"
subprocess.run(['node','--input-type=module','-e',script,str(proof/'input.json'),str(proof/'plan.json')],cwd=repo,check=True)
plan=read(proof/'plan.json');vps=copy.deepcopy(plan['configs'][2]);binary='/build/coherent-e33ad89e-amd64/network/nano-p2p';vps.update({'p2p_binary':binary,'p2p_identity':'/var/lib/nano/peer.key','bootnodes':[],'rpc_host':'127.0.0.1','rpc_port':8545,'listen_addr':'/ip4/127.0.0.1/tcp/30303'});vps['capella']['profile']['file']='/config/profile.json';vps['capella']['network']['enabled']=True
expected=hashlib.sha256(json.dumps(vps,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()).hexdigest();options={k:x[k] for k in ['retentionExperiment','coldCheckpoint']}
# Only helper source and administrative options/digest go to VPS; no configuration
# or signer material crosses hosts. Derive only, no writes yet.
helper=(repo/'tests/capella/vps-local-launch-prepare.py').read_text().split("if __name__=='__main__':")[0]
remote=helper+"\naudit=Path('/tmp/nano-restored-three-node-vps-muwtdigu/local-readonly-audit');c=derive(json.loads((audit/'config.json').read_text()),json.loads((audit/'capture.json').read_text()),8545,30303,sys.argv[1],json.loads(sys.argv[2]));assert canonical_hash(c)==sys.argv[3];print(json.dumps({'independentConfigDigestVerified':True,'localOnly':True}))"
assert json.loads(ssh(['python3','-c',remote,binary,json.dumps(options),expected]))['independentConfigDigestVerified']
save(proof/'signer-pin-verification.json',checks);save(proof/'vps-local-plan-check.json',{'independentConfigDigestVerified':True,'expectedCanonicalSHA256':expected,'noConfigurationExport':True,'signersStarted':False})
# Bin identities re-read from fully qualified outputs, not old candidate hashes.
art={'coreRevision':'e33ad89e61c0e915cf455aed3726da7cc3867ba0','relayRevision':'e33ad89e61c0e915cf455aed3726da7cc3867ba0','nodes':{},'networkQualified':False,'productionQualified':False}
for field in ['src','include','third_party','network']:subprocess.run(['git','diff','--quiet',art['coreRevision'],'--',field],cwd=repo,check=True)
np=Path('/private/tmp/nano-coherent-e33ad89e-20261006/native-build/nano');pp=Path('/private/tmp/nano-catalog-busy-integrated-native-20261006/network/nano-p2p');art['nodes']['mac-native']={'nano':{'path':str(np),'sha256':sha(np)},'p2p':{'path':str(pp),'sha256':sha(pp)}}
for arch,name,image,volume in [('arm64','mac-docker','e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','nano-clean-arm64-build-20261002'),('amd64','vps-docker','fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14','nano-clean-rpc-build-20261002')]:
 paths=['/build/coherent-e33ad89e-'+arch+'/nano','/build/coherent-e33ad89e-'+arch+'/network/nano-p2p'];lines=docker(['run','--rm','--network','none','--read-only','-v',volume+':/build:ro','--entrypoint','sha256sum',image]+paths,arch=='amd64').splitlines();hashes={l.split('  ',1)[1]:l.split('  ',1)[0] for l in lines};art['nodes'][name]={'image':'sha256:'+image,'buildVolume':volume,**{k:{'path':p,'sha256':hashes[p]} for k,p in zip(['nano','p2p'],paths)}}
save(proof/'artifacts.json',art)
# APFS independent clones preserve full journals and witnesses, excluding peer ID.
root.mkdir();counts={}
for name in ['mac-native','mac-docker']:
 src=source/name/'data';expected_files=read(audits/(name+'-before.json'));actual={str(p.relative_to(src)):sha(p) for p in sorted(src.rglob('*')) if p.is_file()};assert actual==expected_files
 dest=root/name/'data';dest.parent.mkdir();subprocess.run(['/bin/cp','-cR',str(src),str(dest)],check=True);(dest/'peer.key').unlink(missing_ok=True);copied={str(p.relative_to(dest)):sha(p) for p in sorted(dest.rglob('*')) if p.is_file()};assert copied=={k:v for k,v in actual.items() if k!='peer.key'};save(dest.parent/'data-copy-hashes.json',copied);counts[name]=len(copied)
save(proof/'local-copy-receipt.json',{'fullFilesHashVerified':True,'files':counts,'journalsAndWitnessesPreserved':True,'peerIdentityExcluded':True,'signersStarted':False})
print(json.dumps({'prepared':str(root),'checks':checks,'localCopies':counts,'vpsLocalDerivationMatchesIndependentPlan':True,'signersStarted':False}))
