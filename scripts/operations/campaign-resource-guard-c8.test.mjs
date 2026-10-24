// Targeted v2 contracts, fake-time supervision and synthetic receipt files only.
// No campaign, real profile, exporter, key, Docker, SSH or process is launched.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile,mkdtemp,writeFile,chmod,unlink,symlink,link,rm,realpath} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {policy,ResourceGuardError,c8BudgetParameters,c8GuardContract,c8ProbeInspectionTemplate,c8ProbeImageTemplate,validateC8ProbeImage,validateC8ProbeIdentity,validateC8OwnedProbe,validateC8CloseoutReceipt,readC8CloseoutReceipt,publishC8GuardResult,supervise} from './campaign-resource-guard.mjs';
const revision='a'.repeat(40),sha='b'.repeat(64),cid='c'.repeat(64),uuid='d'.repeat(32),image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';
const artifact=path=>({path,sha256:sha,bytes:1,material:Array(9).fill('1')});
function contract(){
 const owners={'mac-native':{publicRole:'native'},'mac-docker':{publicRole:'arm'},'vps-docker':{host:'ubuntu@145.239.199.166'}},overlayManifest={path:'/synthetic/overlay',sha256:sha,bytes:1};
 const c={format:'nano-external-campaign-resource-guard-v2',tools:{},launcher:artifact('/synthetic/run.mjs'),spec:artifact('/synthetic/spec'),sourceBaseRevision:revision,outputDirectory:'/synthetic/guard',campaignOutputDirectory:'/synthetic/campaign',dockerEndpoint:'synthetic',proofVolume:{name:'nano-approved-sampling',driver:'local',labels:{'nano.qualification':'synthetic-readonly'}},maximumRunMs:32400000,closeoutReserveMs:2700000,softwareBindingsArtifact:artifact('/synthetic/bindings')};
 return {c,spec:{format:'nano-fresh-capella-three-owner-campaign-c8-v2',sourceBaseRevision:revision,lifecycle:{deadlines:{maximumRunMs:c.maximumRunMs}},owners,overlayManifest},bindings:{format:'nano-c8-selected-software-bindings-v1',sourceBaseRevision:revision,owners,overlayManifest}};
}
const expected={pid:42424,sourceBaseRevision:revision,specSha256:sha,maximumRunMs:90000,closeoutReserveMs:30000};
const receipt=()=>({format:'nano-fresh-c8-closeout-started-v1',phase:'closeout',controllerPid:expected.pid,sourceBaseRevision:revision,specSha256:sha,maximumRunMs:expected.maximumRunMs,closeoutReserveMs:expected.closeoutReserveMs,productionQualified:false});
const encode=value=>Buffer.from(JSON.stringify(value,null,2)+'\n');

