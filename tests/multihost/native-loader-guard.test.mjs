// Pure synthetic reader/callbacks. No fs, process, environment or exporter.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {c8CampaignFormat,selectNativeExporterLoaderGuard} from './native-loader-guard.mjs';

const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
const json=value=>Buffer.from(JSON.stringify(value));
function fixture(){
 const files=new Map(),reads=[];
 function body(path,bytes){const buffer=Buffer.from(bytes);files.set(path,{buffer});return {path,sha256:sha(buffer),bytes:buffer.length};}
 const exporter=body('/synthetic/native/nano-capella-transition-tests',Buffer.alloc(64,11));
 const shim=body('/synthetic/native/libnano-test-sync-fault.dylib',Buffer.alloc(48,22));
 const material=size=>['16777220','9007199254740993','33088','501','20','1',String(size),'1700000000123456789','1700000000123456791'];
 for(const pin of [exporter,shim])files.get(pin.path).material=material(pin.bytes);
 const original=body('/synthetic/authority/original-overlay.json',json({synthetic:true}));
 const receipt={format:'nano-root-native-exporter-loader-relation-v1',issuer:'ROOT',exporter,shim,
  architecture:'arm64',loadCommand:'LC_LOAD_DYLIB',dependencyInstallName:'@rpath/libnano-test-sync-fault.dylib',
  shimInstallName:'@rpath/libnano-test-sync-fault.dylib',firstRpath:'@loader_path',inspectionPassed:true,
  globalDyldClosureQualified:false,productionQualified:false};
 const structuralReceipt=body('/synthetic/authority/structure.json',json(receipt));
 const envelope={format:'nano-root-fresh-capella-c8-overlay-v1',sourceBaseRevision:'a'.repeat(40),overlayManifest:original,
  nativeExporterLoaderClosure:{format:'nano-native-exporter-loader-closure-v1',
   exporter:{...exporter,originalMaterial:[...files.get(exporter.path).material]},
   shim:{...shim,originalMaterial:[...files.get(shim.path).material]},structuralReceipt}};
 const spec={format:c8CampaignFormat,sourceBaseRevision:'a'.repeat(40),overlayManifest:{path:'/synthetic/authority/c8-overlay.json',sha256:sha(json(envelope)),bytes:json(envelope).length},
  owners:{'mac-native':{uid:501,gid:20,programs:{exporter:structuredClone(exporter)}}},
  fixture:{seedFile:'/synthetic/private/seed',ledgerDirectory:'/synthetic/private/ledger'}};
 async function readArtifact(pin,{body:withBody}){
  reads.push({path:pin.path,body:withBody});const entry=files.get(pin.path);
  assert(entry,'synthetic missing file');assert.equal(sha(entry.buffer),pin.sha256,'synthetic known SHA');assert.equal(entry.buffer.length,pin.bytes,'synthetic known bytes');
  return withBody?Buffer.from(entry.buffer):{...pin,material:[...entry.material]};
 }
 function guard(){return selectNativeExporterLoaderGuard(spec,json(envelope),{readArtifact});}
 function updateReceipt(edit){edit(receipt);Object.assign(structuralReceipt,body(structuralReceipt.path,json(receipt)));}
 return {files,reads,spec,envelope,exporter,shim,receipt,guard,updateReceipt};
}
async function beforeLaunchRefusal(change){
 const f=fixture();change(f);let callbacks=0;
 await assert.rejects(async()=>{const guard=f.guard();await guard.aroundExporter(async()=>{++callbacks;});});
 assert.equal(callbacks,0,'no launch callback on refused preflight');return f;
}

test('good Root join verifies exact string material before and after an inert callback',async()=>{
 const f=fixture(),before=json({spec:f.spec,envelope:f.envelope,receipt:f.receipt});
 const guard=f.guard();assert(Object.isFrozen(guard));let callbacks=0;
 const result=await guard.aroundExporter(async()=>{++callbacks;assert.equal(f.reads.length,4);return 'synthetic-only';});
 assert.equal(result,'synthetic-only');assert.equal(callbacks,1);assert.equal(f.reads.length,8);
 assert.deepEqual(f.reads.slice(0,4),[
  {path:'/synthetic/authority/original-overlay.json',body:true},
  {path:'/synthetic/authority/structure.json',body:true},
  {path:f.exporter.path,body:false},{path:f.shim.path,body:false}]);
 assert.deepEqual(f.reads.slice(4),f.reads.slice(0,4));
 assert.deepEqual(json({spec:f.spec,envelope:f.envelope,receipt:f.receipt}),before);
 // Number would lose this difference; all nine values remain exact strings.
 assert.equal(Number('9007199254740993'),Number('9007199254740992'));
 await beforeLaunchRefusal(x=>{x.files.get(x.shim.path).material[1]='9007199254740992';});
});

test('missing, replaced or historically changed shim refuses before callback',async()=>{
 await beforeLaunchRefusal(f=>{f.files.delete(f.shim.path);});
 await beforeLaunchRefusal(f=>{f.files.get(f.shim.path).buffer=Buffer.alloc(f.shim.bytes,33);});
 for(let field=0;field<9;field++)await beforeLaunchRefusal(f=>{
  const row=f.files.get(f.shim.path).material;row[field]=String(BigInt(row[field])+1n);
 });
 await beforeLaunchRefusal(f=>{f.envelope.nativeExporterLoaderClosure.shim.originalMaterial[8]='1700000000123456792';});
});

