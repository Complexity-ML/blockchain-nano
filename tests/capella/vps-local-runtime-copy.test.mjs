import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../../',import.meta.url)).replace(/\/$/,'');
const helper=root+'/tests/capella/vps-local-launch-prepare.py';
const evidence=root+'/docs/qualification/';
const input=JSON.parse(readFileSync(evidence+'restored-three-node-v37-bounded-route-prepare-20261007/input.json','utf8'));
const refusal=JSON.parse(readFileSync(evidence+'v37-offline-copy-recovery-20261007/source-runtime-refusal-copy-audit.json','utf8'));
const copyProof=JSON.parse(readFileSync(evidence+'publication-cancel-boundary-integrated-20261007/failed-vps-readonly/copy-proof.json','utf8'));
const recoveryProof=JSON.parse(readFileSync(evidence+'v37-offline-copy-recovery-20261007/recovery-proof.json','utf8'));
const recovery=JSON.parse(readFileSync(evidence+'v37-offline-copy-recovery-20261007/offline-public/recovery-run-repair/recovery.json','utf8'));
// Synthetic file contents exercise launch admission, not the historical C++ replay.
// Formats/field names come from committed V37 evidence; fixture seals are local.
const code=String.raw`import tempfile,pathlib,json,hashlib,subprocess,sys,shutil,os,copy,importlib.util
x=json.load(sys.stdin);spec=importlib.util.spec_from_file_location('helper',sys.argv[1]);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
root=pathlib.Path(tempfile.mkdtemp(prefix='nano-restored-three-node-vps-runtimecopy-',dir='/tmp'));old=root/'config-origin';old.mkdir();audit=old/'local-readonly-audit';audit.mkdir();dest=root.with_name(root.name+'-destination');original=root/'source-original';original.mkdir();recovered=root/'recovered-data'
def sha(wire):return hashlib.sha256(wire).hexdigest()
def save(name,value):path=audit/name;path.write_text(json.dumps(value,indent=2)+'\n');return path.read_bytes()
def manifest(directory,prefix):return ''.join(sorted(sha(p.read_bytes())+'  '+prefix+p.relative_to(directory).as_posix()+'\n' for p in directory.rglob('*') if p.is_file())).encode()
def payload(path,content):path.parent.mkdir(parents=True,exist_ok=True);path.write_text(content)
try:
 config=copy.deepcopy(x['input']['vpsConfig']);capsule=copy.deepcopy(x['input']['vpsCapsule']);a=copy.deepcopy(x['refusal']);cp=copy.deepcopy(x['copyProof']);rp=copy.deepcopy(x['recoveryProof']);rec=copy.deepcopy(x['recovery'])
 capsule['restart']=copy.deepcopy(a['recoveredPrimary'])
 paths=[config['capella']['recoveryWitness']]
 for k in config['capella']['validator']['keys']:paths.extend([k['journal'],k['recoveryWitness']])
 for path in paths:
  assert path.startswith('/var/lib/nano/');payload(original/path[len('/var/lib/nano/'):]/'record','unchanged signed intent '+path)
 payload(original/'chain'/'state','before publication');payload(original/'peer.key','synthetic-peer')
 if x['case']=='cross-named-shared-file-storage':payload(original/'peer-other.key','synthetic-peer')
 source_wire=manifest(original,'/source/');shutil.copytree(original,recovered);copy_wire=manifest(recovered,'/recovery/data/');payload(recovered/'chain'/'state','witnessed publication');after_wire=manifest(recovered,'/recovery/data/')
 config['capella']['recoveryWitness']=config['capella']['recoveryWitness'].replace('/var/lib/nano/','/recovery/data/')
 for k in config['capella']['validator']['keys']:
  for field in ['journal','recoveryWitness']:k[field]=k[field].replace('/var/lib/nano/','/recovery/data/')
 for name in ['genesis.json','profile.json','initial.json']:(old/name).write_text('{}')
 (old/'vps.env').write_text(''.join(k['keyEnv']+'=synthetic-only\n' for k in config['capella']['validator']['keys'])+'NANO_TEST_NETWORK_TRACE=1\n');os.chmod(old/'vps.env',0o600)
 capture=save('capture.json',capsule);save('config.json',config)
 a['sourceManifestSha256']=sha(source_wire);a['externalCurrentCaptureSha256']=sha(capture);raw_a=save('source-runtime-refusal-copy-audit.json',a)
 count=len(source_wire.splitlines());cp.update(sourceManifestSha256=sha(source_wire),files=count);save('copy-proof.json',cp)
 rp.update(captureSha256=sha(capture),sourceFiles=count);save('recovery-proof.json',rp);save('recovery.json',rec)
 for name,wire in [('source-before.sha256',source_wire),('source-volume-current.sha256',source_wire),('copy-before.sha256',copy_wire),('copy-after.sha256',after_wire)]:(audit/name).write_bytes(wire)
 proof={'format':'nano-private-audited-runtime-copy-source-v1','hashesUnchanged':True,'requestPreparedOnVps':True,'noPrivateDataExport':True,'readOnly':True,'runtimeRefusalAuditSHA256':sha(raw_a),'sourceManifestSha256':sha(source_wire),'recoveredManifestSha256':sha(after_wire),'captureSHA256':sha(capture),'auditedRecoveryCommitment':capsule['restart'],'files':len(after_wire.splitlines())}
 bindings={p.name:sha(p.read_bytes()) for p in audit.iterdir() if p.is_file()}
 def rebind(name):bindings[name]=sha((audit/name).read_bytes())
 case=x['case'];valid=case in ('valid','valid-startup-catchup')
 options={}
 if case=='valid-startup-catchup':options={'startupCatchup':{'maximumDowntimeMs':'0x249f0000','maximumSteps':'0x64'}}
 planned=m.derive(m.remap_runtime_copy_paths(config),capsule,8545,30303,x['binary'],options)
 expected=m.canonical_hash(planned)
 if case=='missing-runtime-seal':del proof['runtimeRefusalAuditSHA256']
 elif case=='foreign-runtime-seal':proof['runtimeRefusalAuditSHA256']='0'*64
 elif case=='missing-capture-binding':del bindings['capture.json']
 elif case=='substituted-capture':(audit/'capture.json').write_bytes(capture+b' ');rebind('capture.json')
 elif case=='foreign-checkpoint':proof['auditedRecoveryCommitment']={'sequence':'0x1','digest':'0x'+'0'*64}
 elif case=='foreign-source-volume-observation':(audit/'source-volume-current.sha256').write_bytes(source_wire+b' ');rebind('source-volume-current.sha256')
 elif case=='false-current-pairs':rp['current33SignerWitnessPairsVerified']=False;save('recovery-proof.json',rp);rebind('recovery-proof.json')
 elif case=='claimed-live-recovery':rp['runtimeDutiesResumed']=True;save('recovery-proof.json',rp);rebind('recovery-proof.json')
 elif case=='changed-original-manifest':proof['sourceManifestSha256']='0'*64
 elif case=='changed-recovered-manifest':proof['recoveredManifestSha256']='0'*64
 elif case=='false-historical-audit':a['preCampaignExternalMinimumsVerified']=False;wire=save('source-runtime-refusal-copy-audit.json',a);rebind('source-runtime-refusal-copy-audit.json');proof['runtimeRefusalAuditSHA256']=sha(wire)
 elif case=='false-copy-proof':cp['independentCopyVerified']=False;save('copy-proof.json',cp);rebind('copy-proof.json')
 elif case=='forged-copy-file-count':cp['files']=count-1;save('copy-proof.json',cp);rebind('copy-proof.json')
 elif case=='mismatched-recovery-token':rec['afterPrimary']=a['sourcePrimary'];save('recovery.json',rec);rebind('recovery.json')
 elif case=='mutated-original-file':payload(original/'chain'/'state','different original')
 elif case=='mutated-recovered-file':payload(recovered/'chain'/'state','different recovery')
 elif case=='unlisted-recovered-file':payload(recovered/'chain'/'extra','unlisted')
 elif case=='missing-recovered-file':(recovered/'chain'/'state').unlink()
 elif case=='missing-original-file':(original/'chain'/'state').unlink()
 elif case=='shared-file-storage':(recovered/'peer.key').unlink();os.link(original/'peer.key',recovered/'peer.key')
 elif case=='cross-named-shared-file-storage':(recovered/'peer.key').unlink();os.link(original/'peer-other.key',recovered/'peer.key')
 elif case=='symlinked-recovered-file':(recovered/'peer.key').unlink();(recovered/'peer.key').symlink_to(original/'peer.key')
 elif case=='symlinked-recovered-directory':shutil.rmtree(recovered);recovered.symlink_to(original,target_is_directory=True)
 elif case=='changed-signer-manifest':
  payload(recovered/'peer.key','changed signer-side inventory');wire=manifest(recovered,'/recovery/data/');(audit/'copy-after.sha256').write_bytes(wire);rebind('copy-after.sha256');proof['recoveredManifestSha256']=sha(wire)
 elif case=='omitted-signer-manifest':
  (recovered/'peer.key').unlink();wire=manifest(recovered,'/recovery/data/');(audit/'copy-after.sha256').write_bytes(wire);rebind('copy-after.sha256');proof['recoveredManifestSha256']=sha(wire);proof['files']=len(wire.splitlines())
 elif case=='false-exact-stop-bypass':proof['exactStopCommitmentsVerified']=True
 elif case=='empty-recovered-data':shutil.rmtree(recovered);recovered.mkdir()
 elif case=='missing-files-count':del proof['files']
 elif case=='false-files-count':proof['files']=True
 elif case=='unbound-config':del bindings['config.json']
 elif case=='foreign-remap-path':config['capella']['recoveryWitness']='/outside/witness';save('config.json',config);rebind('config.json')
 elif case=='traversal-remap-path':config['capella']['validator']['keys'][0]['journal']='/recovery/data/../original';save('config.json',config);rebind('config.json')
 elif case=='manifest-path-traversal':
  wire=after_wire+('0'*64+'  /recovery/data/../escape\n').encode();(audit/'copy-after.sha256').write_bytes(wire);rebind('copy-after.sha256');proof['recoveredManifestSha256']=sha(wire)
 elif case=='duplicate-manifest-path':
  wire=after_wire+after_wire.splitlines(keepends=True)[0];(audit/'copy-after.sha256').write_bytes(wire);rebind('copy-after.sha256');proof['recoveredManifestSha256']=sha(wire)
 elif case not in ('valid','valid-startup-catchup'):raise AssertionError('unknown case '+case)
 save('immutability.json',proof);save('request-config-hashes.json',bindings)
 source_before=m.tree_hashes(original)[0]
 # The subprocess CLI must reject mismatches before creating destination files.
 args=[sys.executable,'-B',sys.argv[1],str(old),str(dest),'8545','30303',x['binary'],expected,json.dumps(options)]
 run=subprocess.run(args,capture_output=True,text=True)
 if valid:
  assert run.returncode==0,run.stderr;receipt=json.loads(run.stdout);assert receipt['runtimeCopySource']['actualOriginalAndRecoveredFilesVerified'] is True
  assert receipt['runtimeCopySource']['captureSHA256']==sha(capture);assert receipt['runtimeCopySource']['auditedConfigSHA256']==bindings['config.json'];assert receipt['runtimeCopySource']['onlyWitnessAndJournalPathsRemapped'] is True
  assert json.loads((dest/'vps.json').read_text())==planned and (dest/'vps.env').read_text()==(old/'vps.env').read_text()
  assert 'synthetic-only' not in run.stdout;assert source_before==m.tree_hashes(original)[0]
  assert not (recovered/'peer.key').is_symlink();assert not (audit/'config.json').is_symlink()
 else:
  assert run.returncode!=0,'accepted '+case;assert not dest.exists(),'wrote destination before refusing '+case
 print(json.dumps({'case':case,'accepted':valid,'destinationWritten':dest.exists(),'historicalRecoveryNotRequalified':True}))
finally:
 shutil.rmtree(root);shutil.rmtree(dest,ignore_errors=True)
`;
function run(caseName){
 const r=spawnSync('python3',['-B','-c',code,helper],{input:JSON.stringify({input,refusal,copyProof,recoveryProof,recovery,case:caseName,binary:'/build/dependency-route-amd64-20261007/network/nano-p2p'}),encoding:'utf8'});
 assert.equal(r.status,0,r.stderr);return JSON.parse(r.stdout);
}
for(const name of ['valid','valid-startup-catchup'])test('admits sealed runtime-copy '+name+' with actual full-file verification',()=>assert.equal(run(name).accepted,true));
for(const name of ['missing-runtime-seal','foreign-runtime-seal','missing-capture-binding','substituted-capture','foreign-checkpoint','foreign-source-volume-observation','false-current-pairs','claimed-live-recovery','changed-original-manifest','changed-recovered-manifest','false-historical-audit','false-copy-proof','forged-copy-file-count','mismatched-recovery-token','mutated-original-file','mutated-recovered-file','unlisted-recovered-file','missing-recovered-file','missing-original-file','shared-file-storage','cross-named-shared-file-storage','symlinked-recovered-file','symlinked-recovered-directory','changed-signer-manifest','omitted-signer-manifest','false-exact-stop-bypass','empty-recovered-data','missing-files-count','false-files-count','unbound-config','foreign-remap-path','traversal-remap-path','manifest-path-traversal','duplicate-manifest-path'])test('refuses '+name+' before destination writes',()=>{const result=run(name);assert.equal(result.accepted,false);assert.equal(result.destinationWritten,false);});