test('v2 typed budget, exact bindings, real CLI and long-lived readonly sampler contract',async()=>{
 const f=contract(),v=c8GuardContract(f.c,f.spec,f.bindings);assert.equal(v.samplerSeconds,32400);assert(v.samplerSeconds>28800);assert.deepEqual(v.launcherArgs.slice(-2),['--closeout-reserve-ms','2700000']);assert(!v.launcherArgs.some(x=>x.includes('bindings')));assert(Object.isFrozen(v));
 for(const edit of [x=>x.c.maximumRunMs=0,x=>x.c.maximumRunMs=2**31,x=>x.c.closeoutReserveMs=x.c.maximumRunMs,x=>x.c.closeoutReserveMs='2700000',x=>x.c.extra=true,x=>x.spec.lifecycle.deadlines.maximumRunMs--,x=>x.spec.sourceBaseRevision='e'.repeat(40),x=>x.bindings.sourceBaseRevision='e'.repeat(40),x=>x.bindings.owners=structuredClone({...x.bindings.owners,'mac-docker':{foreign:true}}),x=>x.bindings.overlayManifest={...x.bindings.overlayManifest,sha256:'f'.repeat(64)},x=>x.c.proofVolume.labels={}]){
  const z=contract();edit(z);assert.throws(()=>c8GuardContract(z.c,z.spec,z.bindings),ResourceGuardError);
 }
 assert.throws(()=>c8BudgetParameters({maximumRunMs:90000,closeoutReserveMs:0}),ResourceGuardError);
 const probe={Id:cid,Name:'/nano-probe',Image:image,Config:{Labels:{'nano.external.resource-guard':uuid}},HostConfig:{Mounts:[{Type:'volume',Source:'nano-proof',Target:'/probe',ReadOnly:true,VolumeOptions:{NoCopy:true}}],RestartPolicy:{Name:'no',MaximumRetryCount:0}},Mounts:[{Type:'volume',Name:'nano-proof',Destination:'/probe',RW:false}]},binding={id:cid,name:'nano-probe',uuid,volumeName:'nano-proof'};
 assert.equal(validateC8OwnedProbe(probe,binding),probe);assert(!c8ProbeInspectionTemplate.includes('{{json .}}'));assert(!c8ProbeInspectionTemplate.includes('.Config.Env'));
 for(const edit of [x=>x.Id='e'.repeat(64),x=>x.Name='/foreign',x=>x.Image='latest',x=>x.Config.Env=['NEVER ACCEPTED'],x=>x.Config.Labels['nano.external.resource-guard']='foreign',x=>x.HostConfig.Mounts[0].VolumeOptions.NoCopy=false,x=>x.HostConfig.Mounts[0].ReadOnly=false,x=>x.Mounts[0].RW=true,x=>x.Mounts[0].Name='foreign',x=>x.Mounts.push({...x.Mounts[0]}),x=>x.HostConfig.RestartPolicy.Name='always']){const z=structuredClone(probe);edit(z);assert.throws(()=>validateC8OwnedProbe(z,binding),ResourceGuardError);}
 // A failed admission must not block cleanup of this exact owned CID.
 for(const edit of [x=>x.HostConfig.RestartPolicy.Name='always',x=>x.Mounts[0].RW=true,x=>x.HostConfig.Mounts[0].VolumeOptions.NoCopy=false]){const z=structuredClone(probe);edit(z);assert.throws(()=>validateC8OwnedProbe(z,binding),ResourceGuardError);assert.equal(validateC8ProbeIdentity(z,binding),z);}
 for(const edit of [x=>x.Id='e'.repeat(64),x=>x.Config.Labels['nano.external.resource-guard']='foreign',x=>x.Image='foreign',x=>x.Name='/foreign']){const z=structuredClone(probe);edit(z);assert.throws(()=>validateC8ProbeIdentity(z,binding),ResourceGuardError);}
 const inspectedImage={Id:image,Os:'linux',Architecture:'arm64',Volumes:null};assert.equal(validateC8ProbeImage(inspectedImage),inspectedImage);assert.equal(validateC8ProbeImage({...inspectedImage,Volumes:{}}).Id,image);
 for(const edit of [x=>x.Volumes={'/anonymous':{}},x=>delete x.Volumes,x=>x.Volumes=[],x=>x.Config={Env:['NO']},x=>x.Id='foreign',x=>x.Architecture='amd64']){const z=structuredClone(inspectedImage);edit(z);assert.throws(()=>validateC8ProbeImage(z),ResourceGuardError);}
 assert(c8ProbeImageTemplate.includes('json (index .Config "Volumes")'));assert(!c8ProbeImageTemplate.includes('{{json .}}'));assert(!c8ProbeImageTemplate.includes('.Env'));
 const source=await readFile(new URL('./campaign-resource-guard.mjs',import.meta.url),'utf8');assert(source.includes("isC8?c8ProbeImageTemplate:'{{json .}}'"));assert(source.includes("isC8?['--restart=no','--no-healthcheck']:[]"));assert(source.includes('await owned(until,{cleanup:isC8})'));
 const markerIntent="if(isC8)await privateFile('sampler-intent.json'",markerCreate="id=(await docker(['create'",markerAttempt='creationAttempted=true;';for(const marker of [markerIntent,markerCreate,markerAttempt])assert.equal(source.split(marker).length,2);assert(source.indexOf(markerIntent)<source.indexOf(markerCreate));assert(source.indexOf(markerIntent)<source.indexOf(markerAttempt));
 assert(source.includes('await directory.sync();}finally{await directory.close();}'));
});