test('foreign path, selected exporter delta, missing and malformed C8 sections fail closed',async()=>{
 for(const change of [
  f=>{f.envelope.nativeExporterLoaderClosure.shim.path='/synthetic/foreign/libnano-test-sync-fault.dylib';},
  f=>{f.envelope.nativeExporterLoaderClosure.shim.path='/synthetic/native/../native/libnano-test-sync-fault.dylib';},
  f=>{f.spec.owners['mac-native'].programs.exporter.sha256='f'.repeat(64);},
  f=>{f.spec.owners['mac-native'].programs.exporter.path='/synthetic/foreign/nano-capella-transition-tests';},
  f=>{delete f.envelope.nativeExporterLoaderClosure;},
  f=>{f.envelope.nativeExporterLoaderClosure.extra=true;},
  f=>{f.envelope.extra=true;},
  f=>{f.envelope.sourceBaseRevision='b'.repeat(40);},
  f=>{f.envelope.format='legacy';},
  f=>{f.envelope.overlayManifest.path=f.shim.path;},
  f=>{f.spec.fixture.ledgerDirectory='/synthetic/native';},
  f=>{f.files.delete('/synthetic/authority/structure.json');},
  f=>{f.files.delete('/synthetic/authority/original-overlay.json');}
 ])await beforeLaunchRefusal(change);
});

test('stat ns stay canonical decimal strings; false types and wrong metadata are rejected',async()=>{
 for(const invalid of [1700000000123456789,17n.toString()+'n','017','-0',true,null,'1.5','9223372036854775808'])await beforeLaunchRefusal(f=>{
  f.envelope.nativeExporterLoaderClosure.shim.originalMaterial[8]=invalid;
 });
 for(const change of [
  f=>{f.envelope.nativeExporterLoaderClosure.exporter.bytes=true;},
  f=>{f.envelope.nativeExporterLoaderClosure.exporter.originalMaterial[2]='16832';},
  f=>{f.envelope.nativeExporterLoaderClosure.exporter.originalMaterial[2]='33024';},
  f=>{f.envelope.nativeExporterLoaderClosure.shim.originalMaterial[5]='2';},
  f=>{f.envelope.nativeExporterLoaderClosure.shim.originalMaterial[3]='0';},
  f=>{f.envelope.nativeExporterLoaderClosure.shim.originalMaterial.pop();}
 ])await beforeLaunchRefusal(change);
});

test('Root structural receipt must bind these exact binaries and the qualified relation',async()=>{
 for(const change of [
  r=>{r.exporter={...r.exporter,sha256:'f'.repeat(64)};},
  r=>{r.shim={...r.shim,bytes:r.shim.bytes+1};},
  r=>{r.architecture='amd64';},r=>{r.firstRpath='/foreign';},
  r=>{r.shimInstallName='/foreign/libnano-test-sync-fault.dylib';},
  r=>{r.dependencyInstallName='/foreign/libnano-test-sync-fault.dylib';},
  r=>{r.loadCommand='LC_LOAD_WEAK_DYLIB';},r=>{r.inspectionPassed=1;},
  r=>{r.issuer='FOREIGN';},r=>{r.globalDyldClosureQualified=true;},
  r=>{r.productionQualified=true;},r=>{r.extra=true;}
 ])await beforeLaunchRefusal(f=>{f.updateReceipt(change);});
});

test('post-callback fence refuses changes and preserves a first callback error',async()=>{
 const f=fixture(),guard=f.guard();let callbacks=0;
 await assert.rejects(guard.aroundExporter(async()=>{++callbacks;f.files.get(f.shim.path).material[8]='1700000000123456792';return 'not accepted';}),/original full-nine/);
 assert.equal(callbacks,1);
 const g=fixture(),first=Error('synthetic callback first');
 await assert.rejects(g.guard().aroundExporter(async()=>{g.files.delete(g.shim.path);throw first;}),error=>error===first);
 assert.match(first.nativeLoaderSecondaryFailure,/synthetic missing file/);
 const h=fixture(),frozen=Object.freeze(Error('frozen first'));
 await assert.rejects(h.guard().aroundExporter(async()=>{h.files.delete(h.shim.path);throw frozen;}),error=>error===frozen);
});

test('legacy generation retains its opaque reader; unknown generation does not downgrade',()=>{
 let reads=0;const readArtifact=()=>{++reads;throw Error('must not read');};
 assert.equal(selectNativeExporterLoaderGuard({format:'nano-fresh-capella-three-owner-campaign-v1'},Buffer.from('old opaque bytes'),{readArtifact}),null);
 assert.equal(reads,0);
 assert.throws(()=>selectNativeExporterLoaderGuard({format:'FOREIGN'},Buffer.from('{}'),{readArtifact}),/supported campaign generation/);
 assert.throws(()=>selectNativeExporterLoaderGuard(fixture().spec,Buffer.from('invalid'),{readArtifact}),/C8 overlay JSON/);
 assert.equal(reads,0);
});
