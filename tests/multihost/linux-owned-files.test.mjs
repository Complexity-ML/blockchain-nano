// Pure changed-contract tests. Fake Docker callbacks establish only order and
// refusals; they cannot qualify a filesystem, a node, private transfer or chain.
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {canonical,knownFrame,transportReceipt,macDockerCopier,linuxInventory,linuxBootstrapProof,createMacLinuxFiles,linuxSourceFiles} from './linux-owned-files.mjs';
const sha=b=>createHash('sha256').update(b).digest('hex'),body=v=>Buffer.from(canonical(v));
const material=['1','2','8180','1','501','20','10','2026-10-15 12:00:00.000000000 +0000','2026-10-15 12:00:00.000000000 +0000'];
function receipt(mode,count,total,hash='1'.repeat(64)){
 return {format:'nano-linux-known-files-result-v1',mode,passed:true,filesWritten:mode==='write'?count:0,filesVerified:count,payloadBytes:total,knownBytesMatched:true,fullNineFieldPinsRetained:true,privateModesAndOwnerVerified:true,rootFdIdentityRetained:true,existingFilesMutated:false,namespaceInventoryQualified:false,authorityName:'.known-files-'+hash+'.authority.private.json',authoritySha256:'2'.repeat(64),authorityBytes:100,authorityMaterial:[...material],stabilitySamples:3,stabilitySpanMs:2000,productionQualified:false};
}
test('known frame is ASCII canonical and hashes bytes before output',()=>{
 const b=Buffer.from([0,255,13,10]),r=knownFrame([{name:'b.json',bytes:b}]);assert.equal(r.frame.toString(),'{"files":[{"bytes":4,"hex":"00ff0d0a","name":"b.json","sha256":"'+sha(b)+'"}],"format":"nano-linux-known-files-v1"}');assert.equal(r.sha256,sha(r.frame));
 for(const name of ['../x','x/y','.','..','.known-files-bad','é.json'])assert.throws(()=>knownFrame([{name,bytes:b}]));assert.throws(()=>knownFrame([{name:'x',bytes:b},{name:'x',bytes:b}]));
});
test('nineteen-field receipt keeps exact counts/full9/no LF and false scope',()=>{
 const r=receipt('write',2,40);assert.equal(Object.keys(r).length,19);assert.equal(transportReceipt(r,'write',2,40),r);
 for(const edit of [x=>x.extra=true,x=>x.authorityMaterial[8]+='\n',x=>x.authorityMaterial[8]+='\r',x=>x.namespaceInventoryQualified=true,x=>x.filesWritten=0,x=>x.stabilitySpanMs=1999]){const bad=structuredClone(r);edit(bad);assert.throws(()=>transportReceipt(bad,'write',2,40));}
 assert.doesNotThrow(()=>transportReceipt(receipt('verify',2,40),'verify',2,40));
});
test('copier successor changes exactly owner and assigned indices without broadening',async()=>{
 const original=await readFile(new URL('../../'+linuxSourceFiles.copier.path,import.meta.url));assert.equal(original.length,10433);const next=macDockerCopier(original).toString();assert(!next.includes('mac-native'));assert.equal((next.match(/mac-docker/g)||[]).length,4);assert(next.includes('my $i=16+$offset;my $p=$spec->{pairs}[$offset]'));assert(next.includes('@{$spec->{pairs}}==16'));assert(next.includes('O_EXCL'));assert(next.includes('same($old[$i],scan($sources[$i],0,{}))'));assert.throws(()=>macDockerCopier(Buffer.concat([original,Buffer.from('\n')])));
});
function bootstrapFixture(){
 const campaignId='a'.repeat(64),keys=Array.from({length:65},(_,i)=>({index:i,publicKey:'0x'+i.toString(16).padStart(96,'0')})),journals=keys.map((x,i)=>({validatorIndex:i,publicKey:x.publicKey,primary:'/native/import/primary-'+i,witness:'/native/import/witness-'+i,primaryCommitment:{sequence:'0x1',digest:'0x'+sha(Buffer.from('p'+i))},witnessCommitment:{sequence:'0x1',digest:'0x'+sha(Buffer.from('w'+i))},proposalSlot:null,attestationTarget:null}));
 const nativeBindings={format:'nano-isolated-bootstrap-journal-bindings-v1',campaignId,journals},linuxBindings=structuredClone(nativeBindings);for(let i=0;i<65;i++){linuxBindings.journals[i].primary='/bootstrap/import/primary-'+i;linuxBindings.journals[i].witness='/bootstrap/import/witness-'+i;}
 const linuxBindingsBytes=body(linuxBindings),nativeAudit={format:'nano-isolated-fixture-private-binding-audit-v1',passed:true,campaignId,inputManifestSha256:'b'.repeat(64),journalBindingsSha256:sha(body(nativeBindings)),newSignaturesCreated:0,runtimeStarted:false},linuxAudit={...nativeAudit,journalBindingsSha256:sha(linuxBindingsBytes)},nativeInterchange={metadata:{genesis_validators_root:'0x'+'c'.repeat(64),interchange_format_version:'5'},data:[]};
 const plan={campaignNamespace:'isolated-capella-'+campaignId,inputEvidencePins:{manifest:'b'.repeat(64)},planSha256:'d'.repeat(64),owners:[{owner:'mac-native',keys:[]},{owner:'mac-docker',keys:journals.slice(16,32).map(j=>({publicKeyIndex:j.validatorIndex,publicKey:j.publicKey,runtimeKey:{restart:j.primaryCommitment},expectedWitnessCommitment:j.witnessCommitment}))}]},sourceInventory={format:'nano-isolated-bootstrap-file-inventory-v1',root:'/bootstrap/import',rootUid:501,rootGid:20,rootMode:448};
 return {manifest:{campaignId,publicKeys:keys},plan,planBytes:body(plan),nativeAudit,linuxAudit,nativeBindings,linuxBindings,linuxBindingsBytes,nativeInterchange,linuxInterchange:structuredClone(nativeInterchange),sourceInventory,sourceInventoryBytes:body(sourceInventory)};
}
test('physical Linux anchors bind unchanged native plan after all 65 correspondences',()=>{
 const f=bootstrapFixture(),old=sha(f.planBytes),r=linuxBootstrapProof(f),a=JSON.parse(r.anchors),spec=JSON.parse(r.copySpec);assert.equal(sha(f.planBytes),old);assert.equal(a.planDocumentSha256,old);assert.equal(a.sourceInventorySha256,sha(f.sourceInventoryBytes));assert.equal(spec.pairs.length,16);for(let o=0;o<16;o++){const i=16+o;assert.equal(spec.pairs[o].sourcePrimary,'/bootstrap/import/primary-'+i);assert(spec.pairs[o].targetPrimary.endsWith('/mac-docker/journal-'+i));}
});
test('bootstrap correspondence refuses old-source rebasing, token/history/report drift',()=>{
 for(const edit of [f=>f.linuxBindings.journals[64].primary='/native/import/primary-64',f=>f.linuxBindings.journals[0].primaryCommitment.sequence='0x2',f=>f.linuxBindings.journals[20].publicKey='0x'+'0'.repeat(96),f=>f.linuxAudit.newSignaturesCreated=1,f=>f.linuxInterchange.data.push({forged:true}),f=>f.sourceInventory.rootUid=1000]){const f=bootstrapFixture();edit(f);if(f.linuxBindings){f.linuxBindingsBytes=body(f.linuxBindings);f.linuxAudit.journalBindingsSha256=sha(f.linuxBindingsBytes);}f.sourceInventoryBytes=body(f.sourceInventory);assert.throws(()=>linuxBootstrapProof(f));}
 const f=bootstrapFixture();f.linuxAudit.journalBindingsSha256='f'.repeat(64);assert.throws(()=>linuxBootstrapProof(f));
});
test('inventory is owner-local, bounded and rejects aliases before metadata read',()=>{
 const x={format:'nano-isolated-bootstrap-file-inventory-v1',root:'/bootstrap/import',rootDevice:'1',rootInode:'2',rootUid:501,rootGid:20,rootMode:448,rootMtime:'1:0',rootCtime:'1:0',entries:[{path:'journal-bindings.json',kind:'file',device:'1',inode:'3',uid:501,gid:20,mode:384,bytes:5,mtime:'1:0',ctime:'1:0',sha256:'a'.repeat(64)}]};assert.equal(linuxInventory(body(x),x.root).entries.length,1);
 for(const edit of [v=>v.rootUid=1000,v=>v.entries.push({...v.entries[0],path:'other'}),v=>v.entries[0].path='../private',v=>v.entries[0].bytes=536870913,v=>v.entries[0].kind='symlink',v=>v.entries[0].mode=438]){const y=structuredClone(x);edit(y);assert.throws(()=>linuxInventory(body(y),x.root));}
});
test('fake callbacks enforce new Linux volumes, immutable generations and original authority',async()=>{
 const helper=await readFile(new URL('../../'+linuxSourceFiles.helper.path,import.meta.url)),copier=await readFile(new URL('../../'+linuxSourceFiles.copier.path,import.meta.url)),uuid='12345678-1234-1234-1234-123456789abc',calls=[],volumes=new Set(),writes=new Map();let drift=false,helperChecks=0;
 const docker=async(args,remote=false,timeout=0,input=null)=>{calls.push({args:[...args],remote,timeout,input});assert.equal(remote,false);if(args[0]==='ps')return '';if(args[0]==='volume'){if(args[1]==='ls')return '';if(args[1]==='create'){volumes.add(args.at(-1));return args.at(-1);}if(args[1]==='inspect')return JSON.stringify({Name:args[2],Labels:{'nano.fresh.campaign':uuid}});}
  assert.equal(args[0],'run');assert.equal(args.includes('-i'),input!==null);assert(args.includes('umask 077; exec "$@"'));const at=args.indexOf('--'),argv=args.slice(at+1);if(argv[0]==='/lib/ld-linux-aarch64.so.1')return 'known loader listing';if(argv[0]==='perl'&&argv[1]==='/core/known-files.pl'){
   const mode=argv[2];if(mode==='write'){const frame=JSON.parse(input),total=frame.files.reduce((n,x)=>n+x.bytes,0),r=receipt(mode,frame.files.length,total,sha(input));writes.set(r.authorityName,structuredClone(r));return JSON.stringify(r);}if(mode==='verify'){const r=structuredClone(writes.get(argv[6]));assert.deepEqual(JSON.parse(input).authorityMaterial,r.authorityMaterial);r.mode='verify';r.filesWritten=0;if(drift)r.authorityMaterial[8]+=' changed';return JSON.stringify(r);}}
  return '';
 };
 const files=createMacLinuxFiles({docker,image:'sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb',uuid,prefix:'test-'+uuid,softwareMounts:()=>[],helperPath:'/public/known-files.pl',helperBytes:helper,copierBytes:copier,save:async()=>{},verifyHelper:async()=>{++helperChecks;}});
 await files.preflight('/public/nano-capella-isolated-bootstrap-import');const docs=['isolated-domain.json','genesis.json','profile.json','initial.json','genesis-validators.ssz','genesis-state.ssz'].map(name=>({name,bytes:Buffer.from(name)}));
 const a=await files.generation(docs,Buffer.from('{"generation":1}')),b=await files.generation(docs,Buffer.from('{"generation":2}'));assert.notEqual(a.volume,b.volume);assert.equal(a.node.toString(),'{"generation":1}');assert(a.mounts[1].endsWith(',readonly'));await files.verifyAll();assert.equal(files.authorityCount,2);assert.equal(helperChecks,2*calls.filter(x=>x.args[0]==='run').length);assert(calls.every(x=>!x.args.some(a=>a.includes('src=/private/'))));drift=true;await assert.rejects(files.verifyAll(),/original authority differs/);
 assert.throws(()=>files.mount('not-owned','/proof'));const refused=createMacLinuxFiles;assert.throws(()=>refused({docker,image:'other',uuid,prefix:'x',softwareMounts:()=>[],helperPath:'x',helperBytes:helper,copierBytes:copier,save:async()=>{}}));
});
