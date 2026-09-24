import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {audit,compareCases,parseReport,digest} from './audit-evidence.mjs';
const row=(status,variant=0)=>({case:'fixture[fork_Prague-state_test]',fork:'Prague',status,variant});
function report(rows){return rows.map(r=>JSON.stringify(r)).join('\n')+'\n'+JSON.stringify({summary:{matched:rows.filter(r=>r.status==='matched').length,mismatched:rows.filter(r=>r.status==='mismatch').length,unsupported:0,errors:0}})+'\n';}
test('a green but unrelated variant does not resolve a failure',()=>{
 const comparison=compareCases(parseReport(report([row('mismatch',1)])),parseReport(report([row('matched',0)])));
 assert.equal(comparison.coverageVerified,false);assert.equal(comparison.unresolved[0].after,'missing');
});
test('same-case correction is recognized without claiming a new execution',()=>{
 const comparison=compareCases(parseReport(report([row('mismatch')])),parseReport(report([row('matched')])));
 assert.equal(comparison.coverageVerified,true);assert.equal(comparison.matchedAfter,1);
});
test('duplicate cases cannot inflate coverage',()=>{
 const duplicate=parseReport(report([row('matched'),row('matched')]));
 assert(duplicate.issues.some(i=>i.startsWith('duplicate case identity')));
 assert.equal(compareCases(parseReport(report([row('mismatch')])),duplicate).coverageVerified,false);
});
test('truncated report and fabricated summary remain incomplete',()=>{
 assert(parseReport(JSON.stringify(row('matched'))+'\n{"summary":').issues.includes('missing summary'));
 const fake=report([row('matched')]).replace('"matched":1','"matched":2');
 assert(parseReport(fake).issues.some(i=>i.startsWith('summary matched')));
});
test('changed bytes invalidate an expected report hash',async()=>{
 const root=await mkdtemp(join(tmpdir(),'nano-evidence-'));
 try{
  await mkdir(join(root,'tests/fixtures'),{recursive:true});await mkdir(join(root,'build-cmake/qualification'),{recursive:true});
  const relative='build-cmake/qualification/run.jsonl',file=join(root,relative);
  await writeFile(file,report([row('matched')]));const hash=await digest(file);
  await writeFile(join(root,'tests/fixtures/ethereum-official.json'),JSON.stringify({proof:{report:relative,reportSha256:hash}}));
  assert.equal((await audit(root)).references[0].status,'verified');
  await writeFile(file,report([row('mismatch')]));assert.equal((await audit(root)).references[0].status,'hash-mismatch');
 }finally{await rm(root,{recursive:true,force:true});}
});

test('a matched label cannot hide a wrong rejection or state mutation',()=>{
 const good={...row('matched'),validation:{actual:'A',expected:'A|B',stateUnchanged:true,transactionExecuted:false}};
 assert.equal(parseReport(report([good])).issues.length,0);
 for(const validation of [{...good.validation,actual:'C'},{...good.validation,stateUnchanged:false},{...good.validation,transactionExecuted:true}]){
  assert(parseReport(report([{...good,validation}])).issues.some(i=>i.includes('inconsistent matched record')));
 }
 const badRoot={...row('matched'),stateRoot:{actual:'0xaa',expected:'0xbb'}};
 assert(parseReport(report([badRoot])).issues.some(i=>i.includes('stateRoot')));
});

test('fork inference uses the parameter, not fork_transition in a test path',()=>{
 const name='tests/test_fork_transition.py::test_fork_transition[fork_CancunToPragueAtTime15k-blockchain_test]';
 const old={case:name,status:'mismatch'};
 const current={case:name,fork:'CancunToPragueAtTime15k',status:'matched'};
 assert.equal(compareCases(parseReport(report([old])),parseReport(report([current]))).coverageVerified,true);
 const wrong={...current,fork:'ShanghaiToCancunAtTime15k'};
 assert.equal(compareCases(parseReport(report([old])),parseReport(report([wrong]))).coverageVerified,false);
 const noParameter=parseReport(report([{case:'test_fork_transition.py::test_plain',status:'matched'}]));
 assert.equal(JSON.parse([...noParameter.index.keys()][0])[1],'');
});