test('closeout phase binds actual child PID, source, spec, budgets and canonical bounded bytes',()=>{
 assert.equal(validateC8CloseoutReceipt(encode(receipt()),expected),true);
 for(const edit of [x=>x.controllerPid++,x=>x.sourceBaseRevision='f'.repeat(40),x=>x.specSha256='f'.repeat(64),x=>x.maximumRunMs++,x=>x.closeoutReserveMs++,x=>x.phase='work',x=>x.productionQualified=true,x=>x.extra=true]){const z=receipt();edit(z);assert.throws(()=>validateC8CloseoutReceipt(encode(z),expected),ResourceGuardError);}
 for(const bytes of [Buffer.alloc(4097),Buffer.from('{"phase":"work","phase":"closeout"}'),Buffer.from(JSON.stringify(receipt())),Buffer.from('not JSON')])assert.throws(()=>validateC8CloseoutReceipt(bytes,expected),ResourceGuardError);
});

const GiB=1024n**3n;
const sample=now=>({startedMs:now,finishedMs:now,surfaces:Object.fromEntries(['macHost','macDockerVm','vpsHost'].map(name=>[name,{diskBytes:String(4n*GiB),memoryBytes:String(3n*GiB),memoryMetric:name==='macHost'?'mac-free-inactive-speculative-estimate':'linux-MemAvailable'}]))});
async function runScenario({phase=true,phaseError=false,closeAt=80000,overshoot=false,unjoined=false}={}){
 let at=0,signals=0,phaseReads=0;const child={pid:expected.pid,closed:false,exitCode:null,signalCode:null,kill:s=>{assert.equal(s,'SIGINT');signals++;return true;}};
 const result=await supervise({c8:{maximumRunMs:90000,closeoutReserveMs:30000},startedAt:0,clock:()=>at,sleep:async ms=>{at+=ms;if(overshoot&&at>=50000)at=95000;if(!unjoined&&((signals&&at>=70000)||(!signals&&at>=closeAt))){child.closed=true;child.exitCode=0;}},sample:async()=>sample(at),launch:async()=>child,closeoutStarted:async()=>{phaseReads++;if(phaseError)throw new ResourceGuardError('c8-closeout-receipt-refused');return phase;}});
 return {result,at,signals,phaseReads};
}
test('v2 phase-aware closeout shares the same overall deadline and refuses late terminal zero',async()=>{
 const valid=await runScenario();assert.equal(valid.result.passed,true);assert.equal(valid.result.closeoutPhaseObserved,true);assert.equal(valid.signals,0);assert.equal(valid.at,80000);
 const absent=await runScenario({phase:false});assert.equal(absent.result.reason,'campaign-work-deadline');assert.equal(absent.signals,1);assert.equal(absent.result.launcherJoined,true);assert.equal(absent.result.passed,false);
 const substituted=await runScenario({phaseError:true});assert.equal(substituted.result.reason,'c8-closeout-receipt-refused');assert.equal(substituted.signals,1);assert.equal(substituted.result.passed,false);
 const late=await runScenario({overshoot:true});assert.equal(late.result.reason,'campaign-wall-timeout');assert.equal(late.result.launcherJoined,true);assert.equal(late.result.passed,false);
 const unjoined=await runScenario({phase:false,unjoined:true});assert.equal(unjoined.at,90000);assert.equal(unjoined.signals,1);assert.equal(unjoined.result.launcherUnjoined,true);assert.equal(unjoined.result.passed,false);
 const hungClosing=await runScenario({phase:true,unjoined:true});assert.equal(hungClosing.at,90000);assert.equal(hungClosing.signals,1);assert.equal(hungClosing.result.reason,'campaign-wall-timeout');assert.equal(hungClosing.result.launcherUnjoined,true);
 assert.equal(policy.maximumRunMs,25200000);assert.equal(policy.cleanupGraceMs,1800000);assert.equal(policy.startDiskBytes,4n*GiB);assert.equal(policy.floorMemoryBytes,GiB);
});

