// Pure contract tests only: no filesystem, private inputs, Docker, DB or SSH.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {selectVpsLinuxJournals,verifiedVpsLinuxFiles,knownUstar,buildVpsLinuxTransport,exportVpsLinuxBootstrap} from './vps-linux-bootstrap.mjs';
const sha=b=>createHash('sha256').update(b).digest('hex'),encode=v=>Buffer.from(JSON.stringify(v));
function fixture(){
 const campaignId='a'.repeat(64),namespace='isolated-capella-'+campaignId,root={format:'nano-isolated-bootstrap-file-inventory-v1',root:'/bootstrap/import',rootDevice:'1',rootInode:'2',rootUid:501,rootGid:20,rootMode:448,rootMtime:'10:0',rootCtime:'10:0'},entries=[],payload=new Map();let ino=10;
 for(let i=0;i<65;i++)for(const role of ['primary','witness']){const name=role+'-'+i,b=Buffer.from('synthetic '+name);entries.push({path:name,kind:'directory',device:'1',inode:String(++ino),uid:501,gid:20,mode:448,bytes:100,mtime:'10:0',ctime:'10:0',sha256:null},{path:name+'/CURRENT',kind:'file',device:'1',inode:String(++ino),uid:501,gid:20,mode:384,bytes:b.length,mtime:'10:0',ctime:'10:0',sha256:sha(b)});payload.set('/bootstrap/import/'+name+'/CURRENT',b);}
 const empty=Buffer.alloc(0);entries.push({path:'primary-64/LOCK',kind:'file',device:'1',inode:String(++ino),uid:501,gid:20,mode:384,bytes:0,mtime:'10:0',ctime:'10:0',sha256:sha(empty)});payload.set('/bootstrap/import/primary-64/LOCK',empty);
 const inventoryBytes=encode({...root,entries}),wholeInventoryBytes=encode({...root,root:'/bootstrap',rootInode:'3',entries:[]}),selection=selectVpsLinuxJournals(inventoryBytes,wholeInventoryBytes,namespace),manifestBytes=encode({campaignId}),keys=Array.from({length:33},(_,o)=>({publicKeyIndex:32+o,runtimeKey:{restart:{sequence:'0x1',digest:'0x'+'b'.repeat(64)}},expectedWitnessCommitment:{sequence:'0x1',digest:'0x'+'c'.repeat(64)},importedHistory:{proposalSlot:null}})),planBytes=encode({campaignId,campaignNamespace:namespace,inputEvidencePins:{manifest:sha(manifestBytes)},planSha256:'d'.repeat(64),owners:[{owner:'mac-native'},{owner:'mac-docker'},{owner:'vps-docker',keys}]}),readerBytes=encode({format:'nano-vps-linux-journal-read-result-v1',files:selection.rows.filter(x=>x.entry.kind==='file').map(x=>({path:x.archivePath,hex:payload.get(x.sourcePath).toString('hex')}))}),configuration=['genesis.json','profile.json','initial.json','node.json'].map(name=>({name,bytes:Buffer.from('{}')}));configuration.push({name:'isolated-domain.json',bytes:manifestBytes},{name:'fixture.env',bytes:Buffer.from(keys.map(k=>'NANO_TEST_CAPELLA_NETWORK_KEY_'+k.publicKeyIndex+'=0x'+'1'.repeat(64)).join('\n')+'\n')});
 return {selection,readerBytes,configuration,planBytes,inventoryBytes,wholeInventoryBytes,namespace};
}
// Independent minimal reader of the standard archive wire format, used to
// inspect paths, flags and payloads rather than the producer's own objects.
function untar(bytes){
 const out=new Map();let offset=0;const text=b=>b.subarray(0,b.indexOf(0)<0?b.length:b.indexOf(0)).toString(),oct=b=>Number.parseInt(b.toString().replace(/[\0 ]+$/,''),8);
 for(;offset+512<=bytes.length;){const h=bytes.subarray(offset,offset+512);if(h.every(x=>x===0)){assert(bytes.subarray(offset).every(x=>x===0));assert(bytes.length-offset>=1024);return out;}assert.equal(h.subarray(257,265).toString(),'ustar\x0000');const prefix=text(h.subarray(345,500)),name=(prefix?prefix+'/':'')+text(h.subarray(0,100)),size=oct(h.subarray(124,136));assert(!out.has(name));assert(['0','5'].includes(String.fromCharCode(h[156])));const header=Buffer.from(h);header.fill(32,148,156);assert.equal(oct(h.subarray(148,156)),header.reduce((a,b)=>a+b,0));offset+=512;out.set(name,{kind:String.fromCharCode(h[156]),mode:oct(h.subarray(100,108)),bytes:bytes.subarray(offset,offset+size)});offset+=Math.ceil(size/512)*512;}
 assert.fail('Missing archive end marker');
}
test('VPS transport contains exactly33 Linux pairs and no Native database or Mac environment',()=>{
 const f=fixture(),before=sha(f.inventoryBytes),bundle=buildVpsLinuxTransport(f),config=untar(bundle.archives[0].bytes),proof=untar(bundle.archives[1].bytes);assert.equal(f.selection.rows.length,133);assert.equal(bundle.publicScope.sourceFiles,67);assert.equal(bundle.publicScope.nativeSstFilesTransferred,false);assert.equal(bundle.publicScope.macOwnerPrivateKeysTransferred,false);
 const journalFiles=[...config].filter(([p,x])=>x.kind==='0'&&(p.startsWith('data/')||p.startsWith('witness/')));assert.equal(journalFiles.length,67);for(let i=32;i<65;i++)for(const [root,leaf]of [['data','journal'],['witness','witness']])assert(config.has(root+'/'+f.namespace+'/vps-docker/'+leaf+'-'+i+'/CURRENT'));assert(![...config.keys()].some(p=>p.includes('mac-native')||p.includes('mac-docker')));assert([...config.values()].every(x=>x.mode===(x.kind==='5'?448:384)));
 const retained=proof.get('source-inventory.json').bytes;assert(retained.equals(f.inventoryBytes));assert.equal(sha(retained),before);assert(proof.get('plan.json').bytes.equals(f.planBytes));const anchors=JSON.parse(proof.get('anchors.json').bytes);assert.equal(anchors.sourceInventorySha256,before);assert.equal(bundle.expected.sourceInventorySha256,before);assert.equal(bundle.expected.keys.length,33);
});
test('known source corruption, aliases, incomplete33pairs and widened private configuration refuse',()=>{
 for(const edit of [x=>x.entries.find(z=>z.path==='primary-64').path='primary-63',x=>x.entries.find(z=>z.path==='witness-32/CURRENT').sha256='bad',x=>x.entries.find(z=>z.path==='primary-32/CURRENT').inode=x.entries[0].inode,x=>x.entries.find(z=>z.path==='primary-32').kind='symlink',x=>x.entries.find(z=>z.path==='primary-32/CURRENT').mode=420]){const f=fixture(),inventory=JSON.parse(f.inventoryBytes);edit(inventory);assert.throws(()=>selectVpsLinuxJournals(encode(inventory),f.wholeInventoryBytes,f.namespace));}
 for(const edit of [x=>x.files.pop(),x=>x.files.push(x.files[0]),x=>x.files[0].hex='00'.repeat(x.files[0].hex.length/2),x=>x.files[0].path=x.files[0].path.replace('journal-32','journal-0')]){const f=fixture(),result=JSON.parse(f.readerBytes);edit(result);assert.throws(()=>verifiedVpsLinuxFiles(f.selection,encode(result)));}
 const f=fixture();f.configuration.find(x=>x.name==='fixture.env').bytes=Buffer.from('NANO_TEST_CAPELLA_NETWORK_KEY_0=0x'+'1'.repeat(64)+'\n');assert.throws(()=>buildVpsLinuxTransport(f));assert.throws(()=>knownUstar([{path:'../outside',mode:384,bytes:Buffer.from('x')}]));assert.throws(()=>knownUstar([{path:'x',mode:384,bytes:Buffer.from('x')},{path:'x/y',mode:384,bytes:Buffer.from('y')}]));
});
test('every reader refusal and success fences original Linux sources before any bundle is returned',async()=>{
 for(const mode of ['pass','read-refusal','source-drift','bad-known-bytes']){const f=fixture(),calls=[];let fences=0;const context={...f,fenceSource:async()=>{calls.push('source');if(++fences===2&&mode==='source-drift')throw Error('original source drift');},verifyAll:async()=>calls.push('all'),read:async control=>{calls.push('read');const rows=JSON.parse(control).rows;assert(rows.every(r=>/^\/bootstrap\/import\/(?:primary|witness)-(?:3[2-9]|[45][0-9]|6[0-4])(?:\/|$)/.test(r.sourcePath)));if(mode==='read-refusal')throw Error('FD refusal');if(mode==='bad-known-bytes'){const x=JSON.parse(f.readerBytes);x.files[0].hex='00'.repeat(x.files[0].hex.length/2);return encode(x);}return f.readerBytes;}};
  if(mode==='pass')assert.equal((await exportVpsLinuxBootstrap(context)).expected.keys.length,33);else await assert.rejects(exportVpsLinuxBootstrap(context));assert.deepEqual(calls,['source','all','read','source','all']);
 }
});