test("a matched label cannot hide a failed gas or transaction hash check",()=>{
 for(const field of ["gasUsedMatches","transactionHashMatches"])assert(parseReport(report([{...row("matched"),[field]:false}])).issues.some(i=>i.includes(field)));
});

async function withArtifactIndex(action){
 const root=await mkdtemp(join(tmpdir(),'nano-index-'));
 try{
  await mkdir(join(root,'campaign'));const path='campaign/result.json';await writeFile(join(root,path),'original result');
  const entries=[{path,sha256:await digest(join(root,path))}];
  const save=async rows=>{await writeFile(join(root,'campaign/index.json'),JSON.stringify(rows));return {path:'campaign/index.json',sha256:await digest(join(root,'campaign/index.json'))};};
  await action({root,entries,save,descriptor:await save(entries)});
 }finally{await rm(root,{recursive:true,force:true});}
}
const {verifyArtifactIndex}=await import('./audit-evidence.mjs');
test('artifact index validates immutable campaign files',()=>withArtifactIndex(async({root,descriptor})=>{
 const result=await verifyArtifactIndex(root,descriptor);assert.equal(result.status,'verified');assert.equal(result.checked,1);
}));
test('changed artifact invalidates otherwise intact index',()=>withArtifactIndex(async({root,descriptor})=>{
 await writeFile(join(root,'campaign/result.json'),'altered result');const result=await verifyArtifactIndex(root,descriptor);assert.equal(result.status,'invalid-artifacts');assert(result.issues.some(i=>i.error==='artifact hash mismatch'));
}));
test('missing artifact is a failure',()=>withArtifactIndex(async({root,descriptor})=>{
 await rm(join(root,'campaign/result.json'));assert.equal((await verifyArtifactIndex(root,descriptor)).status,'invalid-artifacts');
}));
test('altered index is not trusted',()=>withArtifactIndex(async({root,descriptor})=>{
 await writeFile(join(root,'campaign/index.json'),'[]');assert.equal((await verifyArtifactIndex(root,descriptor)).status,'hash-mismatch');
}));
test('duplicate entries and unindexed outputs are failures',()=>withArtifactIndex(async({root,entries,save})=>{
 const duplicate=await verifyArtifactIndex(root,await save([...entries,...entries]));assert(duplicate.issues.some(i=>i.error==='duplicate artifact'));
 await writeFile(join(root,'campaign/extra.json'),'unlisted');const extra=await verifyArtifactIndex(root,await save(entries));assert(extra.issues.some(i=>i.error==='unindexed artifact'));
}));
test('index cannot escape its campaign or repository',()=>withArtifactIndex(async({root,entries,save,descriptor})=>{
 const escaped=await verifyArtifactIndex(root,await save([{...entries[0],path:'../outside.json'}]));assert(escaped.issues.some(i=>i.error==='artifact outside campaign or index includes itself'));
 assert.equal((await verifyArtifactIndex(root,{...descriptor,path:'../outside-index.json'})).status,'invalid-index');
}));
test('manifest audit includes nested artifact indexes and detects tampering',()=>withArtifactIndex(async({root,descriptor})=>{
 await mkdir(join(root,'tests/fixtures'),{recursive:true});await mkdir(join(root,'build-cmake/qualification'),{recursive:true});
 await writeFile(join(root,'tests/fixtures/ethereum-official.json'),JSON.stringify({campaign:{artifactIndex:descriptor}}));
 const before=await audit(root);assert.equal(before.artifactIndices.length,1);assert.equal(before.artifactIndices[0].status,'verified');
 await writeFile(join(root,'campaign/result.json'),'corrupted');const after=await audit(root);assert.equal(after.artifactIndices[0].status,'invalid-artifacts');assert.equal(after.complete,false);
}));