test('v2 publication remains provisional and refuses late sync, interruption or publication failure',async()=>{
 const base={passed:true,reason:null,productionQualified:false};
 for(const cause of ['normal','late','interrupted','failed','already-late','already-interrupted']){
  let at=cause==='already-late'?100:0,interrupted=cause==='already-interrupted',written;
  const terminal=await publishC8GuardResult({result:base,overallEnd:100,clock:()=>at,interrupted:()=>interrupted,write:async value=>{written=structuredClone(value);if(cause==='late')at=100;if(cause==='interrupted')interrupted=true;if(cause==='failed')throw new Error('synthetic fsync refusal');}});
  assert.equal(written.passed,false);assert.equal(written.terminalVerdictProvisional,true);assert.equal(written.effectiveTerminalExitRequired,true);assert.equal(terminal.report.passed,false);assert.equal(terminal.report.terminalEvidenceReady,cause==='normal');assert.equal(terminal.exitCode,cause==='normal'?0:1);
  if(cause.includes('late'))assert.equal(terminal.report.reason,'campaign-wall-timeout');if(cause.includes('interrupted'))assert.equal(terminal.report.reason,'wrapper-interrupted');if(cause==='failed')assert.equal(terminal.report.reason,'c8-result-publication-refused');
 }
 const retained=await publishC8GuardResult({result:{...base,passed:false,reason:'first-causal-failure'},overallEnd:100,clock:()=>100,interrupted:()=>true,write:async()=>{throw new Error('secondary');}});assert.equal(retained.report.reason,'first-causal-failure');assert.equal(retained.exitCode,1);
});

test('actual run publishes its phase receipt before cleanup, only for C8 budget mode',async()=>{
 const source=await readFile(new URL('../../tests/multihost/run.mjs',import.meta.url),'utf8'),marker="if(campaignBudget)await terminalStage('publish-closeout-started',";
 assert.equal(source.split(marker).length,2);const line=source.split('\n').find(x=>x.includes(marker));assert(source.indexOf('campaignBudget.enterCloseout()')<source.indexOf(marker));assert(source.indexOf(marker)<source.indexOf("terminalStage('join-owned-inflight-clients'"));
 let saved;const deps={campaignBudget:{},terminalStage:async(label,fn)=>{assert.equal(label,'publish-closeout-started');return fn();},save:async(path,value)=>{assert.equal(path,'/synthetic/campaign/campaign-closeout-started.json');saved=value;},join,output:'/synthetic/campaign',process:{pid:expected.pid},revision,specSha:sha,spec:{lifecycle:{deadlines:{maximumRunMs:expected.maximumRunMs}}},campaignCloseoutReserve:expected.closeoutReserveMs};
 await Function(...Object.keys(deps),'return (async()=>{'+line+'})()')(...Object.values(deps));assert.equal(validateC8CloseoutReceipt(encode(saved),expected),true);
 saved=null;await Function(...Object.keys(deps),'return (async()=>{'+line+'})()')(...Object.values({...deps,campaignBudget:null}));assert.equal(saved,null);
});

test('synthetic receipt files enforce absence, no-follow, owner modes and unique regular inode',async()=>{
 const directory=await realpath(await mkdtemp(join(tmpdir(),'nano-public-c8-receipt-'))),file=join(directory,'campaign-closeout-started.json'),other=join(directory,'other');await chmod(directory,0o700);
 try{
  assert.equal(await readC8CloseoutReceipt(directory,expected),false);await writeFile(file,encode(receipt()),{mode:0o600,flag:'wx'});assert.equal(await readC8CloseoutReceipt(directory,expected),true);
  await chmod(file,0o644);await assert.rejects(readC8CloseoutReceipt(directory,expected),ResourceGuardError);await chmod(file,0o600);
  await link(file,other);await assert.rejects(readC8CloseoutReceipt(directory,expected),ResourceGuardError);await unlink(other);
  await unlink(file);await writeFile(other,encode(receipt()),{mode:0o600});await symlink(other,file);await assert.rejects(readC8CloseoutReceipt(directory,expected),ResourceGuardError);
 }finally{await rm(directory,{recursive:true,force:true});}
});
