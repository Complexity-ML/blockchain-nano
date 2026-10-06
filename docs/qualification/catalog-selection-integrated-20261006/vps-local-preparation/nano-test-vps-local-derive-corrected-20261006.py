from pathlib import Path
import subprocess,json,importlib.util,copy
repo=Path('/Users/boris/Dev/blockchain-nano');r=Path('/private/tmp/nano-v17-local-prepare-diagnostic-corrected-20261006');r.mkdir(exist_ok=False)
old=repo/'docs/qualification/restored-three-node-v16-admission-prepare-20261006';terminal=repo/'docs/qualification/restored-three-node-v16-admission-terminal-20261006';i=json.loads((old/'input.json').read_text())
for key,file in [('macNativeCapsule','post-stop-mac/mac-native-capture.json'),('macCapsule','post-stop-mac/mac-docker-capture.json'),('vpsCapsule','post-stop-vps/capture.json')]:i[key]=json.loads((terminal/file).read_text())
(r/'input.json').write_text(json.dumps(i))
code="import {readFile,writeFile} from 'node:fs/promises';import {restoredThreeNodePlan} from './tests/capella/restored-three-node-plan.mjs';await writeFile(process.argv[2],JSON.stringify(restoredThreeNodePlan(JSON.parse(await readFile(process.argv[1],'utf8')))));"
subprocess.run(['node','--input-type=module','-e',code,str(r/'input.json'),str(r/'plan.json')],cwd=repo,check=True)
p=json.loads((r/'plan.json').read_text());expected=p['configs'][2];binary='/build/catalog-selection-amd64-20261006/network/nano-p2p';expected.update({'p2p_binary':binary,'p2p_identity':'/var/lib/nano/peer.key','bootnodes':[],'rpc_host':'127.0.0.1','rpc_port':12345,'listen_addr':'/ip4/127.0.0.1/tcp/12346'});expected['capella']['profile']['file']='/config/profile.json';expected['capella']['network']['enabled']=True;expected['capella']['clock']=copy.deepcopy(p['configs'][0]['capella']['clock'])
spec=importlib.util.spec_from_file_location('local','/private/tmp/nano-vps-local-launch-prepare.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);digest=m.canonical_hash(expected)
source=json.loads(Path('/private/tmp/nano-restored-three-node-v16-admission-20261006/config/vps.json').read_text());actual=m.derive(source,i['vpsCapsule'],12345,12346,binary);assert m.canonical_hash(actual)==digest,'Derived local fixture differs from independent JS planner'
for label,caps,port in [('duplicate',copy.deepcopy(i['vpsCapsule']),12345),('unverified',copy.deepcopy(i['vpsCapsule']),12345),('invalid-port',copy.deepcopy(i['vpsCapsule']),12346)]:
 if label=='duplicate':caps['validatorKeys'].append(caps['validatorKeys'][0])
 if label=='unverified':caps['primaryWitnessAndSignersVerified']=False
 try:m.derive(source,caps,port,12346,binary)
 except AssertionError:continue
 raise AssertionError(label+' unexpectedly accepted')
(r/'summary.json').write_text(json.dumps({'cases':4,'independentPlannerMatches':True,'canonicalExpectedSHA256':digest,'signersStarted':False,'sourceConfigTransferred':False},indent=2));print('4 local derivation scenarios passed against independent restart planner')