// The lifecycle stage reads these exact manifest paths from remoteRoot/config.
// Inspect actual archive bytes, including a large public SSZ kept out of argv.
function lifecycleFixture(){
 const f=fixture(),ssz=[['genesis-state.ssz',Buffer.alloc(200000,0x53)],['genesis-validators.ssz',Buffer.from('PUBLIC opaque validator SSZ')]];f.configuration.push(...ssz.map(([name,bytes])=>({name,bytes})));
 const files=Object.fromEntries([['genesisState','genesis-state.ssz'],['genesisValidators','genesis-validators.ssz'],['genesis','genesis.json'],['profile','profile.json'],['initial','initial.json']].map(([kind,name])=>{const bytes=f.configuration.find(x=>x.name===name).bytes;return [kind,{path:name,sha256:sha(bytes),bytes:bytes.length}];}));
 const manifest=JSON.parse(f.configuration.find(x=>x.name==='isolated-domain.json').bytes);manifest.files=files;rebindManifest(f,manifest);f.lifecycle=true;return f;
}
function rebindManifest(f,manifest){
 const bytes=encode(manifest);f.configuration.find(x=>x.name==='isolated-domain.json').bytes=bytes;const plan=JSON.parse(f.planBytes);plan.inputEvidencePins.manifest=sha(bytes);f.planBytes=encode(plan);
}
test('lifecycle USTAR supplies every manifest path used by remote control staging',()=>{
 const f=lifecycleFixture(),before=f.configuration.map(x=>[x.name,sha(x.bytes)]),bundle=buildVpsLinuxTransport(f),archive=untar(bundle.archives[0].bytes),manifest=JSON.parse(archive.get('isolated-domain.json').bytes);
 const stageNames=['isolated-domain.json',...Object.values(manifest.files).map(x=>x.path),'node.json'];assert.equal(stageNames.length,7);
 for(const name of stageNames){const entry=archive.get(name),known=f.configuration.find(x=>x.name===name);assert(entry,'Actual remote config source for '+name+' must exist');assert.equal(entry.kind,'0');assert.equal(entry.mode,0o600);assert(entry.bytes.equals(known.bytes));}
 for(const pin of Object.values(manifest.files)){const bytes=archive.get(pin.path).bytes;assert.equal(bytes.length,pin.bytes);assert.equal(sha(bytes),pin.sha256);}
 assert.equal(archive.get('genesis-state.ssz').bytes.length,200000);assert.deepEqual(f.configuration.map(x=>[x.name,sha(x.bytes)]),before);assert.equal(bundle.publicScope.macOwnerPrivateKeysTransferred,false);assert.equal(bundle.publicScope.nativeSstFilesTransferred,false);
});
test('lifecycle transport refuses missing, corrupt or misbound stage inputs without widening ordinary transport',()=>{
 for(const name of ['genesis-state.ssz','genesis-validators.ssz']){const f=lifecycleFixture();f.configuration=f.configuration.filter(x=>x.name!==name);assert.throws(()=>buildVpsLinuxTransport(f));}
 const ordinary=fixture();assert.throws(()=>buildVpsLinuxTransport({...ordinary,lifecycle:true}));assert.equal(buildVpsLinuxTransport(ordinary).archives.length,2);
 for(const change of [f=>f.configuration.push(f.configuration[0]),f=>f.lifecycle=false,f=>f.lifecycle='true',f=>f.configuration.find(x=>x.name==='genesis-state.ssz').bytes[0]^=1]){const f=lifecycleFixture();change(f);assert.throws(()=>buildVpsLinuxTransport(f));}
 for(const kind of ['genesisState','genesisValidators','genesis','profile','initial'])for(const change of [p=>p.path='../'+p.path,p=>p.sha256='0'.repeat(64),p=>p.bytes+=1,p=>p.bytes=String(p.bytes)]){const f=lifecycleFixture(),manifest=JSON.parse(f.configuration.find(x=>x.name==='isolated-domain.json').bytes);change(manifest.files[kind]);rebindManifest(f,manifest);assert.throws(()=>buildVpsLinuxTransport(f));}
 const f=lifecycleFixture(),manifest=JSON.parse(f.configuration.find(x=>x.name==='isolated-domain.json').bytes);delete manifest.files.genesisState;rebindManifest(f,manifest);assert.throws(()=>buildVpsLinuxTransport(f));
});
test('lifecycle export forwards explicit mode and retains the original before/after source fences',async()=>{
 const f=lifecycleFixture(),calls=[],bundle=await exportVpsLinuxBootstrap({...f,fenceSource:async()=>calls.push('source'),verifyAll:async()=>calls.push('all'),read:async()=>{calls.push('read');return f.readerBytes;}});assert(untar(bundle.archives[0].bytes).has('genesis-state.ssz'));assert.deepEqual(calls,['source','all','read','source','all']);
 calls.length=0;await assert.rejects(exportVpsLinuxBootstrap({...f,lifecycle:1,fenceSource:async()=>calls.push('source'),verifyAll:async()=>calls.push('all'),read:async()=>calls.push('read')}));assert.deepEqual(calls,[]);
});
