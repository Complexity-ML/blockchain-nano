import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {restoredThreeNodePlan} from './restored-three-node-plan.mjs';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../../',import.meta.url)).replace(/\/$/,'');
const fixture=JSON.parse(readFileSync(root+'/docs/qualification/restored-three-node-prepare-20261005/input.json','utf8'));
const helper=root+'/tests/capella/vps-local-launch-prepare.py';
const binary='/build/coherent-e33ad89e-amd64/network/nano-p2p';
const options={retentionExperiment:{blocks:'0x1000',catalogRootBytes:'0x20000',rationale:'Bounded cold restoration beyond the historical graph boundary.',captureBudget:{blocks:'0x1000',envelopeBytes:'0x8000000'}},coldCheckpoint:{cacheRecords:'0x8',cacheEncodedBytes:'0x1000000'}};
function derive(input=fixture, runtime={}, path=binary){
 const r=spawnSync('python3',['-B','-c',`import importlib.util,json,sys\ns=importlib.util.spec_from_file_location('helper',sys.argv[1]);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)\nx=json.load(sys.stdin);print(json.dumps(m.derive(x['input']['vpsConfig'],x['input']['vpsCapsule'],8545,30303,x['binary'],x['options'])))`,helper],{input:JSON.stringify({input,options:runtime,binary:path}),encoding:'utf8'});
 return r.status===0?{value:JSON.parse(r.stdout)}:{error:true};
}
function expected(input){const c=structuredClone(restoredThreeNodePlan(input).configs[2]);Object.assign(c,{p2p_binary:binary,p2p_identity:'/var/lib/nano/peer.key',bootnodes:[],rpc_host:'127.0.0.1',rpc_port:8545,listen_addr:'/ip4/127.0.0.1/tcp/30303'});c.capella.profile.file='/config/profile.json';c.capella.network.enabled=true;return c;}
test('legacy VPS derivation equals the independent JS plan',()=>assert.deepEqual(derive().value,expected(fixture)));
test('cold 4096-record preparation equals JS plan and preserves every signed pin and clock',()=>{const x={...structuredClone(fixture),...options},before=structuredClone(x),actual=derive(x,options).value;assert.deepEqual(actual,expected(x));assert.deepEqual(x,before);assert.deepEqual(actual.capella.clock,fixture.vpsConfig.capella.clock);assert.deepEqual(actual.capella.recoveryCommitment,fixture.vpsCapsule.restart);assert.deepEqual(actual.capella.validator.keys.map(k=>k.restart),fixture.vpsCapsule.validatorKeys.map(k=>k.restart));});
test('legacy retention policy remains supported without explicit capture budget',()=>{const o={retentionExperiment:{blocks:'0x400',rationale:options.retentionExperiment.rationale}};const x={...structuredClone(fixture),...o};assert.deepEqual(derive(x,o).value,expected(x));});
for(const [name,change] of [
 ['same graph budget',o=>o.retentionExperiment.blocks=fixture.vpsConfig.capella.limits.blocks],
 ['capture count too small',o=>o.retentionExperiment.captureBudget.blocks='0x400'],
 ['capture bytes too small',o=>o.retentionExperiment.captureBudget.envelopeBytes='0x10'],
 ['noncanonical count',o=>o.retentionExperiment.blocks='0x01000'],
 ['uint64 overflow',o=>o.retentionExperiment.blocks='0x10000000000000000'],
 ['witness overflow',o=>o.retentionExperiment.captureBudget.envelopeBytes='0xffffffffffffffff'],
 ['missing coupled inventory budget',o=>delete o.retentionExperiment.catalogRootBytes],
 ['insufficient coupled inventory budget',o=>o.retentionExperiment.catalogRootBytes='0x10000'],
 ['noncanonical catalog budget',o=>o.retentionExperiment.catalogRootBytes='0x020000'],
 ['excessive catalog budget',o=>o.retentionExperiment.catalogRootBytes='0x8000001'],
 ['missing rationale',o=>delete o.retentionExperiment.rationale],
 ['unknown consensus override',o=>o.retentionExperiment.profile='changed'],
 ['cold records beyond budget',o=>o.coldCheckpoint.cacheRecords='0x1001'],
 ['cold bytes beyond budget',o=>o.coldCheckpoint.cacheEncodedBytes='0x8000001'],
 ['zero cold cache',o=>o.coldCheckpoint.cacheRecords='0x0'],
 ['cold unknown field',o=>o.coldCheckpoint.automaticRecovery=true]
])test('both independent planners reject '+name,()=>{const o=structuredClone(options);change(o);assert.equal(derive(fixture,o).error,true);assert.throws(()=>restoredThreeNodePlan({...structuredClone(fixture),...o}));});
test('unchanged cold options are allowed but sticky options cannot change',()=>{const x=structuredClone(fixture);x.vpsConfig.capella.coldCheckpoint={...options.coldCheckpoint};assert.ok(derive(x,options).value);const changed=structuredClone(options);changed.coldCheckpoint.cacheRecords='0x9';assert.equal(derive(x,changed).error,true);});
for(const path of ['/build/coherent-e33ad89e-arm64/network/nano-p2p','/build/../coherent-amd64/network/nano-p2p','/tmp/coherent-amd64/network/nano-p2p'])test('refuses foreign binary '+path,()=>assert.equal(derive(fixture,{},path).error,true));
test('dated qualified AMD64 path remains accepted',()=>assert.ok(derive(fixture,{},'/build/fragment-runtime-amd64-20261006/network/nano-p2p').value));
for(const [name,change] of [['retired signer',x=>x.vpsCapsule.validatorKeys[0].retired=true],['missing pin',x=>x.vpsCapsule.validatorKeys.pop()],['foreign profile',x=>x.vpsCapsule.profileSha256='0x'+'11'.repeat(32)]])test('new options never bypass '+name,()=>{const x=structuredClone(fixture);change(x);assert.equal(derive(x,options).error,true);});
test('options cannot override clock or keys',()=>{assert.equal(derive(fixture,{clock:{intervalMs:'0x1'}}).error,true);assert.equal(derive(fixture,{validator:{keys:[]}}).error,true);});
test('CLI prepares only locally after independent digest match and never returns signer environment',()=>{
 const input=structuredClone(fixture),planned=expected({...input,...options});
 const r=spawnSync('python3',['-B','-c',`import tempfile,pathlib,json,hashlib,subprocess,sys,shutil,os\nx=json.load(sys.stdin);old=pathlib.Path(tempfile.mkdtemp(prefix='nano-restored-three-node-vps-test-',dir='/tmp'));dest=old.with_name(old.name+'-destination')\ntry:\n a=old/'local-readonly-audit';a.mkdir()\n for name,value in [('config.json',x['input']['vpsConfig']),('capture.json',x['input']['vpsCapsule'])]: (a/name).write_text(json.dumps(value))\n (a/'immutability.json').write_text(json.dumps({'hashesUnchanged':True,'exactStopCommitmentsVerified':True}))\n (a/'request-config-hashes.json').write_text(json.dumps({n:hashlib.sha256((a/n).read_bytes()).hexdigest() for n in ['config.json','capture.json']}))\n env='\\n'.join(k['keyEnv']+'=synthetic-only' for k in x['input']['vpsConfig']['capella']['validator']['keys'])+'\\nNANO_TEST_NETWORK_TRACE=1\\n';(old/'vps.env').write_text(env)\n for n in ['genesis.json','profile.json','initial.json']:(old/n).write_text('{}')\n args=[sys.executable,sys.argv[1],str(old),str(dest),'8545','30303',x['binary']]\n bad=subprocess.run(args+['0'*64,json.dumps(x['options'])],capture_output=True,text=True);assert bad.returncode!=0 and not dest.exists()\n digest=hashlib.sha256(json.dumps(x['planned'],sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()).hexdigest()\n good=subprocess.run(args+[digest,json.dumps(x['options'])],capture_output=True,text=True);assert good.returncode==0,good.stderr\n proof=json.loads(good.stdout);assert proof['configCanonicalSHA256']==digest;assert json.loads((dest/'vps.json').read_text())==x['planned'];assert (dest/'vps.env').read_text()==env;assert (dest/'vps.env').stat().st_mode&0o777==0o600;assert 'synthetic-only' not in good.stdout\n print(json.dumps({'independentHashVerified':True,'mismatchRefusedBeforeWrites':True,'localEnvironmentPreserved':True,'environmentNotEmitted':True}))\nfinally:\n shutil.rmtree(old);shutil.rmtree(dest,ignore_errors=True)`,helper],{input:JSON.stringify({input,options,binary,planned}),encoding:'utf8'});
 assert.equal(r.status,0,r.stderr);assert.deepEqual(JSON.parse(r.stdout),{independentHashVerified:true,mismatchRefusedBeforeWrites:true,localEnvironmentPreserved:true,environmentNotEmitted:true});
});