test('explicit startup catch-up preserves the unchanged clock and validates the ceiling',()=>{
 const clockCode=String.raw`import importlib.util,json,sys,copy
s=importlib.util.spec_from_file_location('helper',sys.argv[1]);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
c=json.load(sys.stdin);before=copy.deepcopy(c);forward=int(c['capella']['clock']['maximumForwardStepMs'],16);downtime=forward*3+1
valid={'maximumDowntimeMs':hex(downtime),'maximumSteps':'0x4'};actual=m.apply_runtime_options(c,{'startupCatchup':valid});expected=copy.deepcopy(c);expected['capella']['clock']['startupCatchup']=valid;assert actual==expected and c==before
for changed in [{'maximumDowntimeMs':hex(downtime),'maximumSteps':'0x3'},{'maximumDowntimeMs':'0x0','maximumSteps':'0x4'},{'maximumDowntimeMs':'0x01','maximumSteps':'0x4'},{'maximumDowntimeMs':'0x10000000000000000','maximumSteps':'0x4'},{'maximumDowntimeMs':hex(downtime)},{**valid,'maximumForwardStepMs':'0x1'},{'maximumDowntimeMs':hex(downtime),'maximumSteps':'0x0'},{'maximumDowntimeMs':hex(downtime),'maximumSteps':'0x10000000000000000'}]:
 try:m.apply_runtime_options(c,{'startupCatchup':changed})
 except (AssertionError,KeyError):pass
 else:raise AssertionError('accepted invalid startup catch-up')
assert c==before;print('unchanged limits, clock fields and pins; ceiling enforced')`;
 const r=spawnSync('python3',['-B','-c',clockCode,helper],{input:JSON.stringify(input.vpsConfig),encoding:'utf8'});assert.equal(r.status,0,r.stderr);
});


