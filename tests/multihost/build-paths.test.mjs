import {test} from 'node:test';
import assert from 'node:assert/strict';
import {selectBuildPaths,selectHostBuildPaths,selectRemoteSourceRepository,remoteDockerArgument,selectNativeBuildPaths} from './build-paths.mjs';
test('legacy read-only volume paths remain available',()=>{
 const p=selectBuildPaths(true,undefined);
 assert.equal(p.nano,'/build/nano');assert.equal(p.revision,'/build/production-source-revision.txt');
});
test('isolated runtime, provenance and rotation auditor share the selected build',()=>{
 const p=selectBuildPaths(true,'production-evm-c10426b2-20261001');
 assert.equal(p.nano,'/build/production-evm-c10426b2-20261001/nano');
 assert.equal(p.revision,p.directory+'/production-source-revision.txt');
 assert.equal(p.rotationAudit,p.directory+'/nano-capella-rotation-audit');
 assert.equal(p.p2p,'/build/network/nano-p2p');
});
test('packaged/default mode cannot silently ignore a requested build',()=>{
 assert.throws(()=>selectBuildPaths(false,'qualified'),/requires build-volume/);
 assert.doesNotThrow(()=>selectBuildPaths(false,undefined));
});
test('escape, whitespace, empty and multiline build selectors are rejected',()=>{
 for(const value of ['', '..', '.hidden', '-option','../old','a/../../old','/build','a\nb','a\rb','a\n','a\r',null,123,'a b','a;id','a$HOME'])assert.throws(()=>selectBuildPaths(true,value),/Invalid/);
});

test('architecture-specific paths select separate existing builds',()=>{
 const p=selectHostBuildPaths(true,undefined,'coherent-arm','coherent-amd');
 assert.equal(p.arm64.nano,'/build/coherent-arm/nano');assert.equal(p.amd64.nano,'/build/coherent-amd/nano');
 assert.equal(p.arm64.p2p,p.amd64.p2p);
});
test('common selector remains backward compatible and supports explicit override',()=>{
 const p=selectHostBuildPaths(true,'common',undefined,'amd');
 assert.equal(p.arm64.directory,'/build/common');assert.equal(p.amd64.directory,'/build/amd');
 assert.deepEqual(selectHostBuildPaths(true,undefined,undefined,undefined).arm64,selectBuildPaths(true,undefined));
});
test('one missing architecture without fallback cannot silently use the legacy binary',()=>{
 for(const pair of [['arm',undefined],[undefined,'amd']])assert.throws(()=>selectHostBuildPaths(true,undefined,...pair),/Both architecture/);
});
test('per-architecture selectors preserve packaged and path validation',()=>{
 assert.throws(()=>selectHostBuildPaths(false,undefined,'arm','amd'),/requires build-volume/);
 for(const bad of ['../other','a\nb',''])assert.throws(()=>selectHostBuildPaths(true,undefined,'arm',bad),/Invalid/);
});
test('remote source can select a Git candidate without changing the main checkout',()=>{
 assert.equal(selectRemoteSourceRepository('ubuntu'),'/home/ubuntu/blockchain-nano-git');
 assert.equal(selectRemoteSourceRepository('ubuntu','/home/ubuntu/candidate'),'/home/ubuntu/candidate');
});
test('remote source rejects traversal, relative paths and mount injection',()=>{
 for(const bad of ['relative','/','/tmp/../old','/tmp/./old','/tmp//old','/tmp/a,b','/tmp/a\nb','/tmp/a b',null,123])assert.throws(()=>selectRemoteSourceRepository('ubuntu',bad),/Invalid/);
});

test('remote Docker invocations translate provenance, native audit, restore and rotation executables',()=>{
 const p=selectHostBuildPaths(true,undefined,'arm','amd');
 const translate=a=>remoteDockerArgument(a,'local-image','remote-image',true,p);
 for(const key of ['nano','revision','rotationAudit'])assert.equal(translate(p.arm64[key]),p.amd64[key]);
 assert.equal(translate('local-image'),'remote-image');assert.equal(translate(p.arm64.p2p),p.arm64.p2p);
 assert.deepEqual(['--entrypoint',p.arm64.nano,'local-image','audit-capella'].map(translate),['--entrypoint',p.amd64.nano,'remote-image','audit-capella']);
 assert.deepEqual(['bash','/work/tests/multihost/backup-restore.sh',p.arm64.nano].map(translate),['bash','/work/tests/multihost/backup-restore.sh',p.amd64.nano]);
 assert.equal(remoteDockerArgument(p.arm64.nano,'local-image','remote-image',false,p),p.arm64.nano);
});

test('coherent campaign selects distinct daemon paths and translates remote argv',()=>{
 const p=selectHostBuildPaths(true,undefined,'current-arm','current-amd',true);
 assert.equal(p.arm64.p2p,'/build/current-arm/network/nano-p2p');
 assert.equal(p.amd64.p2p,'/build/current-amd/network/nano-p2p');
 const argv=['sha256sum',p.arm64.nano,p.arm64.p2p].map(a=>remoteDockerArgument(a,'arm-image','amd-image',true,p));
 assert.deepEqual(argv,['sha256sum',p.amd64.nano,p.amd64.p2p]);
});
test('coherent network cannot be requested in packaged mode or with an ambiguous selector',()=>{
 assert.throws(()=>selectBuildPaths(false,undefined,true),/requires build-volume/);
 for(const bad of ['1',1,null,{},[]])assert.throws(()=>selectHostBuildPaths(true,'current',undefined,undefined,bad),/Invalid/);
});
test('explicit native core and network selections keep independently qualified binaries',()=>{
 const p=selectNativeBuildPaths('/repo','/tmp/native-core','/tmp/current-network');
 assert.equal(p.nano,'/tmp/native-core/nano');assert.equal(p.p2p,'/tmp/current-network/nano-p2p');
 assert.equal(p.transitionTests.capella,'/tmp/native-core/nano-capella-transition-tests');
 assert.equal(p.rotationAudit,'/tmp/native-core/nano-capella-rotation-audit');
 const legacy=selectNativeBuildPaths('/repo');assert.equal(legacy.nano,'/repo/build-release/nano');assert.equal(legacy.p2p,'/repo/build-network/nano-p2p');
});
test('native selectors reject traversal, whitespace and argument injection',()=>{
 for(const bad of ['relative','/','/tmp/../old','/tmp/./old','/tmp//old','/tmp/a,b','/tmp/a b','/tmp/a\nb',null,123]){
  assert.throws(()=>selectNativeBuildPaths('/repo',bad,'/tmp/network'),/Invalid/);
  assert.throws(()=>selectNativeBuildPaths('/repo','/tmp/core',bad),/Invalid/);
 }
});