function mixedSource(){const x=structuredClone(fixture);x.vpsConfig.capella.limits.blocks='0x1000';x.vpsConfig.capella.coldCheckpoint=structuredClone(options.coldCheckpoint);const o=structuredClone(options);o.retentionExperiment.sourceBlockBudgets={'mac-native':x.macConfig.capella.limits.blocks,'mac-docker':x.macConfig.capella.limits.blocks,'vps-docker':'0x1000'};return {x,o};}
test('independently audited mixed source budgets converge without lowering or rewriting source pins',()=>{const {x,o}=mixedSource(),before=structuredClone(x);assert.deepEqual(derive(x,o).value,expected({...x,...o}));assert.deepEqual(x,before);const p=restoredThreeNodePlan({...x,...o});assert.deepEqual(p.retentionBudget.sourceBlockBudgets,o.retentionExperiment.sourceBlockBudgets);assert.deepEqual(p.sourceCheckpoints,{mac:x.macCapsule.restart,vps:x.vpsCapsule.restart});});
for(const [name,change] of [
 ['undeclared disparity',(x,o)=>delete o.retentionExperiment.sourceBlockBudgets],
 ['wrong VPS source',(x,o)=>o.retentionExperiment.sourceBlockBudgets['vps-docker']='0x400'],
 ['missing owner',(x,o)=>delete o.retentionExperiment.sourceBlockBudgets['mac-native']],
 ['unknown owner',(x,o)=>o.retentionExperiment.sourceBlockBudgets.other='0x400'],
 ['noncanonical source',(x,o)=>o.retentionExperiment.sourceBlockBudgets['vps-docker']='0x01000'],
 ['candidate below VPS',(x,o)=>o.retentionExperiment.blocks='0x800'],
 ['source budget not uint64',(x,o)=>o.retentionExperiment.sourceBlockBudgets['vps-docker']='0x10000000000000000']
])test('both planners reject mixed source '+name,()=>{const {x,o}=mixedSource();change(x,o);assert.equal(derive(x,o).error,true);assert.throws(()=>restoredThreeNodePlan({...x,...o}));});
test('heterogeneous budget declaration never permits other source resource changes',()=>{const {x,o}=mixedSource();x.vpsConfig.capella.limits.messages='0x801';assert.throws(()=>restoredThreeNodePlan({...x,...o}));});

