import test from 'node:test';import assert from 'node:assert/strict';
import {selectExitArtifacts,verifyExitExecutableHashes,verifyExitVpsArtifacts,verifyExitCoreCompatibility} from './exit-artifacts.mjs';
const hash='a'.repeat(64),revision='b'.repeat(40),names=['nano','p2p','exporter'];
function fixture(){return {runtimeRevision:revision,native:Object.fromEntries(names.map(name=>[name,{path:'/private/tmp/current/'+name,sha256:hash}])),vps:{image:'sha256:'+hash,volume:'qualified-build',directory:'/build/current',executables:Object.fromEntries(names.map(name=>[name,{path:'/build/current/'+name,sha256:hash}]))}};}
const wire=s=>Object.values(s).map(x=>x.sha256+'  '+x.path).join('\n')+'\n';
test('explicit immutable artifacts produce a read-only mount and bound revision',()=>{const a=selectExitArtifacts(fixture());assert.equal(a.mount,'type=volume,src=qualified-build,dst=/build,readonly');assert.equal(a.revisionFile,'/build/current/production-source-revision.txt');assert.equal(verifyExitVpsArtifacts(a,{Id:a.vps.image,Architecture:'amd64'},revision+'\n',wire(a.vps.executables))['/build/current/nano'],hash);});
test('missing, mutable and unknown provenance fail before fixtures',()=>{for(const change of [x=>delete x.native.exporter,x=>x.vps.image='nano-build-dependencies:local',x=>x.runtimeRevision='main',x=>x.vps.extra=true,x=>x.native.nano.sha256='bad']){const f=fixture();change(f);assert.throws(()=>selectExitArtifacts(f));}});
test('unsafe or duplicate paths and remote directory escapes fail',()=>{for(const change of [x=>x.native.nano.path='/tmp/../nano',x=>x.native.nano.path='/tmp/nano;rm',x=>x.native.p2p.path=x.native.nano.path,x=>x.vps.executables.nano.path='/build/old/nano',x=>x.vps.directory='/build',x=>x.vps.volume='bad,readonly=false']){const f=fixture();change(f);assert.throws(()=>selectExitArtifacts(f));}});
test('hash verification rejects changed, missing, duplicate and extra programs',()=>{const a=selectExitArtifacts(fixture()),w=wire(a.native);for(const text of [w.replace(hash,'c'.repeat(64)),w.split('\n').slice(0,2).join('\n'),w+w.split('\n')[0]+'\n',w+hash+'  /other\n'])assert.throws(()=>verifyExitExecutableHashes(a.native,text));});
test('image, architecture and runtime marker are independently checked',()=>{const a=selectExitArtifacts(fixture());for(const [info,rev] of [[{Id:'sha256:'+'c'.repeat(64),Architecture:'amd64'},revision],[{Id:a.vps.image,Architecture:'arm64'},revision],[{Id:a.vps.image,Architecture:'amd64'},'d'.repeat(40)]])assert.throws(()=>verifyExitVpsArtifacts(a,info,rev,wire(a.vps.executables)));});

test('explicit new-relay composition retains exact stopped core and exporter',()=>{
 const source=fixture(),next=fixture();next.coreRevision=revision;next.runtimeRevision='c'.repeat(40);next.native.p2p.sha256='d'.repeat(64);next.vps.executables.p2p.sha256='d'.repeat(64);
 assert.equal(verifyExitCoreCompatibility(selectExitArtifacts(next),source),revision);
 for(const change of [x=>x.coreRevision='e'.repeat(40),x=>x.native.nano.sha256='e'.repeat(64),x=>x.vps.executables.exporter.sha256='e'.repeat(64)]){const bad=structuredClone(next);change(bad);assert.throws(()=>verifyExitCoreCompatibility(selectExitArtifacts(bad),source));}
 for(const invalid of ['main',null,'a'.repeat(39)]){const bad=structuredClone(next);bad.coreRevision=invalid;assert.throws(()=>selectExitArtifacts(bad));}
 assert.equal(verifyExitCoreCompatibility(selectExitArtifacts(source),source),revision);
});
