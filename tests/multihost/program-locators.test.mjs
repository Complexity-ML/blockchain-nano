import {test} from 'node:test';
import assert from 'node:assert/strict';
import {freshSoftwareLocators,freshSoftwareFileMounts} from './program-locators.mjs';

test('pure fresh software role locators preserve legacy pins and admit bounded mixed thin layouts',()=>{
 const leaves={nano:'nano',p2p:'nano-p2p',exporter:'nano-capella-transition-tests',bootstrapImport:'nano-capella-isolated-bootstrap-import',bootstrapAudit:'nano-capella-isolated-bootstrap-audit',stoppedCapture:'nano-capella-isolated-stopped-capture',lifecycleTool:'nano-capella-deposit-audit',lifecycleSubmit:'nano-capella-lifecycle-submit'};
 const artifact=path=>({path,sha256:'a'.repeat(64),bytes:17});
 const fixture=native=>({coreBuildRoot:'/home/operator/previous/core',networkBuildRoot:'/home/operator/previous/network',coreArchive:artifact(native?'/home/operator/previous/core/libnano_core.a':'/outputs/libnano_core.a'),programs:Object.fromEntries(Object.entries(leaves).map(([role,leaf])=>[role,artifact((native?(role==='p2p'?'/home/operator/previous/network':'/home/operator/previous/core'):role==='p2p'?'/network-build':'/outputs')+'/'+leaf)]))});
 const native=fixture(true),linux=fixture(false),before=structuredClone({native,linux});
 for(const [name,value]of [['mac-native',native],['mac-docker',linux],['vps-docker',linux]]){
  const rows=freshSoftwareLocators(name,value);assert.equal(rows.length,9);assert(rows.every(row=>row.legacy));assert.deepEqual(freshSoftwareFileMounts(name,value),[]);
  assert.equal(rows.find(x=>x.role==='p2p').hostPath,'/home/operator/previous/network/nano-p2p');assert.equal(rows.find(x=>x.role==='nano').hostPath,'/home/operator/previous/core/nano');
 }
 assert.deepEqual({native,linux},before);
 const selected=['nano','bootstrapAudit','stoppedCapture'],mixedNative=structuredClone(native),mixedLinux=structuredClone(linux);
 for(const value of [mixedNative,mixedLinux]){
  value.coreArchive.path='/home/operator/current-api/libnano_core.a';for(const role of selected)value.programs[role].path='/home/operator/current-api/'+leaves[role];
 }
 const mixedBefore=structuredClone({mixedNative,mixedLinux});
 const nativeRows=freshSoftwareLocators('mac-native',mixedNative),linuxRows=freshSoftwareLocators('mac-docker',mixedLinux);
 assert.equal(nativeRows.filter(x=>!x.legacy).length,4);assert.equal(linuxRows.filter(x=>!x.legacy).length,4);assert.deepEqual(freshSoftwareFileMounts('mac-native',mixedNative),[]);
 const expected=['libnano_core.a',...selected.map(role=>leaves[role])].flatMap(leaf=>['--mount','type=bind,src=/home/operator/current-api/'+leaf+',dst=/home/operator/current-api/'+leaf+',readonly']);
 assert.deepEqual(freshSoftwareFileMounts('mac-docker',mixedLinux),expected);assert.deepEqual(freshSoftwareFileMounts('vps-docker',mixedLinux),expected);
 for(const row of linuxRows){const original=row.role==='coreArchive'?mixedLinux.coreArchive:mixedLinux.programs[row.role];assert.equal(row.runtimePath,original.path);assert.equal(row.sha256,original.sha256);assert.equal(row.bytes,original.bytes);}
 assert.equal(linuxRows.find(x=>x.role==='lifecycleTool').runtimePath,'/outputs/nano-capella-deposit-audit');assert.equal(linuxRows.find(x=>x.role==='p2p').runtimePath,'/network-build/nano-p2p');
 assert.deepEqual({mixedNative,mixedLinux},mixedBefore);
 const nested=structuredClone(linux);nested.programs.nano.path=nested.coreBuildRoot+'/thin/nano';assert.equal(freshSoftwareLocators('mac-docker',nested).find(x=>x.role==='nano').hostPath,nested.programs.nano.path);assert.deepEqual(freshSoftwareFileMounts('mac-docker',nested),['--mount','type=bind,src=/home/operator/previous/core/thin/nano,dst=/home/operator/previous/core/thin/nano,readonly']);
 const refused=edit=>{const value=structuredClone(mixedLinux);edit(value);assert.throws(()=>freshSoftwareLocators('mac-docker',value),/^Error: Fresh software locator:/);};
 for(const path of ['/home/operator/current-api/../current-api/nano','/home//operator/current-api/nano','/home/operator/current-api/./nano','/home/operator/current-api/nano/','relative/nano','/home/operator/current-api/nano\n','/home/operator,other/nano','/home/operator\\other/nano'])refused(value=>{value.programs.nano.path=path;});
 for(const parent of ['/config','/proof','/export','/core','/work','/control','/usr/local/bin','/opt/homebrew','/System','/Library','/Applications','/proc','/run','/private/tmp','/outputs/alternate','/network-build/alternate','/lifecycle-owner','/lifecycle-results','/staged','/audit-inputs','/stop.json','/rotation-proof.json','/request.json'])refused(value=>{value.programs.nano.path=parent+'/nano';});
 refused(value=>{value.programs.nano.path='/home/operator/current-api/nano-p2p';});
 refused(value=>{value.programs.p2p.path='/home/operator/current-api/nano-p2p';});
 refused(value=>{value.programs.nano.path='/home/operator/previous/network/nano';});
 refused(value=>{value.coreArchive.path='/home/operator/current-api/nano/nested/libnano_core.a';});
 refused(value=>{value.programs.nano.sha256='fresh-hash-is-not-authority';});
 refused(value=>{value.programs.nano.bytes=0;});
 refused(value=>{value.programs.nano.extra='unreviewed';});
 refused(value=>{value.programs.unknown=artifact('/home/operator/current-api/unknown');});
 assert.throws(()=>freshSoftwareLocators('mac-docker',mixedLinux,{protectedPaths:['/home/operator/current-api']}),/^Error: Fresh software locator:/);
 assert.throws(()=>freshSoftwareLocators('mac-docker',mixedLinux,{protectedPaths:['/home/operator/current-api/nano/seed']}),/^Error: Fresh software locator:/);
 // Path semantics cannot prove absence of filesystem symlinks or fresh
// authority. No files, process, seed, ledger, RPC or network are touched here.
});