test('audited forced-stop source binds current capsule and rejects incomplete crash evidence',()=>{
 const code=`import importlib.util,json,sys,hashlib,copy
s=importlib.util.spec_from_file_location('helper',sys.argv[1]);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
c=json.load(sys.stdin);wire=json.dumps(c).encode();digest=hashlib.sha256(wire).hexdigest();count=len(c['validatorKeys'])
p={'format':'nano-private-audited-crash-source-v1','hashesUnchanged':True,'minimumCoreAndSignerCommitmentsVerified':True,'primaryAndIndependentWitnessVerified':True,'requestPreparedOnVps':True,'noPrivateDataExport':True,'readOnly':True,'sourceForcedStopExit':137,'sourceOomKilled':False,'exactUnchangedSignerPins':count,'signerInventory':count,'files':2936,'captureSHA256':digest,'auditedRecoveryCommitment':c['restart']};b={'capture.json':digest}
m.verify_local_audit(p,wire,b);original=copy.deepcopy(p);refusals=0
for key in p:
 q=copy.deepcopy(p);del q[key]
 try:m.verify_local_audit(q,wire,b)
 except (AssertionError,KeyError):refusals+=1
 else:raise AssertionError('accepted missing '+key)
for key,value in [('sourceForcedStopExit',0),('sourceOomKilled',True),('exactUnchangedSignerPins',count-1),('files',0),('captureSHA256','0'*64),('auditedRecoveryCommitment',{'sequence':'0x1','digest':'0x'+'0'*64})]:
 q=copy.deepcopy(p);q[key]=value
 try:m.verify_local_audit(q,wire,b)
 except AssertionError:refusals+=1
 else:raise AssertionError('accepted altered '+key)
for changed,bindings in [(wire+b' ',b),(wire,{}),(wire,{'capture.json':'0'*64})]:
 try:m.verify_local_audit(p,changed,bindings)
 except AssertionError:refusals+=1
 else:raise AssertionError('accepted unbound capsule')
assert p==original
print(json.dumps({'validCrashSourceAccepted':True,'refusals':refusals,'proofUnchanged':True}))`;
 const r=spawnSync('python3',['-B','-c',code,helper],{input:JSON.stringify(fixture.vpsCapsule),encoding:'utf8'});
 assert.equal(r.status,0,r.stderr);const result=JSON.parse(r.stdout);assert.equal(result.validCrashSourceAccepted,true);assert.ok(result.refusals>=20);assert.equal(result.proofUnchanged,true);
});