test('resolved preparation paths refuse source and destination symlink escapes before writes',()=>{
 const code=String.raw`import tempfile,pathlib,json,sys,shutil,importlib.util
s=importlib.util.spec_from_file_location('helper',sys.argv[1]);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
base=pathlib.Path(tempfile.mkdtemp(prefix='nano-restored-three-node-vps-paths-',dir='/tmp'));outside=pathlib.Path(tempfile.mkdtemp(prefix='nano-outside-paths-',dir=sys.argv[2]))
try:
 origin=outside/'config-origin';origin.mkdir();alias=base/'config-origin';alias.symlink_to(origin,target_is_directory=True);dest=base.with_name(base.name+'-destination')
 try:m.prepare(alias,dest,8545,30303,'/build/dependency-route-amd64-20261007/network/nano-p2p','0'*64)
 except AssertionError:pass
 else:raise AssertionError('accepted symlinked source')
 assert not dest.exists()
 origin=base/'real-origin';origin.mkdir();escape=base/'runtime';escape.symlink_to(outside,target_is_directory=True);dest=escape/'nano-restored-three-node-vps-destination'
 try:m.prepare(origin,dest,8545,30303,'/build/dependency-route-amd64-20261007/network/nano-p2p','0'*64)
 except AssertionError:pass
 else:raise AssertionError('accepted destination symlink escape')
 assert not dest.exists();print('source and destination escapes refused before preparation writes')
finally:shutil.rmtree(base);shutil.rmtree(outside)`;
 const r=spawnSync('python3',['-B','-c',code,helper,root],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
});
